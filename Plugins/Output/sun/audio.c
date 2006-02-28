/*
 *  Copyright (C) 2001  CubeSoft Communications, Inc.
 *  <http://www.csoft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* FIXME: g_error() is used several places, it will exit xmms */

#include <errno.h>
#include "libaudacious/util.h"
#include "sun.h"
#include "resample.h"

static int sun_bps(int, int, int);
static int sun_format(AFormat);

static void sun_setformat(AFormat, int, int);
static void sun_setparams(void);
static void *sun_loop(void *);
static int sun_downsample(gpointer, guint, guint, guint);

static gboolean	prebuffer, remove_prebuffer;
static pthread_t buffer_thread;
static int (*sun_convert)(void **, int);
static int realtime;
static int rd_index, wr_index;
static int buffer_size;
static int prebuffer_size;
static int output_time_offset;
static int device_buffer_used;
static int blocksize;
static char *buffer;
static guint64 output_bytes;
static guint64 written;

/*
 * The format of the data from the input plugin
 * This will never change during a song. 
 */
struct sun_format input;

/*
 * The format we get from the effect plugin.
 * This will be different from input if the effect plugin does
 * some kind of format conversion.
 */
struct sun_format effect;

/*
 * The format of the data we actually send to the soundcard.
 * This might be different from effect if we need to resample or do
 * some other format conversion.
 */
struct sun_format output;

static int sun_bps(int sunfmt, int rate, int nch)
{
	int bitrate;

	bitrate = rate * nch;

	switch (sunfmt)
	{
		case AUDIO_ENCODING_ULINEAR_BE:
		case AUDIO_ENCODING_ULINEAR_LE:
		case AUDIO_ENCODING_SLINEAR_BE:
		case AUDIO_ENCODING_SLINEAR_LE:
			bitrate *= 2;
			break;
	}

	return (bitrate);
}

static int sun_format(AFormat fmt)
{
	switch (fmt)
	{
		case FMT_U8:
			return (AUDIO_ENCODING_PCM8);
		case FMT_S8:
			return (AUDIO_ENCODING_SLINEAR);
		case FMT_U16_LE:
			return (AUDIO_ENCODING_ULINEAR_LE);
		case FMT_U16_BE:
			return (AUDIO_ENCODING_ULINEAR_BE);
		case FMT_U16_NE:
#ifdef WORDS_BIGENDIAN
			return (AUDIO_ENCODING_ULINEAR_BE);
#else
			return (AUDIO_ENCODING_ULINEAR_LE);
#endif
		case FMT_S16_LE:
			return (AUDIO_ENCODING_SLINEAR_LE);
		case FMT_S16_BE:
			return (AUDIO_ENCODING_SLINEAR_BE);
		case FMT_S16_NE:
#ifdef WORDS_BIGENDIAN
			return (AUDIO_ENCODING_SLINEAR_BE);
#else
			return (AUDIO_ENCODING_SLINEAR_LE);
#endif
	}
	return -1;
}

static void sun_setformat(AFormat fmt, int rate, int nch)
{
	int sun;
	
	sun = sun_format(fmt);

	effect.format.sun = sun;
	effect.format.xmms = fmt;
	effect.frequency = rate;
	effect.channels = nch;
	effect.bps = sun_bps(sun, rate, nch);

	output.format.sun = sun;
	output.format.xmms = fmt;
	output.frequency = rate;
	output.channels = nch;
	sun_setparams();

	output.bps = sun_bps(output.format.sun, output.frequency,
			     output.channels);

	audio.input = &input;
	audio.output = &output;
	audio.effect = &effect;
}

void sun_setparams(void)
{
	audio_info_t info;
	audio_encoding_t enc;

	AUDIO_INITINFO(&info);

	info.mode = AUMODE_PLAY;
	if (ioctl(audio.fd, AUDIO_SETINFO, &info) != 0)
	{
		g_error("%s: cannot play (%s)", audio.devaudio,
			strerror(errno));
		return;
	}

	/*
	 * Pass 1: try the preferred encoding, if it is supported.
	 */
	enc.index = 0;
	while (ioctl(audio.fd, AUDIO_GETENC, &enc) == 0 &&
	       enc.encoding != output.format.sun)
		enc.index++;

	info.play.encoding = enc.encoding;
	info.play.precision = enc.precision;
	strcpy(output.name, enc.name);
	if (ioctl(audio.fd, AUDIO_SETINFO, &info) != 0)
	{
		g_error("%s: unsupported encoding: %s (%s)", audio.devaudio,
			output.name, strerror(errno));
		return;
	}

	info.play.channels = output.channels;
	ioctl(audio.fd, AUDIO_SETINFO, &info);

	info.play.sample_rate = output.frequency;
	if (ioctl(audio.fd, AUDIO_SETINFO, &info) < 0)
	{
		g_error("%s: cannot handle %i Hz (%s)", audio.devaudio,
			output.frequency, strerror(errno));
		return;
	}

	if (ioctl(audio.fd, AUDIO_GETINFO, &info) != 0)
	{
		blocksize = SUN_DEFAULT_BLOCKSIZE;
		output.channels = info.play.channels;
	}
	else
	{
		blocksize = blocksize;
	}

	sun_convert = sun_get_convert_func(output.format.sun,
					   sun_format(effect.format.xmms));
#if 0
	if (sun_convert != NULL)
	{
		g_warning("audio conversion (output=0x%x effect=0x%x)",
		    output.format.sun, sun_format(effect.format.xmms));
	}
#endif
}

