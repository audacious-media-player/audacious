/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#include <glib.h>
#include <libaudacious/util.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "OSS.h"


#define NFRAGS		32

static gint fd = 0;
static char *buffer;
static gboolean going, prebuffer, paused, unpause, do_pause, remove_prebuffer;
static gint buffer_size, prebuffer_size, blk_size;
static gint rd_index = 0, wr_index = 0;
static gint output_time_offset = 0;
static guint64 written = 0, output_bytes = 0, device_buffer_used;
static gint flush;
static gint fragsize, device_buffer_size;
static gchar *device_name;
static GThread *buffer_thread;
static gboolean realtime, select_works;

static int (*oss_convert_func) (void **data, int length);
static int (*oss_stereo_convert_func) (void **data, int length, int fmt);

struct format_info {
    union {
        AFormat xmms;
        int oss;
    } format;
    int frequency;
    int channels;
    int bps;
};


/*
 * The format of the data from the input plugin
 * This will never change during a song. 
 */
struct format_info input;

/*
 * The format we get from the effect plugin.
 * This will be different from input if the effect plugin does
 * some kind of format conversion.
 */
struct format_info effect;

/*
 * The format of the data we actually send to the soundcard.
 * This might be different from effect if we need to resample or do
 * some other format conversion.
 */
struct format_info output;


static void
oss_calc_device_buffer_used(void)
{
    audio_buf_info buf_info;
    if (paused)
        device_buffer_used = 0;
    else if (!ioctl(fd, SNDCTL_DSP_GETOSPACE, &buf_info))
        device_buffer_used =
            (buf_info.fragstotal * buf_info.fragsize) - buf_info.bytes;
}


static gint oss_downsample(gpointer ob, guint length, guint speed,
                           guint espeed);

static int
oss_calc_bitrate(int oss_fmt, int rate, int channels)
{
    int bitrate = rate * channels;

    if (oss_fmt == AFMT_U16_BE || oss_fmt == AFMT_U16_LE ||
        oss_fmt == AFMT_S16_BE || oss_fmt == AFMT_S16_LE)
        bitrate *= 2;

    return bitrate;
}

static int
oss_get_format(AFormat fmt)
{
    int format = 0;

    switch (fmt) {
    case FMT_U8:
        format = AFMT_U8;
        break;
    case FMT_S8:
        format = AFMT_S8;
        break;
    case FMT_U16_LE:
        format = AFMT_U16_LE;
        break;
    case FMT_U16_BE:
        format = AFMT_U16_BE;
        break;
    case FMT_U16_NE:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
        format = AFMT_U16_BE;
#else
        format = AFMT_U16_LE;
#endif
        break;
    case FMT_S16_LE:
        format = AFMT_S16_LE;
        break;
    case FMT_S16_BE:
        format = AFMT_S16_BE;
        break;
    case FMT_S16_NE:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
        format = AFMT_S16_BE;
#else
        format = AFMT_S16_LE;
#endif
        break;
    }

    return format;
}

static void
oss_setup_format(AFormat fmt, int rate, int nch)
{
    effect.format.xmms = fmt;
    effect.frequency = rate;
    effect.channels = nch;
    effect.bps = oss_calc_bitrate(oss_get_format(fmt), rate, nch);

    output.format.oss = oss_get_format(fmt);
    output.frequency = rate;
    output.channels = nch;


    fragsize = 0;
    while ((1L << fragsize) < effect.bps / 25)
        fragsize++;
    fragsize--;

    device_buffer_size = ((1L << fragsize) * (NFRAGS + 1));

    oss_set_audio_params();

    output.bps = oss_calc_bitrate(output.format.oss, output.frequency,
                                  output.channels);
}


gint
oss_get_written_time(void)
{
    if (!going)
        return 0;
    return (written * 1000) / effect.bps;
}

gint
oss_get_output_time(void)
{
    guint64 bytes;

    if (!fd || !going)
        return 0;

    if (realtime)
        oss_calc_device_buffer_used();
    bytes = output_bytes < device_buffer_used ?
        0 : output_bytes - device_buffer_used;

    return output_time_offset + ((bytes * 1000) / output.bps);
}

static int
oss_used(void)
{
    if (realtime)
        return 0;
    else {
        if (wr_index >= rd_index)
            return wr_index - rd_index;
        return buffer_size - (rd_index - wr_index);
    }
}

gint
oss_playing(void)
{
    if (!going)
        return 0;
    if (realtime)
        oss_calc_device_buffer_used();
    if (!oss_used() && (device_buffer_used - (3 * blk_size)) <= 0)
        return FALSE;

    return TRUE;
}

