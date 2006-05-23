/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2005  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2005  Haavard Kvaalen
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
 * 2004-12-21 Dirk Jagdmann <doj@cubic.org>
 * - added 32bit pcm,float support
 *
 * 2004-12-22 Dirk Jagdmann <doj@cubic.org>
 * - added 12bit pcm support
 * - added 24bit pcm support
 * - added 64bit float support
 * - fixed big endian
 *
 * TODO:
 * - dither to 16 bits
 */

#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"
#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

static InputPlugin wav_ip;

#if 0
#define WAVDEBUGf
#define WAVDEBUG(f, a...) printf(f, ##a)
#else
#define WAVDEBUG(f, a...)
#endif

#define	WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define	WAVE_FORMAT_ALAW       0x0006
#define	WAVE_FORMAT_MULAW      0x0007
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

struct wave_file
{
	FILE *file;
	unsigned short format_tag;
	guint16 channels, block_align, bits_per_sample, bits_per_sample_out;
	guint32 samples_per_sec, avg_bytes_per_sec;
	unsigned long position, length;
	int seek_to, data_offset;
	pid_t pid;
	volatile gboolean going, eof;
	int afmt;
	gint16 *table;
};

static struct wave_file *wav_file = NULL;
static pthread_t decode_thread;
static gboolean audio_error = FALSE;

InputPlugin *get_iplugin_info(void)
{
	wav_ip.description = g_strdup_printf(_("Wave Player %s"), VERSION);
	return &wav_ip;
}

static gint16 *get_alaw_table(void)
{
	static gint16 *table;

	if (!table)
	{
		int i;
		table = g_malloc(128 * sizeof (gint16));
		for (i = 0; i < 128; i++)
		{
			int v = i ^ 0x55;
			int seg = (v >> 4) & 7;

			table[i] = (((v & 0x0f) << 4) + (seg ? 0x108 : 8)) <<
				(seg ? seg - 1 : 0);
		}
	}
	return table;
}

static gint16 *get_mulaw_table(void)
{
	static gint16 *table;

	if (!table)
	{
		int i;
		table = g_malloc(128 * sizeof (gint16));
		for (i = 0; i < 128; i++) {
			table[127 - i] = ((((i & 0x0f) << 3) + 0x84) <<
					  ((i >> 4) & 7)) - 0x84;
		}
	}
	return table;
}

static char *get_title(gchar *filename)
{
	TitleInput *input;
	char *temp, *ext, *title, *path, *temp2;

	XMMS_NEW_TITLEINPUT(input);

	path = g_strdup(filename);
	temp = g_strdup(filename);
	ext = strrchr(temp, '.');
	if (ext)
		*ext = '\0';
	temp2 = strrchr(path, '/');
	if (temp2)
		*temp2 = '\0';

	input->file_name = g_basename(filename);
	input->file_ext = ext ? ext+1 : NULL;
	input->file_path = g_strdup_printf("%s/", path);

	title = xmms_get_titlestring(xmms_get_gentitle_format(), input);
	if (title == NULL)
		title = g_strdup(input->file_name);

	g_free(temp);
	g_free(path);
	g_free(input);

	return title;
}

static float convert_ieee_float32(guint32 data)
{
	gint32 sign, exponent, mantissa;
	float value;

	sign = (data & 0x80000000) != 0;
	exponent = (data & 0x7f800000) >> 23;
	mantissa = data & 0x7fffff;

	if (exponent == 0xff)
		value = G_MAXFLOAT;
	else if (!exponent && !mantissa)
		value = 0.0;
	else
	{
		if (exponent)
		{
			exponent -= 127;
			mantissa |= 0x800000;
		}
		else
			exponent = -126;
		value = ldexp((float) mantissa / 0x800000, exponent);
	}
	if (sign)
		value *= -1;
	return value;
}

