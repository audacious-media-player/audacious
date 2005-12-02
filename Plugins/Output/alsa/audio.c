/*  XMMS - ALSA output plugin
 *  Copyright (C) 2001-2003 Matthieu Sozeau <mattam@altern.org>
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2005  Haavard Kvaalen
 *  Copyright (C) 2005       Takashi Iwai
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

/*
 *  CHANGES
 *
 *  2005.01.05  Takashi Iwai <tiwai@suse.de>
 *	Impelemented the multi-threaded mode with an audio-thread.
 *	Many fixes and cleanups.
 */


#include "alsa.h"
#include <ctype.h>
#include <libaudacious/xconvert.h>

static snd_pcm_t *alsa_pcm;
static snd_output_t *logs;


/* Number of bytes that we have received from the input plugin */
static guint64 alsa_total_written;
/* Number of bytes that we have sent to the sound card */
static guint64 alsa_hw_written;
static guint64 output_time_offset;

/* device buffer/period sizes in bytes */
static int hw_buffer_size, hw_period_size;		/* in output bytes */
static int hw_buffer_size_in, hw_period_size_in;	/* in input bytes */

/* Set/Get volume */
static snd_mixer_elem_t *pcm_element;
static snd_mixer_t *mixer;

static gboolean going, paused, mixer_start = TRUE;
static gboolean prebuffer, remove_prebuffer;

static gboolean alsa_can_pause;

/* for audio thread */
static GThread *audio_thread;	 /* audio loop thread */
static int thread_buffer_size;	 /* size of intermediate buffer in bytes */
static char *thread_buffer;	 /* audio intermediate buffer */
static int rd_index, wr_index;	 /* current read/write position in int-buffer */
static gboolean pause_request;	 /* pause status currently requested */
static int flush_request;	 /* flush status (time) currently requested */
static int prebuffer_size;

static guint mixer_timeout;

struct snd_format {
	unsigned int rate;
	unsigned int channels;
	snd_pcm_format_t format;
	AFormat xmms_format;
	int sample_bits;
	int bps;
};

static struct snd_format *inputf = NULL;
static struct snd_format *effectf = NULL;
static struct snd_format *outputf = NULL;

static int alsa_setup(struct snd_format *f);
static void alsa_write_audio(char *data, int length);
static void alsa_cleanup_mixer(void);
static int get_thread_buffer_filled(void);

static struct snd_format * snd_format_from_xmms(AFormat fmt, int rate, int channels);

static struct xmms_convert_buffers *convertb;

static convert_func_t alsa_convert_func;
static convert_channel_func_t alsa_stereo_convert_func;
static convert_freq_func_t alsa_frequency_convert_func;

static const struct {
	AFormat xmms;
	snd_pcm_format_t alsa;
} format_table[] =
{{FMT_S16_LE, SND_PCM_FORMAT_S16_LE},
 {FMT_S16_BE, SND_PCM_FORMAT_S16_BE},
 {FMT_S16_NE, SND_PCM_FORMAT_S16},
 {FMT_U16_LE, SND_PCM_FORMAT_U16_LE},
 {FMT_U16_BE, SND_PCM_FORMAT_U16_BE},
 {FMT_U16_NE, SND_PCM_FORMAT_U16},
 {FMT_U8, SND_PCM_FORMAT_U8},
 {FMT_S8, SND_PCM_FORMAT_S8},
};


static void debug(char *str, ...) G_GNUC_PRINTF(1, 2);

static void debug(char *str, ...)
{
	va_list args;

	if (alsa_cfg.debug)
	{
		va_start(args, str);
		g_logv(NULL, G_LOG_LEVEL_MESSAGE, str, args);
		va_end(args);
	}
}

int alsa_playing(void)
{
	if (!going || paused || alsa_pcm == NULL)
		return FALSE;

	return snd_pcm_state(alsa_pcm) == SND_PCM_STATE_RUNNING;
}

static int xrun_recover(void)
{
	if (alsa_cfg.debug)
	{
		snd_pcm_status_t *alsa_status;
		snd_pcm_status_alloca(&alsa_status);
		if (snd_pcm_status(alsa_pcm, alsa_status) < 0)
			g_warning("xrun_recover(): snd_pcm_status() failed");
		else
		{
			printf("Status:\n");
			snd_pcm_status_dump(alsa_status, logs);
		}
	}
	return snd_pcm_prepare(alsa_pcm);
}

