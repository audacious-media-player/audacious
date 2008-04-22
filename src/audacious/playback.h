/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
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

#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <glib.h>

#include "playlist.h"
#include "plugin.h"

typedef struct _PlaybackInfo PlaybackInfo;

struct _PlaybackInfo {
    gint bitrate;
    gint frequency;
    gint n_channels;
};

gint playback_get_time(void);
gint playback_get_length(void);
void playback_initiate(void);
void playback_pause(void);
void playback_stop(void);
gboolean playback_play_file(PlaylistEntry *entry);
gboolean playback_get_playing(void);
gboolean playback_get_paused(void);
void playback_seek(gint time);
void playback_seek_relative(gint offset);
void playback_eof(void);
void playback_error(void);
InputPlayback *playback_new(void);
void playback_free(InputPlayback *);
void playback_run(InputPlayback *);
void playback_get_sample_params(gint *bitrate, gint *frequency, gint *n_channels);
void playback_set_sample_params(gint bitrate, gint frequency, gint n_channels);

#endif
