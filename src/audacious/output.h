/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <glib.h>

#include "plugin.h"

typedef struct _OutputPluginData OutputPluginData;

struct _OutputPluginData {
    GList *output_list;
    OutputPlugin *current_output_plugin;
};

typedef struct _OutputPluginState OutputPluginState;

struct _OutputPluginState {
    AFormat fmt;
    gint rate;
    gint nch;
};

GList *get_output_list(void);
OutputPlugin *get_current_output_plugin(void);
void set_current_output_plugin(gint i);
void output_about(gint i);
void output_configure(gint i);
void output_get_volume(gint * l, gint * r);
void output_set_volume(gint l, gint r);
void output_set_eq(gboolean, gfloat, gfloat *);
gint output_open_audio(AFormat, gint, gint);
void output_write_audio(gpointer ptr, gint length);
void output_close_audio(void);

void output_flush(gint);
void output_pause(gshort);
gint output_buffer_free(void);
gint output_buffer_playing(void);

void output_pass_audio(InputPlayback *, AFormat, gint, gint, gpointer, int *);

gint get_written_time(void);
gint get_output_time(void);

extern OutputPlugin psuedo_output_plugin;
extern OutputPluginData op_data;

#endif