static int suspend_recover(void)
{
	int err;

	while ((err = snd_pcm_resume(alsa_pcm)) == -EAGAIN)
		/* wait until suspend flag is released */
		sleep(1);
	if (err < 0)
	{
		g_warning("alsa_handle_error(): "
			  "snd_pcm_resume() failed.");
		return snd_pcm_prepare(alsa_pcm);
	}
	return err;
}

/* handle generic errors */
static int alsa_handle_error(int err)
{
	switch (err)
	{
		case -EPIPE:
			return xrun_recover();
		case -ESTRPIPE:
			return suspend_recover();
	}

	return err;
}

/* update and get the available space on h/w buffer (in frames) */
static snd_pcm_sframes_t alsa_get_avail(void)
{
	snd_pcm_sframes_t ret;

	if (alsa_pcm == NULL)
		return 0;

	while ((ret = snd_pcm_avail_update(alsa_pcm)) < 0)
	{
		ret = alsa_handle_error(ret);
		if (ret < 0)
		{
			g_warning("alsa_get_avail(): snd_pcm_avail_update() failed: %s",
				  snd_strerror(-ret));
			return 0;
		}
	}
	return ret;
}

/* get the free space on buffer */
int alsa_free(void)
{
	if (remove_prebuffer && prebuffer)
	{
		prebuffer = FALSE;
		remove_prebuffer = FALSE;
	}
	if (prebuffer)
		remove_prebuffer = TRUE;
	
	return thread_buffer_size - get_thread_buffer_filled() - 1;
}

/* do pause operation */
static void alsa_do_pause(gboolean p)
{
	if (paused == p)
		return;

	if (alsa_pcm)
	{
		if (alsa_can_pause)
			snd_pcm_pause(alsa_pcm, p);
		else if (p)
		{
			snd_pcm_drop(alsa_pcm);
			snd_pcm_prepare(alsa_pcm);
		}
	}
	paused = p;
}

void alsa_pause(short p)
{
	debug("alsa_pause");
	pause_request = p;
}

/* close PCM and release associated resources */
static void alsa_close_pcm(void)
{
	if (alsa_pcm)
	{
		int err;
		snd_pcm_drop(alsa_pcm);
		if ((err = snd_pcm_close(alsa_pcm)) < 0)
			g_warning("alsa_pcm_close() failed: %s",
				  snd_strerror(-err));
		alsa_pcm = NULL;
	}
}

void alsa_close(void)
{
	if (!going)
		return;

	debug("Closing device");

	going = 0;

	g_thread_join(audio_thread);

	alsa_cleanup_mixer();

	xmms_convert_buffers_destroy(convertb);
	convertb = NULL;
	g_free(inputf);
	inputf = NULL;
	g_free(effectf);
	effectf = NULL;
	g_free(outputf);
	outputf = NULL;

	alsa_save_config();

	if (alsa_cfg.debug)
		snd_output_close(logs);
	debug("Device closed");
}

/* reopen ALSA PCM */
static int alsa_reopen(struct snd_format *f)
{
	/* remember the current position */
	output_time_offset += (alsa_hw_written * 1000) / outputf->bps;
	alsa_hw_written = 0;

	alsa_close_pcm();

	return alsa_setup(f);
}

/* do flush (drop) operation */
static void alsa_do_flush(int time)
{
	if (alsa_pcm)
	{
		snd_pcm_drop(alsa_pcm);
		snd_pcm_prepare(alsa_pcm);
	}
	/* correct the offset */
	output_time_offset = time;
	alsa_total_written = (guint64) time * inputf->bps / 1000;
	rd_index = wr_index = alsa_hw_written = 0;
}

void alsa_flush(int time)
{
	flush_request = time;
	while (flush_request != -1)
		xmms_usleep(10000);
}

static void parse_mixer_name(char *str, char **name, int *index)
{
	char *end;

	while (isspace(*str))
		str++;

	if ((end = strchr(str, ',')) != NULL)
	{
		*name = g_strndup(str, end - str);
		end++;
		*index = atoi(end);
	}
	else
	{
		*name = g_strdup(str);
		*index = 0;
	}
}