gint
oss_free(void)
{
    if (!realtime) {
        if (remove_prebuffer && prebuffer) {
            prebuffer = FALSE;
            remove_prebuffer = FALSE;
        }
        if (prebuffer)
            remove_prebuffer = TRUE;

        if (rd_index > wr_index)
            return (rd_index - wr_index) - device_buffer_size - 1;
        return (buffer_size - (wr_index - rd_index)) - device_buffer_size - 1;
    }
    else if (paused)
        return 0;
    else
        return 1000000;
}

static inline ssize_t
write_all(int fd, const void *buf, size_t count)
{
    size_t done = 0;
    do {
        ssize_t n = write(fd, (gchar *) buf + done, count - done);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            else
                break;
        }
        done += n;
    } while (count > done);

    return done;
}

static void
oss_write_audio(gpointer data, int length)
{

    audio_buf_info abuf_info;
    AFormat new_format;
    int new_frequency, new_channels;
    EffectPlugin *ep;

    new_format = input.format.xmms;
    new_frequency = input.frequency;
    new_channels = input.channels;


    ep = get_current_effect_plugin();
    if (effects_enabled() && ep && ep->query_format) {
        ep->query_format(&new_format, &new_frequency, &new_channels);
    }

    if (new_format != effect.format.xmms ||
        new_frequency != effect.frequency ||
        new_channels != effect.channels) {
        output_time_offset += (output_bytes * 1000) / output.bps;
        output_bytes = 0;
        close(fd);
        fd = open(device_name, O_WRONLY);
        oss_setup_format(new_format, new_frequency, new_channels);
    }
    if (effects_enabled() && ep && ep->mod_samples)
        length = ep->mod_samples(&data, length,
                                 input.format.xmms,
                                 input.frequency, input.channels);
    if (realtime && !ioctl(fd, SNDCTL_DSP_GETOSPACE, &abuf_info)) {
        while (abuf_info.bytes < length) {
            xmms_usleep(10000);
            if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &abuf_info))
                break;
        }
    }

    if (oss_convert_func != NULL)
        length = oss_convert_func(&data, length);

    if (oss_stereo_convert_func != NULL)
        length = oss_stereo_convert_func(&data, length, output.format.oss);

    if (effect.frequency == output.frequency)
        output_bytes += write_all(fd, data, length);
    else
        output_bytes += oss_downsample(data, length,
                                       effect.frequency, output.frequency);
}

static void
swap_endian(guint16 * data, int length)
{
    int i;
    for (i = 0; i < length; i += 2, data++)
        *data = GUINT16_SWAP_LE_BE(*data);
}

#define NOT_NATIVE_ENDIAN ((IS_BIG_ENDIAN &&				\
			   (output.format.oss == AFMT_S16_LE ||		\
			    output.format.oss == AFMT_U16_LE)) ||	\
			  (!IS_BIG_ENDIAN &&				\
			   (output.format.oss == AFMT_S16_BE ||		\
			    output.format.oss == AFMT_U16_BE)))


#define RESAMPLE_STEREO(sample_type)				\
do {								\
	const gint shift = sizeof (sample_type);		\
        gint i, in_samples, out_samples, x, delta;		\
	sample_type *inptr = (sample_type *)ob, *outptr;	\
	guint nlen = (((length >> shift) * espeed) / speed);	\
	if (nlen == 0)						\
		break;						\
	nlen <<= shift;						\
	if (NOT_NATIVE_ENDIAN)					\
		swap_endian(ob, length);			\
	if(nlen > nbuffer_size)					\
	{							\
		nbuffer = g_realloc(nbuffer, nlen);		\
		nbuffer_size = nlen;				\
	}							\
	outptr = (sample_type *)nbuffer;			\
	in_samples = length >> shift;				\
        out_samples = nlen >> shift;				\
	delta = (in_samples << 12) / out_samples;		\
	for (x = 0, i = 0; i < out_samples; i++)		\
	{							\
		gint x1, frac;					\
		x1 = (x >> 12) << 12;				\
		frac = x - x1;					\
		*outptr++ =					\
			(sample_type)				\
			((inptr[(x1 >> 12) << 1] *		\
			  ((1<<12) - frac) +			\
			  inptr[((x1 >> 12) + 1) << 1] *	\
			  frac) >> 12);				\
		*outptr++ =					\
			(sample_type)				\
			((inptr[((x1 >> 12) << 1) + 1] *	\
			  ((1<<12) - frac) +			\
			  inptr[(((x1 >> 12) + 1) << 1) + 1] *	\
			  frac) >> 12);				\
		x += delta;					\
	}							\
	if (NOT_NATIVE_ENDIAN)					\
		swap_endian(nbuffer, nlen);			\
	w = write_all(fd, nbuffer, nlen);			\
} while (0)


