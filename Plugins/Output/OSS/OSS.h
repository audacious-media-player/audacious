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

#ifndef OSS_H
#define OSS_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "soundcard.h"
#include "audacious/plugin.h"

#define IS_BIG_ENDIAN (G_BYTE_ORDER == G_BIG_ENDIAN)

extern OutputPlugin op;

typedef struct {
    gint audio_device;
    gint mixer_device;
    gint buffer_size;
    gint prebuffer;
    gboolean use_master;
    gboolean use_alt_audio_device, use_alt_mixer_device;
    gchar *alt_audio_device, *alt_mixer_device;
} OSSConfig;

extern OSSConfig oss_cfg;

void oss_init(void);
void oss_cleanup(void);
void oss_about(void);
void oss_configure(void);

void oss_get_volume(int *l, int *r);
void oss_set_volume(int l, int r);

int oss_playing(void);
int oss_free(void);
void oss_write(void *ptr, int length);
void oss_close(void);
void oss_flush(int time);
void oss_pause(short p);
int oss_open(AFormat fmt, int rate, int nch);
int oss_get_output_time(void);
int oss_get_written_time(void);
void oss_set_audio_params(void);

void oss_free_convert_buffer(void);
int (*oss_get_convert_func(int output, int input)) (void **, int);
int (*oss_get_stereo_convert_func(int output, int input)) (void **, int,
                                                           int);

void oss_tell(AFormat * fmt, gint * rate, gint * nch);

#endif