int alsa_get_mixer(snd_mixer_t **mixer, int card)
{
	char *dev;
	int err;

	debug("alsa_get_mixer");

	dev = g_strdup_printf("hw:%i", card);

	if ((err = snd_mixer_open(mixer, 0)) < 0)
	{
		g_warning("alsa_get_mixer(): Failed to open empty mixer: %s",
			  snd_strerror(-err));
		mixer = NULL;
		return -1;
	}
	if ((err = snd_mixer_attach(*mixer, dev)) < 0)
	{
		g_warning("alsa_get_mixer(): Attaching to mixer %s failed: %s",
			  dev, snd_strerror(-err));
		return -1;
	}
	if ((err = snd_mixer_selem_register(*mixer, NULL, NULL)) < 0)
	{
		g_warning("alsa_get_mixer(): Failed to register mixer: %s",
			  snd_strerror(-err));
		return -1;
	}
	if ((err = snd_mixer_load(*mixer)) < 0)
	{
		g_warning("alsa_get_mixer(): Failed to load mixer: %s",
			  snd_strerror(-err));
		return -1;
	}

	g_free(dev);

	return (*mixer != NULL);
}


static snd_mixer_elem_t* alsa_get_mixer_elem(snd_mixer_t *mixer, char *name, int index)
{
	snd_mixer_selem_id_t *selem_id;
	snd_mixer_elem_t* elem;
	snd_mixer_selem_id_alloca(&selem_id);

	if (index != -1)
		snd_mixer_selem_id_set_index(selem_id, index);
	if (name != NULL)
		snd_mixer_selem_id_set_name(selem_id, name);

	elem = snd_mixer_find_selem(mixer, selem_id);

	return elem;
}

static int alsa_setup_mixer(void)
{
	char *name;
	long int a, b;
	long alsa_min_vol, alsa_max_vol;
	int err, index;

	debug("alsa_setup_mixer");

	if ((err = alsa_get_mixer(&mixer, alsa_cfg.mixer_card)) < 0)
		return err;

	parse_mixer_name(alsa_cfg.mixer_device, &name, &index);

	pcm_element = alsa_get_mixer_elem(mixer, name, index);

	g_free(name);

	if (!pcm_element)
	{
		g_warning("alsa_setup_mixer(): Failed to find mixer element: %s",
			  alsa_cfg.mixer_device);
		return -1;
	}

	/*
	 * Work around a bug in alsa-lib up to 1.0.0rc2 where the
	 * new range don't take effect until the volume is changed.
	 * This hack should be removed once we depend on Alsa 1.0.0.
	 */
	snd_mixer_selem_get_playback_volume(pcm_element,
					    SND_MIXER_SCHN_FRONT_LEFT, &a);
	snd_mixer_selem_get_playback_volume(pcm_element,
					    SND_MIXER_SCHN_FRONT_RIGHT, &b);

	snd_mixer_selem_get_playback_volume_range(pcm_element,
						  &alsa_min_vol, &alsa_max_vol);
	snd_mixer_selem_set_playback_volume_range(pcm_element, 0, 100);

	if (alsa_max_vol == 0)
	{
		pcm_element = NULL;
		return -1;
	}

	if (!alsa_cfg.soft_volume)
		alsa_set_volume(a * 100 / alsa_max_vol, b * 100 / alsa_max_vol);

	debug("alsa_setup_mixer: end");

	return 0;
}

static int alsa_mixer_timeout(void *data)
{
	if (mixer)
	{
		snd_mixer_close(mixer);
		mixer = NULL;
		pcm_element = NULL;
	}
	mixer_timeout = 0;
	mixer_start = TRUE;

	g_message("alsa mixer timed out");
	return FALSE;
}

static void alsa_cleanup_mixer(void)
{
	pcm_element = NULL;
	if (mixer)
	{
		snd_mixer_close(mixer);
		mixer = NULL;
	}
}