#define RESAMPLE_MONO(sample_type)				\
do {								\
	const gint shift = sizeof (sample_type) - 1;		\
        gint i, x, delta, in_samples, out_samples;		\
	sample_type *inptr = (sample_type *)ob, *outptr;	\
	guint nlen = (((length >> shift) * espeed) / speed);	\
	if (nlen == 0)						\
		break;						\
	nlen <<= shift;						\
	if (NOT_NATIVE_ENDIAN)					\
		swap_endian(ob, length);			\
	if(nlen > nbuffer_size)					\
	{							\
		nbuffer = g_realloc(nbuffer, nlen);		\
		nbuffer_size = nlen;				\
	}							\
	outptr = (sample_type *)nbuffer;			\
	in_samples = length >> shift;				\
        out_samples = nlen >> shift;				\
	delta = ((length >> shift) << 12) / out_samples;	\
	for (x = 0, i = 0; i < out_samples; i++)		\
	{							\
		gint x1, frac;					\
		x1 = (x >> 12) << 12;				\
		frac = x - x1;					\
		*outptr++ =					\
			(sample_type)				\
			((inptr[x1 >> 12] * ((1<<12) - frac) +	\
			  inptr[(x1 >> 12) + 1] * frac) >> 12);	\
		x += delta;					\
	}							\
	if (NOT_NATIVE_ENDIAN)					\
		swap_endian(nbuffer, nlen);			\
	w = write_all(fd, nbuffer, nlen);			\
} while (0)


static gint
oss_downsample(gpointer ob, guint length, guint speed, guint espeed)
{
    guint w = 0;
    static gpointer nbuffer = NULL;
    static guint nbuffer_size = 0;

    switch (output.format.oss) {
    case AFMT_S16_BE:
    case AFMT_S16_LE:
        if (output.channels == 2)
            RESAMPLE_STEREO(gint16);
        else
            RESAMPLE_MONO(gint16);
        break;
    case AFMT_U16_BE:
    case AFMT_U16_LE:
        if (output.channels == 2)
            RESAMPLE_STEREO(guint16);
        else
            RESAMPLE_MONO(guint16);
        break;
    case AFMT_S8:
        if (output.channels == 2)
            RESAMPLE_STEREO(gint8);
        else
            RESAMPLE_MONO(gint8);
        break;
    case AFMT_U8:
        if (output.channels == 2)
            RESAMPLE_STEREO(guint8);
        else
            RESAMPLE_MONO(guint8);
        break;
    }
    return w;
}

void
oss_write(gpointer ptr, int length)
{
    int cnt, off = 0;

    if (!realtime) {
        remove_prebuffer = FALSE;

        written += length;
        while (length > 0) {
            cnt = MIN(length, buffer_size - wr_index);
            memcpy(buffer + wr_index, (char *) ptr + off, cnt);
            wr_index = (wr_index + cnt) % buffer_size;
            length -= cnt;
            off += cnt;
        }
    }
    else {
        if (paused)
            return;
        oss_write_audio(ptr, length);
        written += length;
    }
}

void
oss_close(void)
{
    if (!going)
        return;
    going = 0;
    if (!realtime)
        g_thread_join(buffer_thread);
    else {
        ioctl(fd, SNDCTL_DSP_RESET, 0);
        close(fd);
    }
    g_free(device_name);
    oss_free_convert_buffer();
    wr_index = 0;
    rd_index = 0;
}

void
oss_flush(gint time)
{
    if (!realtime) {
        flush = time;
        while (flush != -1)
            xmms_usleep(10000);
    }
    else {
        ioctl(fd, SNDCTL_DSP_RESET, 0);
        close(fd);
        fd = open(device_name, O_WRONLY);
        oss_set_audio_params();
        output_time_offset = time;
        written = ((guint64) time * input.bps) / 1000;
        output_bytes = 0;
    }
}

void
oss_pause(short p)
{
    if (!realtime) {
        if (p == TRUE)
            do_pause = TRUE;
        else
            unpause = TRUE;
    }
    else
        paused = p;

}

