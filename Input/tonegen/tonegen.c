/*
 *  Copyright 2000,2001 Haavard Kvaalen <havardk@sol.no>
 *
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

#include "audacious/plugin.h"
#include "libaudacious/util.h"
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MIN_FREQ 10
#define MAX_FREQ 20000
#define OUTPUT_FREQ 44100

#ifndef PI
#define PI 3.14159265358979323846
#endif

static InputPlugin tone_ip;

static gboolean going;
static gboolean audio_error;
static pthread_t play_thread;

static void tone_about(void)
{
	static GtkWidget *box;
	box = xmms_show_message(
		_("About Tone Generator"),
/* I18N: UTF-8 Translation: "Haavard Kvaalen" -> "H\303\245vard Kv\303\245len" */
		_("Sinus tone generator by Haavard Kvaalen <havardk@xmms.org>\n"
		  "Modified by Daniel J. Peng <danielpeng@bigfoot.com>\n\n"
		  "To use it, add a URL: tone://frequency1;frequency2;frequency3;...\n"
		  "e.g. tone://2000;2005 to play a 2000Hz tone and a 2005Hz tone"),
		_("Ok"), FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(box), "destroy",
			   gtk_widget_destroyed, &box);
}

static int tone_is_our_file(char *filename)
{
	if (!strncmp(filename, "tone://", 7))
		return TRUE;
	return FALSE;
}

#define BUF_SAMPLES 512
#define BUF_BYTES BUF_SAMPLES * 2

static void* play_loop(void *arg)
{
	GArray* frequencies = arg;
	gint16 data[BUF_SAMPLES];
	int i;
	struct {
		double wd;
		unsigned int period, t;
	} *tone;

	tone = g_malloc(frequencies->len * sizeof(*tone));

	for (i = 0; i < frequencies->len; i++)
	{
		double f = g_array_index(frequencies, double, i);
		tone[i].wd = 2 * PI * f / OUTPUT_FREQ;
		tone[i].period = (G_MAXINT * 2U / OUTPUT_FREQ) *
							(OUTPUT_FREQ / f);
		tone[i].t = 0;
	}

	while (going)
	{
		for (i = 0; i < BUF_SAMPLES; i++)
		{
			int j;
			double sum_sines;

			for (sum_sines = 0, j = 0; j < frequencies->len; j++)
			{
				sum_sines += sin(tone[j].wd * tone[j].t);
				if (tone[j].t > tone[j].period)
					tone[j].t -= tone[j].period;
				tone[j].t++;
			}
			data[i] = rint(((1 << 15) - 1) *
				       (sum_sines / frequencies->len));
		}
		tone_ip.add_vis_pcm(tone_ip.output->written_time(),
				    FMT_S16_NE, 1, BUF_BYTES, data);
		while (tone_ip.output->buffer_free() < BUF_BYTES && going)
			xmms_usleep(30000);
		if (going)
			tone_ip.output->write_audio(data, BUF_BYTES);
	}

	g_array_free(frequencies, TRUE);
	g_free(tone);

	/* Make sure the output plugin stops prebuffering */
	tone_ip.output->buffer_free();
	tone_ip.output->buffer_free();

	pthread_exit(NULL);
}

static GArray* tone_filename_parse(const char* filename)
{
	GArray *frequencies = g_array_new(FALSE, FALSE, sizeof(double));
	char **strings, **ptr;

	if (strncmp(filename,"tone://", 7))
		return NULL;

	filename += 7;
	strings = g_strsplit(filename, ";", 100);

	for (ptr = strings; *ptr != NULL; ptr++)
	{
		double freq = strtod(*ptr, NULL);
		if (freq >= MIN_FREQ && freq <= MAX_FREQ)
			g_array_append_val(frequencies, freq);
	}
	g_strfreev(strings);

	if (frequencies->len == 0)
	{
		g_array_free(frequencies, TRUE);
		frequencies = NULL;
	}

	return frequencies;
}

static char* tone_title(char *filename)
{
	GArray *freqs;
	char* title;
	int i;

	freqs = tone_filename_parse(filename);
	if (freqs == NULL)
		return NULL;

	title = g_strdup_printf("%s %.1f Hz", _("Tone Generator: "),
				g_array_index(freqs, double, 0));
	for (i = 1; i < freqs->len; i++)
	{
		char *old_title;
		old_title = title;
		title = g_strdup_printf("%s;%.1f Hz", old_title,
					g_array_index(freqs, double, i));
		g_free(old_title);
	}
	g_array_free(freqs, TRUE);

	return title;
}
	

static void tone_play(char *filename)
{
	GArray* frequencies;
	char *name;

	frequencies = tone_filename_parse(filename);
	if (frequencies == NULL)
		return;

 	going = TRUE;
	audio_error = FALSE;
	if (tone_ip.output->open_audio(FMT_S16_NE, OUTPUT_FREQ, 1) == 0)
	{
		audio_error = TRUE;
		going = FALSE;
		return;
	}

	name = tone_title(filename);
	tone_ip.set_info(name, -1, 16 * OUTPUT_FREQ, OUTPUT_FREQ, 1);
	g_free(name);
	pthread_create(&play_thread, NULL, play_loop, frequencies);
}

static void tone_stop(void)
{
	if (going)
	{
		going = FALSE;
		pthread_join(play_thread, NULL);
		tone_ip.output->close_audio();
	}
}

static void tone_pause(short paused)
{
	tone_ip.output->pause(paused);
}

static int tone_get_time(void)
{
	if (audio_error)
		return -2;
	if (!going && !tone_ip.output->buffer_playing())
		return -1;
	return tone_ip.output->output_time();
}

static void tone_song_info(char *filename, char **title, int *length)
{
	*length = -1;
	*title = tone_title(filename);
}

static InputPlugin tone_ip = 
{
	NULL,
	NULL,
	NULL, /* Description */
	NULL,
	tone_about,
	NULL,
	tone_is_our_file,
	NULL,
	tone_play,
	tone_stop,
	tone_pause,
	NULL,
	NULL,
	tone_get_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	tone_song_info,
	NULL,
	NULL
};

InputPlugin *get_iplugin_info(void)
{
	tone_ip.description = g_strdup_printf(_("Tone Generator %s"), VERSION);
	return &tone_ip;
}