void alsa_get_volume(int *l, int *r)
{
	long ll = *l, lr = *r;

	if (mixer_start)
	{
		alsa_setup_mixer();
		mixer_start = FALSE;
	}

	if (alsa_cfg.soft_volume)
	{
		*l = alsa_cfg.vol.left;
		*r = alsa_cfg.vol.right;
	}

	if (!pcm_element)
		return;

	snd_mixer_handle_events(mixer);

	if (!alsa_cfg.soft_volume)
	{
		snd_mixer_selem_get_playback_volume(pcm_element,
						    SND_MIXER_SCHN_FRONT_LEFT,
						    &ll);
		snd_mixer_selem_get_playback_volume(pcm_element,
						    SND_MIXER_SCHN_FRONT_RIGHT,
						    &lr);
		*l = ll;
		*r = lr;
	}
	if (mixer_timeout)
		gtk_timeout_remove(mixer_timeout);
	mixer_timeout = gtk_timeout_add(5000, alsa_mixer_timeout, NULL);
}


void alsa_set_volume(int l, int r)
{
	if (alsa_cfg.soft_volume)
	{
		alsa_cfg.vol.left = l;
		alsa_cfg.vol.right = r;
		return;
	}

	if (!pcm_element)
		return;

	snd_mixer_selem_set_playback_volume(pcm_element,
					    SND_MIXER_SCHN_FRONT_LEFT, l);
	snd_mixer_selem_set_playback_volume(pcm_element,
					    SND_MIXER_SCHN_FRONT_RIGHT, r);
}


/*
 * audio stuff
 */

/* return the size of audio data filled in the audio thread buffer */
static int get_thread_buffer_filled(void)
{
	if (wr_index >= rd_index)
		return wr_index - rd_index;
	return thread_buffer_size - (rd_index - wr_index);
}

int alsa_get_output_time(void)
{
	snd_pcm_sframes_t delay;
	guint64 bytes = alsa_hw_written;

	if (!going || alsa_pcm == NULL)
		return 0;

	if (!snd_pcm_delay(alsa_pcm, &delay))
	{
		unsigned int d = snd_pcm_frames_to_bytes(alsa_pcm, delay);
		if (bytes < d)
			bytes = 0;
		else
			bytes -= d;
	}
	return output_time_offset + (bytes * 1000) / outputf->bps;
}

int alsa_get_written_time(void)
{
	if (!going)
		return 0;
	return (alsa_total_written * 1000) / inputf->bps;
}

#define STEREO_ADJUST(type, type2, endian)					\
do {										\
	type *ptr = data;							\
	for (i = 0; i < length; i += 4)						\
	{									\
		*ptr = type2##_TO_##endian(type2##_FROM_## endian(*ptr) *	\
					   alsa_cfg.vol.left / 100);		\
		ptr++;								\
		*ptr = type2##_TO_##endian(type2##_FROM_##endian(*ptr) *	\
					   alsa_cfg.vol.right / 100);		\
		ptr++;								\
	}									\
} while (0)

#define MONO_ADJUST(type, type2, endian)					\
do {										\
	type *ptr = data;							\
	for (i = 0; i < length; i += 2)						\
	{									\
		*ptr = type2##_TO_##endian(type2##_FROM_## endian(*ptr) *	\
					   vol / 100);				\
		ptr++;								\
	}									\
} while (0)

#define VOLUME_ADJUST(type, type2, endian)		\
do {							\
	if (channels == 2)				\
		STEREO_ADJUST(type, type2, endian);	\
	else						\
		MONO_ADJUST(type, type2, endian);	\
} while (0)

#define STEREO_ADJUST8(type)				\
do {							\
	type *ptr = data;				\
	for (i = 0; i < length; i += 2)			\
	{						\
		*ptr = *ptr * alsa_cfg.vol.left / 100;	\
		ptr++;					\
		*ptr = *ptr * alsa_cfg.vol.right / 100;	\
		ptr++;					\
	}						\
} while (0)

#define MONO_ADJUST8(type)			\
do {						\
	type *ptr = data;			\
	for (i = 0; i < length; i++)		\
	{					\
		*ptr = *ptr * vol / 100;	\
		ptr++;				\
	}					\
} while (0)

#define VOLUME_ADJUST8(type)			\
do {						\
	if (channels == 2)			\
		STEREO_ADJUST8(type);		\
	else					\
		MONO_ADJUST8(type);		\
} while (0)