static double convert_ieee_float64(guint64 data)
{
	int sign, exponent;
	guint64 mantissa;
	double value;

	sign = (data & 0x8000000000000000ULL) != 0;
	exponent = (data & 0x7ff0000000000000ULL) >> 52;
	mantissa = data & 0xfffffffffffffLL;

	if (exponent == 0x7ff)
		value = G_MAXDOUBLE;
	else if (!exponent && !mantissa)
		value = 0.0;
	else
	{
		if (exponent)
		{
			exponent -= 1023;
			mantissa |= 0x10000000000000ULL;
		}
		else
			exponent = -1022;

		value = ldexp((double) mantissa / 0x10000000000000LL, exponent);
	}
	if (sign)
		value *= -1;
	return value;
}

static unsigned int convert(void *data, int in)
{
	int out = 0;

	if (wav_file->format_tag == WAVE_FORMAT_PCM)
	{
		switch (wav_file->bits_per_sample)
		{
			case 8:
			case 12:
			case 16:
			default:
				return in;

			case 24:
			{
				char *dst = data;
				char *src = data;

				while (in > 0)
				{
					out += 2;
					in -= 3;
					src++;		/* skip low byte */
					*dst++ = *src++;/* copy 16bit */
					*dst++ = *src++;
				}
			}
			break;

			case 32:
			{
				guint16 *dst = data;
				guint16 *src = data;
				int i;
				out = in / sizeof(guint16);
				for (i = 0; i < out; i++)
				{
					src++;		/* skip low word */
					*dst++ = *src++;/* copy 16 bit */
				}
			}
			break;
		}
	}
	else if (wav_file->format_tag == WAVE_FORMAT_IEEE_FLOAT)
	{
		if (wav_file->bits_per_sample == 32)
		{
			guint16 *dst = data;
			guint32 *src = data;
			int i;
			out = in / 2;
			for (i = 0; i < in / 4; i++)
			{
				float f;
				f = convert_ieee_float32(GUINT32_FROM_LE(*src));
				*dst++ = CLAMP(f, -1.0, 1.0) * 32767;
				src++;
			}
		}
		else if (wav_file->bits_per_sample == 64)
		{
			gint16 *dst = data;
			guint64 *src = data;
			int i;
			out = in / 4;
			for (i = 0; i < in / 8; i++)
			{
				double f;
				f = convert_ieee_float64(GUINT64_FROM_LE(*src));
				*dst++ = CLAMP(f, -1.0, 1.0) * 32767;
				src++;
			}
		}
	}
	else if (wav_file->format_tag == WAVE_FORMAT_ALAW ||
		 wav_file->format_tag == WAVE_FORMAT_MULAW)
	{
		int i;
		guint8 *src = data;
		guint16 *dst = data;
		for (i = in - 1; i >= 0; i--)
		{
			dst[i] = wav_file->table[src[i] & 0x7f];
			if (src[i] & 0x80)
				dst[i] *= -1;
		}
		out = in * 2;
	}
	return out;
}

