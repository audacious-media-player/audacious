/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
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

#ifndef AUDACIOUS_PLAYBACK_H
#define AUDACIOUS_PLAYBACK_H

#include <glib.h>

void playback_play (gint seek_time, gboolean pause);
gint playback_get_time(void);
void playback_pause(void);
void playback_stop(void);
gboolean playback_get_playing(void);
gboolean playback_get_ready (void);
gboolean playback_get_paused(void);
void playback_seek(gint time);

gchar * playback_get_title (void);
gint playback_get_length (void);
void playback_get_info (gint * bitrate, gint * samplerate, gint * channels);

void playback_get_volume (gint * l, gint * r);
void playback_set_volume (gint l, gint r);

#endif /* AUDACIOUS_PLAYBACK_H */