gpointer
oss_loop(gpointer arg)
{
    gint length, cnt;
    fd_set set;
    struct timeval tv;

    while (going) {
        if (oss_used() > prebuffer_size)
            prebuffer = FALSE;
        if (oss_used() > 0 && !paused && !prebuffer) {
            tv.tv_sec = 0;
            tv.tv_usec = 10000;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            if (!select_works || (select(fd + 1, NULL, &set, NULL, &tv) > 0)) {
                length = MIN(blk_size, oss_used());
                while (length > 0) {
                    cnt = MIN(length, buffer_size - rd_index);
                    oss_write_audio(buffer + rd_index, cnt);
                    rd_index = (rd_index + cnt) % buffer_size;
                    length -= cnt;
                }
                if (!oss_used())
                    ioctl(fd, SNDCTL_DSP_POST, 0);
            }
        }
        else
            xmms_usleep(10000);
        oss_calc_device_buffer_used();
        if (do_pause && !paused) {
            do_pause = FALSE;
            paused = TRUE;
            /*
             * We lose some data here that is sent to the
             * soundcard, but not yet played.  I don't
             * think this is worth fixing.
             */
            ioctl(fd, SNDCTL_DSP_RESET, 0);
        }
        else if (unpause && paused) {
            unpause = FALSE;
            close(fd);
            fd = open(device_name, O_WRONLY);
            oss_set_audio_params();
            paused = FALSE;
        }

        if (flush != -1) {
            /*
             * This close and open is a work around of a
             * bug that exists in some drivers which cause
             * the driver to get fucked up by a reset
             */

            ioctl(fd, SNDCTL_DSP_RESET, 0);
            close(fd);
            fd = open(device_name, O_WRONLY);
            oss_set_audio_params();
            output_time_offset = flush;
            written = ((guint64) flush * input.bps) / 1000;
            rd_index = wr_index = output_bytes = 0;
            flush = -1;
            prebuffer = TRUE;
        }

    }

    ioctl(fd, SNDCTL_DSP_RESET, 0);
    close(fd);
    g_free(buffer);
    return NULL;
}

void
oss_set_audio_params(void)
{
    int frag, stereo, ret;
    struct timeval tv;
    fd_set set;

    ioctl(fd, SNDCTL_DSP_RESET, 0);
    frag = (NFRAGS << 16) | fragsize;
    ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag);
    /*
     * Set the stream format.  This ioctl() might fail, but should
     * return a format that works if it does.
     */
    ioctl(fd, SNDCTL_DSP_SETFMT, &output.format.oss);
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &output.format.oss) == -1)
        g_warning("SNDCTL_DSP_SETFMT ioctl failed: %s", strerror(errno));

    stereo = output.channels - 1;
    ioctl(fd, SNDCTL_DSP_STEREO, &stereo);
    output.channels = stereo + 1;

    oss_stereo_convert_func = oss_get_stereo_convert_func(output.channels,
                                                          effect.channels);

    if (ioctl(fd, SNDCTL_DSP_SPEED, &output.frequency) == -1)
        g_warning("SNDCTL_DSP_SPEED ioctl failed: %s", strerror(errno));

    if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &blk_size) == -1)
        blk_size = 1L << fragsize;

    oss_convert_func =
        oss_get_convert_func(output.format.oss,
                             oss_get_format(effect.format.xmms));

    /*
     * Stupid hack to find out if the driver support selects, some
     * drivers won't work properly without a select and some won't
     * work with a select :/
     */

    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    ret = select(fd + 1, NULL, &set, NULL, &tv);
    if (ret > 0)
        select_works = TRUE;
    else
        select_works = FALSE;
}

gint
oss_open(AFormat fmt, gint rate, gint nch)
{

    if (oss_cfg.use_alt_audio_device && oss_cfg.alt_audio_device)
        device_name = g_strdup(oss_cfg.alt_audio_device);
    else {
        if (oss_cfg.audio_device > 0)
            device_name =
                g_strdup_printf("%s%d", DEV_DSP, oss_cfg.audio_device);
        else
            device_name = g_strdup(DEV_DSP);
    }

    fd = open(device_name, O_WRONLY);

    if (fd == -1) {
        g_warning("oss_open(): Failed to open audio device (%s): %s",
                  device_name, strerror(errno));
        g_free(device_name);
        return 0;
    }

    input.format.xmms = fmt;
    input.frequency = rate;
    input.channels = nch;
    input.bps = oss_calc_bitrate(oss_get_format(fmt), rate, nch);

    oss_setup_format(fmt, rate, nch);

    realtime = xmms_check_realtime_priority();

    if (!realtime) {
        buffer_size = (oss_cfg.buffer_size * input.bps) / 1000;
        if (buffer_size < 8192)
            buffer_size = 8192;
        prebuffer_size = (buffer_size * oss_cfg.prebuffer) / 100;
        if (buffer_size - prebuffer_size < 4096)
            prebuffer_size = buffer_size - 4096;

        buffer_size += device_buffer_size;
        buffer = g_malloc0(buffer_size);
    }
    flush = -1;
    prebuffer = TRUE;
    wr_index = rd_index = output_time_offset = written = output_bytes = 0;
    paused = FALSE;
    do_pause = FALSE;
    unpause = FALSE;
    remove_prebuffer = FALSE;

    going = 1;
    if (!realtime)
        buffer_thread = g_thread_create(oss_loop, NULL, TRUE, NULL);
    return 1;
}

void oss_tell(AFormat * fmt, gint * rate, gint * nch)
{
	(*fmt) = input.format.xmms;
	(*rate) = input.frequency;
	(*nch) = input.channels;
}
