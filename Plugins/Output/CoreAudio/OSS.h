/*
 *  Copyright (C) 2006 William Pitcock <nenolod -at- nenolod.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef COREAUDIO_H
#define COREAUDIO_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

#include "audacious/plugin.h"

#define IS_BIG_ENDIAN (G_BYTE_ORDER == G_BIG_ENDIAN)

extern OutputPlugin op;

struct CoreAudioState {
	
};

void ca_init(void);
void ca_cleanup(void);
void ca_about(void);
void ca_configure(void);

void ca_get_volume(int *l, int *r);
void ca_set_volume(int l, int r);

int ca_playing(void);
int ca_free(void);
void ca_write(void *ptr, int length);
void ca_close(void);
void ca_flush(int time);
void ca_pause(short p);
int ca_open(AFormat fmt, int rate, int nch);
int ca_get_output_time(void);
int ca_get_written_time(void);
void ca_set_audio_params(void);

void ca_free_convert_buffer(void);
int (*ca_get_convert_func(int output, int input)) (void **, int);
int (*ca_get_stereo_convert_func(int output, int input)) (void **, int,
                                                           int);

void ca_tell(AFormat * fmt, gint * rate, gint * nch);

#endif