static inline void sun_bufused(void)
{
	audio_offset_t ooffs;

	if (audio.paused)
		device_buffer_used = 0;
	else if (ioctl(audio.fd, AUDIO_GETOOFFS, &ooffs) == 0)
		device_buffer_used = ooffs.offset;
}

int sun_written_time(void)
{
	if (!audio.going)
		return 0;

	return ((written * 1000) / effect.bps);
}

int sun_output_time(void)
{
	guint64 bytes;

	if (!audio.fd || !audio.going)
		return 0;

	if (realtime)
		sun_bufused();

	bytes = output_bytes < device_buffer_used ?
		0 : output_bytes - device_buffer_used;
	return (output_time_offset + ((bytes * 1000) / output.bps));
}

static inline int sun_used(void)
{
	if (realtime)
		return 0;
	
	if (wr_index >= rd_index)
		return (wr_index - rd_index);

	return (buffer_size - (rd_index - wr_index));
}

int sun_playing(void)
{
	if (!audio.going)
		return 0;

	if (realtime)
		sun_bufused();

	if (!sun_used() && (device_buffer_used - (3 * blocksize)) <= 0)
		return (FALSE);

	return (TRUE);
}

int sun_free(void)
{
	if (realtime)
		return (audio.paused ? 0 : 1000000);
	
	if (remove_prebuffer && prebuffer)
	{
		prebuffer = FALSE;
		remove_prebuffer = FALSE;
	}
	if (prebuffer)
		remove_prebuffer = TRUE;

	if (rd_index > wr_index)
		return ((rd_index - wr_index) - blocksize - 1);

	return ((buffer_size - (wr_index - rd_index)) - blocksize - 1);
}

static inline ssize_t write_all(int fd, const void *buf, size_t count)
{
	static ssize_t done;

	for (done = 0; count > done; )
	{
		static ssize_t n;

		n = write(fd, buf, count - done);
		if (n == -1)
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		done += n;
	}

	return (done);
}

static inline void sun_write_audio(gpointer data, int length)
{
	AFormat new_format;
	EffectPlugin *ep;
	int new_frequency, new_channels;

	new_format = input.format.xmms;
	new_frequency = input.frequency;
	new_channels = input.channels;

	ep = get_current_effect_plugin();
	if (effects_enabled() && ep && ep->query_format)
		ep->query_format(&new_format, &new_frequency, &new_channels);

	if (new_format != effect.format.xmms || 
	    new_frequency != effect.frequency ||
	    new_channels != effect.channels)
	{
		output_time_offset += (output_bytes * 1000) / output.bps;
		output_bytes = 0;
		close(audio.fd);
		audio.fd = open(audio.devaudio, O_RDWR);
		sun_setformat(new_format, new_frequency, new_channels);
	}
	if (effects_enabled() && ep && ep->mod_samples)
	{
		length = ep->mod_samples(&data, length, input.format.xmms,
					 input.frequency, input.channels);
	}

	if (sun_convert != NULL)
		length = sun_convert(&data, length);

	if (effect.frequency == output.frequency)
	{
		output_bytes += write_all(audio.fd, data, length);
	}
	else
	{
		output_bytes += sun_downsample(data, length,
		    effect.frequency, output.frequency);
	}
}

static void sun_bswap16(guint16 *data, int len)
{
	int i;

	for (i = 0; i < len; i += 2, data++)
		*data = GUINT16_SWAP_LE_BE(*data);
}

static int sun_downsample(gpointer ob, guint length, guint speed, guint espeed)
{
	guint w = 0;
	static gpointer nbuffer = NULL;
	static int nbuffer_size = 0;

	switch (output.format.sun) {
		case AUDIO_ENCODING_SLINEAR_BE:
		case AUDIO_ENCODING_SLINEAR_LE:
			if (output.channels == 2)
				RESAMPLE_STEREO(gint16);
			else
				RESAMPLE_MONO(gint16);
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
		case AUDIO_ENCODING_ULINEAR_LE:
			if (output.channels == 2)
				RESAMPLE_STEREO(guint16);
			else
				RESAMPLE_MONO(guint16);
			break;
		case AUDIO_ENCODING_SLINEAR:
			if (output.channels == 2)
				RESAMPLE_STEREO(gint8);
			else
				RESAMPLE_MONO(gint8);
			break;
		case AUDIO_ENCODING_ULINEAR:
			if (output.channels == 2)
				RESAMPLE_STEREO(guint8);
			else
				RESAMPLE_MONO(guint8);
			break;
	}
	return (w);
}

