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

#ifndef ESDOUT_H
#define ESDOUT_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "audacious/plugin.h"


extern OutputPlugin op;

typedef struct {
    gboolean use_remote;
    gboolean use_oss_mixer;
    char *server;
    char *hostname;
    char *playername;
    int port;
    int buffer_size;
    int prebuffer;
} ESDConfig;

extern ESDConfig esd_cfg;

void esdout_init(void);
void esdout_about(void);
void esdout_configure(void);

void esdout_get_volume(int *l, int *r);
void esdout_fetch_volume(int *l, int *r);
void esdout_set_volume(int l, int r);
void esdout_mixer_init(void);

int esdout_playing(void);
int esdout_free(void);
void esdout_write(void *ptr, int length);
void esdout_close(void);
void esdout_flush(int time);
void esdout_pause(short p);
int esdout_open(AFormat fmt, int rate, int nch);
int esdout_get_output_time(void);
int esdout_get_written_time(void);
void esdout_set_audio_params(void);
void esdout_tell(AFormat * fmt, gint * rate, gint * nch);

#endif