static void volume_adjust(void* data, int length, AFormat fmt, int channels)
{
	int i, vol;

	if ((alsa_cfg.vol.left == 100 && alsa_cfg.vol.right == 100) ||
	    (channels == 1 &&
	     (alsa_cfg.vol.left == 100 || alsa_cfg.vol.right == 100)))
		return;

	vol = MAX(alsa_cfg.vol.left, alsa_cfg.vol.right);

	switch (fmt)
	{
		case FMT_S16_LE:
			VOLUME_ADJUST(gint16, GINT16, LE);
			break;
		case FMT_U16_LE:
			VOLUME_ADJUST(guint16, GUINT16, LE);
			break;
		case FMT_S16_BE:
			VOLUME_ADJUST(gint16, GINT16, BE);
			break;
		case FMT_U16_BE:
			VOLUME_ADJUST(guint16, GUINT16, BE);
			break;
		case FMT_S8:
			VOLUME_ADJUST8(gint8);
			break;
		case FMT_U8:
			VOLUME_ADJUST8(guint8);
			break;
		default:
			g_warning("volue_adjust(): unhandled format: %d", fmt);
			break;
	}
}


/* transfer data to audio h/w; length is given in bytes
 *
 * data can be modified via effect plugin, rate conversion or
 * software volume before passed to audio h/w
 */
static void alsa_do_write(gpointer data, int length)
{
	EffectPlugin *ep = NULL;
	int new_freq;
	int new_chn;
	AFormat f;

	if (paused)
		return;

	new_freq = inputf->rate;
	new_chn = inputf->channels;
	f = inputf->xmms_format;

	if (effects_enabled() && (ep = get_current_effect_plugin()) &&
	    ep->query_format)
		ep->query_format(&f, &new_freq, &new_chn);

	if (f != effectf->xmms_format || new_freq != effectf->rate ||
	    new_chn != effectf->channels)
	{
		debug("Changing audio format for effect plugin");
		g_free(effectf);
		effectf = snd_format_from_xmms(f, new_freq, new_chn);
		if (alsa_reopen(effectf) < 0)
		{
			/* fatal error... */
			alsa_close();
			return;
		}
	}

	if (ep)
		length = ep->mod_samples(&data, length,
					 inputf->xmms_format,
					 inputf->rate,
					 inputf->channels);

	if (alsa_convert_func != NULL)
		length = alsa_convert_func(convertb, &data, length);
	if (alsa_stereo_convert_func != NULL)
		length = alsa_stereo_convert_func(convertb, &data, length);
	if (alsa_frequency_convert_func != NULL)
		length = alsa_frequency_convert_func(convertb, &data, length,
						     effectf->rate,
						     outputf->rate);

	if (alsa_cfg.soft_volume)
		volume_adjust(data, length, outputf->xmms_format, outputf->channels);

	alsa_write_audio(data, length);
}

/* write callback */
void alsa_write(gpointer data, int length)
{
	int cnt;
	char *src = (char *)data;
	
	remove_prebuffer = FALSE;
	
	alsa_total_written += length;
	while (length > 0)
	{
		int wr;
		cnt = MIN(length, thread_buffer_size - wr_index);
		memcpy(thread_buffer + wr_index, src, cnt);
		wr = (wr_index + cnt) % thread_buffer_size;
		wr_index = wr;
		length -= cnt;
		src += cnt;
	}
}

/* transfer data to audio h/w via normal write */
static void alsa_write_audio(char *data, int length)
{
	snd_pcm_sframes_t written_frames;

	while (length > 0)
	{
		int frames = snd_pcm_bytes_to_frames(alsa_pcm, length);
		written_frames = snd_pcm_writei(alsa_pcm, data, frames);

		if (written_frames > 0)
		{
			int written = snd_pcm_frames_to_bytes(alsa_pcm,
							      written_frames);
			length -= written;
			data += written;
			alsa_hw_written += written;
		}
		else
		{
			int err = alsa_handle_error((int)written_frames);
			if (err < 0)
			{
				g_warning("alsa_write_audio(): write error: %s",
					  snd_strerror(-err));
				break;
			}
		}
	}
}

/* transfer audio data from thread buffer to h/w */
static void alsa_write_out_thread_data(void)
{
	gint length, cnt, avail;

	length = MIN(hw_period_size_in, get_thread_buffer_filled());
	avail = snd_pcm_frames_to_bytes(alsa_pcm, alsa_get_avail());
	length = MIN(length, avail);
	while (length > 0)
	{
		int rd;
		cnt = MIN(length, thread_buffer_size - rd_index);
		alsa_do_write(thread_buffer + rd_index, cnt);
		rd = (rd_index + cnt) % thread_buffer_size;
		rd_index = rd;
		length -= cnt;
	}
}