void sun_write(gpointer ptr, int length)
{
	int cnt, off = 0;

	if (realtime)
	{
		if (audio.paused)
			return;
		sun_write_audio(ptr, length);
		written += length;
		return;
	}

	remove_prebuffer = FALSE;
	written += length;
	while (length > 0)
	{
		cnt = MIN(length, buffer_size - wr_index);
		memcpy(buffer + wr_index, (char *) ptr + off, cnt);
		wr_index = (wr_index + cnt) % buffer_size;
		length -= cnt;
		off += cnt;
	}
}

void sun_close(void)
{
	if (!audio.going)
		return;

	audio.going = 0;

	if (realtime)
	{
		ioctl(audio.fd, AUDIO_FLUSH, NULL);
		close(audio.fd);
	}
	else
	{
		pthread_join(buffer_thread, NULL);
	}

	sun_get_convert_buffer(0);
	wr_index = 0;
	rd_index = 0;
}

void sun_flush(int time)
{
	ioctl(audio.fd, AUDIO_FLUSH, NULL);

	output_time_offset = time;
	written = (guint16)(time / 10) * (guint64)(input.bps / 100);
	output_bytes = 0;
}

void sun_pause(short p)
{
	if (!realtime)
	{
		if (p == TRUE)
			audio.do_pause = TRUE;
		else
			audio.unpause = TRUE;
	}
	else
		audio.paused = p;
}

static void* sun_loop(void *arg)
{
	struct timeval tv;
	int length, cnt;
	fd_set set;

	while (audio.going)
	{
		if (sun_used() > prebuffer_size)
			prebuffer = FALSE;

		if (sun_used() > 0 && !audio.paused && !prebuffer)
		{
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			FD_ZERO(&set);
			FD_SET(audio.fd, &set);

			if (select(audio.fd + 1, NULL, &set, NULL, &tv) > 0)
			{
				length = MIN(blocksize, sun_used());
				while (length > 0)
				{
					cnt = MIN(length,
					    buffer_size - rd_index);
					sun_write_audio(
					    buffer + rd_index, cnt);
					rd_index = (rd_index + cnt) %
					    buffer_size;
					length -= cnt;
				}
			}
		}
		else
			xmms_usleep(10000);

		sun_bufused();

		if (audio.do_pause && !audio.paused)
		{
			audio.do_pause = FALSE;
			audio.paused = TRUE;

			rd_index -= device_buffer_used;
			output_bytes -= device_buffer_used;
			if (rd_index < 0)
				rd_index += buffer_size;
			ioctl(audio.fd, AUDIO_FLUSH, NULL);
		}
		else if (audio.unpause && audio.paused)
		{
			audio.unpause = FALSE;
			close(audio.fd);
			audio.fd = open(audio.devaudio, O_RDWR);
			sun_setparams();
			audio.paused = FALSE;
		}
	}

	close(audio.fd);
	g_free(buffer);
	pthread_exit(NULL);
}

int sun_open(AFormat fmt, int rate, int nch)
{
	audio_info_t info;

	AUDIO_INITINFO(&info);

	if ((audio.fd = open(audio.devaudio, O_RDWR)) < 0)
	{
		g_error("%s: %s", audio.devaudio, strerror(errno));
		return 0;
	}

	input.format.xmms = fmt;
	input.frequency = rate;
	input.channels = nch;
	input.bps = sun_bps(sun_format(fmt), rate, nch);
	sun_setformat(fmt, rate, nch);

	realtime = xmms_check_realtime_priority();

	if (ioctl(audio.fd, AUDIO_GETINFO, &info) != 0)
		blocksize = SUN_DEFAULT_BLOCKSIZE;
	else
		blocksize = info.blocksize;

	if (!realtime)
	{
		buffer_size = audio.req_buffer_size;

		if (buffer_size < SUN_MIN_BUFFER_SIZE)
			buffer_size = SUN_MIN_BUFFER_SIZE;

		prebuffer_size = (buffer_size * audio.req_prebuffer_size) / 100;

		buffer_size += blocksize;
		buffer = g_malloc0(buffer_size);
	}
	prebuffer = TRUE;
	wr_index = 0;
	rd_index = 0;
	output_time_offset = 0;
	written = 0;
	output_bytes = 0;

	audio.paused = FALSE;
	audio.do_pause = FALSE;
	audio.unpause = FALSE;
	remove_prebuffer = FALSE;

	audio.going++;
	if (!realtime)
		pthread_create(&buffer_thread, NULL, sun_loop, NULL);

	return 1;
}
