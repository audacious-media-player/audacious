/*  Audacious - Cross platform multimedia player
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

#ifndef AUDACIOUS_INPUT_H
#define AUDACIOUS_INPUT_H

#include "plugin.h"

typedef struct
{
    InputPlayback *current_input_playback;
    gboolean playing;
    gboolean paused;
    gboolean stop;
    GMutex *playback_mutex;
}
PlaybackData;

struct _VisNode {
    gint time;
    gint nch;
    gint length; /* In frames. Always 512. */
    gint16 data[2][512];
};

typedef struct _VisNode VisNode;

GList *get_input_list(void);
InputPlayback *get_current_input_playback(void);
void set_current_input_playback(InputPlayback * ip);
void set_current_input_data(void * data);

Tuple *input_get_song_tuple(const gchar * filename);

void input_get_volume(gint * l, gint * r);
void input_set_volume(gint l, gint r);
void input_file_info_box(const gchar * filename);

extern PlaybackData ip_data;

#endif /* AUDACIOUS_INPUT_H */