/* audio thread loop */
/* FIXME: proper lock? */
static void *alsa_loop(void *arg)
{
	int npfds = snd_pcm_poll_descriptors_count(alsa_pcm);
	struct pollfd *pfds;
	unsigned short *revents;

	if (npfds <= 0)
		goto _error;
	pfds = alloca(sizeof(*pfds) * npfds);
	revents = alloca(sizeof(*revents) * npfds);
	while (going && alsa_pcm)
	{
		if (get_thread_buffer_filled() > prebuffer_size)
			prebuffer = FALSE;
		if (!paused && !prebuffer &&
		    get_thread_buffer_filled() > hw_period_size_in)
		{
			snd_pcm_poll_descriptors(alsa_pcm, pfds, npfds);
			if (poll(pfds, npfds, 10) > 0)
			{
				/*
				 * need to check revents.  poll() with
				 * dmix returns a postive value even
				 * if no data is available
				 */
				int i;
				snd_pcm_poll_descriptors_revents(alsa_pcm, pfds,
								 npfds, revents);
				for (i = 0; i < npfds; i++)
					if (revents[i] & POLLOUT)
					{
						alsa_write_out_thread_data();
						break;
					}
			}
		}
		else
			xmms_usleep(10000);

		if (pause_request != paused)
			alsa_do_pause(pause_request);

		if (flush_request != -1)
		{
			alsa_do_flush(flush_request);
			flush_request = -1;
			prebuffer = TRUE;
		}
	}

 _error:
	alsa_close_pcm();
	g_free(thread_buffer);
	thread_buffer = NULL;
	g_thread_exit(NULL);
	return(NULL);
}

/* open callback */
int alsa_open(AFormat fmt, int rate, int nch)
{
	debug("Opening device");
	inputf = snd_format_from_xmms(fmt, rate, nch);
	effectf = snd_format_from_xmms(fmt, rate, nch);

	if (alsa_cfg.debug)
		snd_output_stdio_attach(&logs, stdout, 0);

	if (alsa_setup(inputf) < 0)
	{
		alsa_close();
		return 0;
	}

	if (!mixer)
		alsa_setup_mixer();

	convertb = xmms_convert_buffers_new();

	output_time_offset = 0;
	alsa_total_written = alsa_hw_written = 0;
	going = TRUE;
	paused = FALSE;
	prebuffer = TRUE;
	remove_prebuffer = FALSE;

	thread_buffer_size = (guint64)alsa_cfg.thread_buffer_time * inputf->bps / 1000;
	if (thread_buffer_size < hw_buffer_size)
		thread_buffer_size = hw_buffer_size * 2;
	if (thread_buffer_size < 8192)
		thread_buffer_size = 8192;
	prebuffer_size = thread_buffer_size / 2;
	if (prebuffer_size < 8192)
		prebuffer_size = 8192;
	thread_buffer_size += hw_buffer_size;
	thread_buffer_size -= thread_buffer_size % hw_period_size;
	thread_buffer = g_malloc0(thread_buffer_size);
	wr_index = rd_index = 0;
	pause_request = FALSE;
	flush_request = -1;
	
	audio_thread = g_thread_create((GThreadFunc)alsa_loop, NULL, TRUE, NULL);
	return 1;
}

static struct snd_format * snd_format_from_xmms(AFormat fmt, int rate, int channels)
{
	struct snd_format *f = g_malloc(sizeof(struct snd_format));
	int i;

	f->xmms_format = fmt;
	f->format = SND_PCM_FORMAT_UNKNOWN;

	for (i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++)
		if (format_table[i].xmms == fmt)
		{
			f->format = format_table[i].alsa;
			break;
		}

	/* Get rid of _NE */
	for (i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++)
		if (format_table[i].alsa == f->format)
		{
			f->xmms_format = format_table[i].xmms;
			break;
		}


	f->rate = rate;
	f->channels = channels;
	f->sample_bits = snd_pcm_format_physical_width(f->format);
	f->bps = (rate * f->sample_bits * channels) >> 3;

	return f;
}