static void *play_loop(void *arg)
{
	void *data;
	int input, output, blk_size, rate;
	int actual_read;

	blk_size = 512 * (wav_file->bits_per_sample / 8) * wav_file->channels;
	rate = wav_file->samples_per_sec * wav_file->channels *
		(wav_file->bits_per_sample / 8);
	data = g_malloc(512 * (MAX(wav_file->bits_per_sample,
				   wav_file->bits_per_sample_out) / 8) *
			wav_file->channels);
	while (wav_file->going)
	{
		if (!wav_file->eof)
		{
			input = MIN(blk_size, wav_file->length - wav_file->position);

			if (input > 0 &&
			    (actual_read = fread(data, 1, input, wav_file->file)) > 0)
			{
				output = convert(data, input);

				wav_ip.add_vis_pcm(wav_ip.output->written_time(),
						   wav_file->afmt, wav_file->channels,
						   output, data);
				while (wav_ip.output->buffer_free() < output &&
				       wav_file->going && wav_file->seek_to == -1)
					xmms_usleep(10000);
				if (wav_file->going && wav_file->seek_to == -1)
					wav_ip.output->write_audio(data, output);
				wav_file->position += actual_read;
			}
			else
			{
				wav_file->eof = TRUE;
				wav_ip.output->buffer_free();
				wav_ip.output->buffer_free();
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if (wav_file->seek_to != -1)
		{
			wav_file->position = wav_file->seek_to * rate;
			fseek(wav_file->file, wav_file->position +
			      wav_file->data_offset, SEEK_SET);
			wav_ip.output->flush(wav_file->seek_to * 1000);
			wav_file->seek_to = -1;
		}

	}
	g_free(data);
	fclose(wav_file->file);
	pthread_exit(NULL);
}

static int read_le_long(FILE * file, guint32 *ret)
{
	guint32 l;
	if (fread(&l, sizeof (l), 1, file) != 1)
		return 0;
	*ret = GUINT32_FROM_LE(l);
	return TRUE;
}

static int read_le_short(FILE * file, guint16 *ret)
{
	guint16 s;
	if (fread(&s, sizeof (s), 1, file) != 1)
		return 0;
	*ret = GUINT16_FROM_LE(s);
	return TRUE;
}

static struct wave_file* construct_wave_file(char *filename)
{
	guint32 len;
	char magic[4] = {0};

	/* alloc mem for struct */
	struct wave_file *wav = g_malloc0(sizeof (*wav));
	if (!wav)
		return wav;

	/* open file */
	wav->file = fopen(filename, "rb");
	if (!wav->file)
		goto exit_on_error;

	/* check for RIFF chunk */
	fread(magic, 1, 4, wav->file);
	if (strncmp(magic, "RIFF", 4) ||
	    !read_le_long(wav->file, &len))
		goto exit_on_error;

	fread(magic, 1, 4, wav->file);
	if (strncmp(magic, "WAVE", 4))
		goto exit_on_error;

	/* wait for fmt chunk */
	for (;;)
	{
		if (fread(magic, 1, 4, wav->file) != 4 ||
		    !read_le_long(wav->file, &len))
			goto exit_on_error;

		if (!strncmp("fmt ", magic, 4))
			break;
		if (fseek(wav->file, len, SEEK_CUR))
			goto exit_on_error;
	}

	/* check length of fmt chunk */
	if (len < 16)
		goto exit_on_error;

	/* read format */
	read_le_short(wav->file, &wav->format_tag);
	read_le_short(wav->file, &wav->channels);
	read_le_long (wav->file, &wav->samples_per_sec);
	read_le_long (wav->file, &wav->avg_bytes_per_sec);
	read_le_short(wav->file, &wav->block_align);
	read_le_short(wav->file, &wav->bits_per_sample);

	len -= 16;

	/* check for valid formats */
	if (wav->format_tag == WAVE_FORMAT_EXTENSIBLE)
	{
		if (len < 24 ||
		    fseek(wav->file, 8, SEEK_CUR) ||
		    !read_le_short(wav->file, &wav->format_tag))
			goto exit_on_error;
		len -= 10;
	}

	if (wav->format_tag == WAVE_FORMAT_PCM)
	{
		switch (wav->bits_per_sample)
		{
			case 8:
				wav->afmt = FMT_U8;
				break;
			case 12:
			case 16:
			case 24:
			case 32:
				wav->afmt = FMT_S16_LE;
				break;
			default:
				goto exit_on_error;
		}
	}
	else if (wav->format_tag == WAVE_FORMAT_IEEE_FLOAT)
	{
		if (wav->bits_per_sample == 32 ||
		    wav->bits_per_sample == 64)
			wav->afmt = FMT_S16_LE;
		else
			goto exit_on_error;
	}
	else if (wav->format_tag == WAVE_FORMAT_ALAW)
	{
		if (wav->bits_per_sample != 8)
			goto exit_on_error;
		wav->afmt = FMT_S16_LE;
		wav->bits_per_sample_out = 16;
		wav->table = get_alaw_table();
	}
	else if (wav->format_tag == WAVE_FORMAT_MULAW)
	{
		if (wav->bits_per_sample != 8)
			goto exit_on_error;
		wav->afmt = FMT_S16_LE;
		wav->bits_per_sample_out = 16;
		wav->table = get_mulaw_table();
	}
	else
		goto exit_on_error;

	if (wav->channels < 1 || wav->channels > 2)
		goto exit_on_error;

	/* skip any superflous bytes */
	if (len)
		fseek(wav->file, len, SEEK_CUR);

	/* wait for data chunk */
	for (;;)
	{
		fread(magic, 4, 1, wav->file);
		if (!read_le_long(wav->file, &len))
			goto exit_on_error;
		if (!strncmp("data", magic, 4))
			break;
		fseek(wav->file, len, SEEK_CUR);
	}
	wav->data_offset = ftell(wav->file);
	wav->length = len;

	wav->position = 0;
	wav->going = TRUE;

	return wav;

 exit_on_error:
	if (wav->file)
		fclose(wav->file);
	g_free(wav);
	return 0;
}

static void play_file(char *filename)
{
	char *name;
	int rate;

	audio_error = FALSE;

	if ((wav_file = construct_wave_file(filename)) == 0)
		return;

	/* open audio channel according to wav format */
	if (wav_ip.output->open_audio(wav_file->afmt,
				      wav_file->samples_per_sec,
				      wav_file->channels) == 0)
	{
		audio_error = TRUE;
		fclose(wav_file->file);
		g_free(wav_file);
		wav_file = NULL;
		return;
	}

	/* set some xmms config */
	name = get_title(filename);
	rate = wav_file->samples_per_sec * wav_file->channels *
		(wav_file->bits_per_sample / 8);
	wav_ip.set_info(name, 1000 * (wav_file->length / rate), 8 * rate,
			wav_file->samples_per_sec, wav_file->channels);
	g_free(name);
	wav_file->seek_to = -1;

	/* create play thread */
	pthread_create(&decode_thread, NULL, play_loop, NULL);
}

static void stop(void)
{
	if (wav_file && wav_file->going)
	{
		wav_file->going = FALSE;
		pthread_join(decode_thread, NULL);
		wav_ip.output->close_audio();
		g_free(wav_file);
		wav_file = NULL;
	}
}

static void wav_pause(short p)
{
	wav_ip.output->pause(p);
}

static void seek(int time)
{
	wav_file->seek_to = time;

	wav_file->eof = FALSE;

	while (wav_file->seek_to != -1)
		xmms_usleep(10000);
}

static int get_time(void)
{
	if (audio_error)
		return -2;
	if (!wav_file)
		return -1;
	if (!wav_file->going || (wav_file->eof && !wav_ip.output->buffer_playing()))
		return -1;
	else
	{
		return wav_ip.output->output_time();
	}
}

static void get_song_info(char *filename, char **title, int *length)
{
	struct wave_file *wav;

	if (!(wav = construct_wave_file(filename)))
		return;

	*length = 1000 * (wav->length / (wav->samples_per_sec * wav->channels *
					 (wav->bits_per_sample / 8)) );
	*title  = get_title(filename);

	fclose(wav->file);
	g_free(wav);
	wav = NULL;
}

static int is_our_file(char *filename)
{
	struct wave_file *wav;

	if (!(wav = construct_wave_file(filename)))
		return FALSE;

	g_free(wav);
	return TRUE;
}

static void plugin_init(void)
{
#ifdef WAVDEBUGf
	printf("wav.c compiled " __DATE__ " " __TIME__ " : ");
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	printf("little endian");
#elif G_BYTE_ORDER == G_BIG_ENDIAN
	printf("big endian");
#elif G_BYTE_ORDER == G_PDP_ENDIAN
	printf("pdp endian");
#else
	printf("unknown endian");
#endif
	printf("\n");
#endif
}

static InputPlugin wav_ip =
{
	NULL,
	NULL,
	NULL,
	plugin_init,
	NULL,
	NULL,
	is_our_file,
	NULL,
	play_file,
	stop,
	wav_pause,
	seek,
	NULL,
	get_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	get_song_info,
};
