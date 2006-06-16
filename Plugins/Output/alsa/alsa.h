/*  XMMS - ALSA output plugin
 *  Copyright (C) 2001-2003 Matthieu Sozeau
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Håvard Kvålen
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
#ifndef ALSA_H
#define ALSA_H

#include "config.h"

#include <libaudacious/util.h>
#include <libaudacious/configdb.h>
#include <audacious/main.h>
#include <audacious/plugin.h>
#include <glib/gi18n.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <alsa/pcm_plugin.h>

#include <gtk/gtk.h>

#ifdef WORDS_BIGENDIAN
# define IS_BIG_ENDIAN TRUE
#else
# define IS_BIG_ENDIAN FALSE
#endif

extern OutputPlugin op;

struct alsa_config
{
	char *pcm_device;
	int mixer_card;
	char *mixer_device;
	int buffer_time;
	int period_time;
	gboolean debug;
	struct
	{
		int left, right;
	} vol;
	gboolean soft_volume;
};

extern struct alsa_config alsa_cfg;

void alsa_init(void);
void alsa_cleanup(void);
void alsa_about(void);
void alsa_configure(void);
int alsa_get_mixer(snd_mixer_t **mixer, int card);
void alsa_save_config(void);

void alsa_get_volume(int *l, int *r);
void alsa_set_volume(int l, int r);

int alsa_playing(void);
int alsa_free(void);
void alsa_write(void *ptr, int length);
void alsa_close(void);
void alsa_flush(int time);
void alsa_pause(short p);
int alsa_open(AFormat fmt, int rate, int nch);
int alsa_get_output_time(void);
int alsa_get_written_time(void);
void alsa_tell(AFormat * fmt, gint * rate, gint * nch);

extern GMutex *alsa_mutex;

#endif