static int format_from_alsa(snd_pcm_format_t fmt)
{
	int i;
	for (i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++)
		if (format_table[i].alsa == fmt)
			return format_table[i].xmms;
	g_warning("Unsupported format: %s", snd_pcm_format_name(fmt));
	return -1;
}

static int alsa_setup(struct snd_format *f)
{
	int err;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	int alsa_buffer_time;
	unsigned int alsa_period_time;
	snd_pcm_uframes_t alsa_buffer_size, alsa_period_size;

	debug("alsa_setup");

	alsa_convert_func = NULL;
	alsa_stereo_convert_func = NULL;
	alsa_frequency_convert_func = NULL;

	g_free(outputf);
	outputf = snd_format_from_xmms(f->xmms_format, f->rate, f->channels);

	debug("Opening device: %s", alsa_cfg.pcm_device);
	/* FIXME: Can snd_pcm_open() return EAGAIN? */
	if ((err = snd_pcm_open(&alsa_pcm, alsa_cfg.pcm_device,
				SND_PCM_STREAM_PLAYBACK,
				SND_PCM_NONBLOCK)) < 0)
	{
		g_warning("alsa_setup(): Failed to open pcm device (%s): %s",
			  alsa_cfg.pcm_device, snd_strerror(-err));
		alsa_pcm = NULL;
		g_free(outputf);
		outputf = NULL;
		return -1;
	}

	/* doesn't care about non-blocking */
	/* snd_pcm_nonblock(alsa_pcm, 0); */

	if (alsa_cfg.debug)
	{
		snd_pcm_info_t *info;
		int alsa_card, alsa_device, alsa_subdevice;

		snd_pcm_info_alloca(&info);
		snd_pcm_info(alsa_pcm, info);
		alsa_card =  snd_pcm_info_get_card(info);
		alsa_device = snd_pcm_info_get_device(info);
		alsa_subdevice = snd_pcm_info_get_subdevice(info);
		printf("Card %i, Device %i, Subdevice %i\n",
		       alsa_card, alsa_device, alsa_subdevice);
	}

	snd_pcm_hw_params_alloca(&hwparams);

	if ((err = snd_pcm_hw_params_any(alsa_pcm, hwparams)) < 0)
	{
		g_warning("alsa_setup(): No configuration available for "
			  "playback: %s", snd_strerror(-err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_access(alsa_pcm, hwparams,
						SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		g_warning("alsa_setup(): Cannot set direct write mode: %s",
			  snd_strerror(-err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_format(alsa_pcm, hwparams, outputf->format)) < 0)
	{
		/*
		 * Try if one of these format work (one of them should work
		 * on almost all soundcards)
		 */
		snd_pcm_format_t formats[] = {SND_PCM_FORMAT_S16_LE,
					      SND_PCM_FORMAT_S16_BE,
					      SND_PCM_FORMAT_U8};
		int i;

		for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
		{
			if (snd_pcm_hw_params_set_format(alsa_pcm, hwparams,
							 formats[i]) == 0)
			{
				outputf->format = formats[i];
				break;
			}
		}
		if (outputf->format != f->format)
		{
			outputf->xmms_format =
				format_from_alsa(outputf->format);
			debug("Converting format from %d to %d",
			      f->xmms_format, outputf->xmms_format);
			if (outputf->xmms_format < 0)
				return -1;
			alsa_convert_func =
				xmms_convert_get_func(outputf->xmms_format,
						      f->xmms_format);
			if (alsa_convert_func == NULL)
				return -1;
		}
		else
		{
			g_warning("alsa_setup(): Sample format not "
				  "available for playback: %s",
				  snd_strerror(-err));
			return -1;
		}
	}

	snd_pcm_hw_params_set_channels_near(alsa_pcm, hwparams, &outputf->channels);
	if (outputf->channels != f->channels)
	{
		debug("Converting channels from %d to %d",
		      f->channels, outputf->channels);
		alsa_stereo_convert_func =
			xmms_convert_get_channel_func(outputf->xmms_format,
						      outputf->channels,
						      f->channels);
		if (alsa_stereo_convert_func == NULL)
			return -1;
	}

	snd_pcm_hw_params_set_rate_near(alsa_pcm, hwparams, &outputf->rate, 0);
	if (outputf->rate == 0)
	{
		g_warning("alsa_setup(): No usable samplerate available.");
		return -1;
	}
	if (outputf->rate != f->rate)
	{
		debug("Converting samplerate from %d to %d",
		      f->rate, outputf->rate);
		alsa_frequency_convert_func =
			xmms_convert_get_frequency_func(outputf->xmms_format,
							outputf->channels);
		if (alsa_frequency_convert_func == NULL)
			return -1;
	}

	outputf->sample_bits = snd_pcm_format_physical_width(outputf->format);
	outputf->bps = (outputf->rate * outputf->sample_bits * outputf->channels) >> 3;

	alsa_buffer_time = alsa_cfg.buffer_time * 1000;
	if ((err = snd_pcm_hw_params_set_buffer_time_near(alsa_pcm, hwparams,
							  &alsa_buffer_time, 0)) < 0)
	{
		g_warning("alsa_setup(): Set buffer time failed: %s.",
			  snd_strerror(-err));
		return -1;
	}

	alsa_period_time = alsa_cfg.period_time * 1000;
	if ((err = snd_pcm_hw_params_set_period_time_near(alsa_pcm, hwparams,
							  &alsa_period_time, 0)) < 0)
	{
		g_warning("alsa_setup(): Set period time failed: %s.",
			  snd_strerror(-err));
		return -1;
	}

	if (snd_pcm_hw_params(alsa_pcm, hwparams) < 0)
	{
		if (alsa_cfg.debug)
			snd_pcm_hw_params_dump(hwparams, logs);
		g_warning("alsa_setup(): Unable to install hw params");
		return -1;
	}

	if ((err = snd_pcm_hw_params_get_buffer_size(hwparams, &alsa_buffer_size)) < 0)
	{
		g_warning("alsa_setup(): snd_pcm_hw_params_get_buffer_size() "
			  "failed: %s",
			  snd_strerror(-err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_get_period_size(hwparams, &alsa_period_size, 0)) < 0)
	{
		g_warning("alsa_setup(): snd_pcm_hw_params_get_period_size() "
			  "failed: %s",
			  snd_strerror(-err));
		return -1;
	}

	alsa_can_pause = snd_pcm_hw_params_can_pause(hwparams);

	snd_pcm_sw_params_alloca(&swparams);
	snd_pcm_sw_params_current(alsa_pcm, swparams);

	/* This has effect for non-mmap only */
	if ((err = snd_pcm_sw_params_set_start_threshold(alsa_pcm,
			swparams, alsa_buffer_size - alsa_period_size) < 0))
		g_warning("alsa_setup(): setting start "
			  "threshold failed: %s", snd_strerror(-err));
	if (snd_pcm_sw_params(alsa_pcm, swparams) < 0)
	{
		g_warning("alsa_setup(): Unable to install sw params");
		return -1;
	}

	if (alsa_cfg.debug)
	{
		snd_pcm_sw_params_dump(swparams, logs);
		snd_pcm_dump(alsa_pcm, logs);
	}

	hw_buffer_size = snd_pcm_frames_to_bytes(alsa_pcm, alsa_buffer_size);
	hw_period_size = snd_pcm_frames_to_bytes(alsa_pcm, alsa_period_size);
	if (inputf->bps != outputf->bps)
	{
		int align = (inputf->sample_bits * inputf->channels) / 8;
		hw_buffer_size_in = ((guint64)hw_buffer_size * inputf->bps +
				     outputf->bps/2) / outputf->bps;
		hw_period_size_in = ((guint64)hw_period_size * inputf->bps +
				     outputf->bps/2) / outputf->bps;
		hw_buffer_size_in -= hw_buffer_size_in % align;
		hw_period_size_in -= hw_period_size_in % align;
	}
	else
	{
		hw_buffer_size_in = hw_buffer_size;
		hw_period_size_in = hw_period_size;
	}

	debug("Device setup: buffer time: %i, size: %i.", alsa_buffer_time,
	      hw_buffer_size);
	debug("Device setup: period time: %i, size: %i.", alsa_period_time,
	      hw_period_size);
	debug("bits per sample: %i; frame size: %i; Bps: %i",
	      snd_pcm_format_physical_width(outputf->format),
	      snd_pcm_frames_to_bytes(alsa_pcm, 1), outputf->bps);

	return 0;
}
