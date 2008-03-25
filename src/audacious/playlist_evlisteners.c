/*
 * Audacious
 * Copyright (c) 2006-2007 Audacious development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include "hook.h"
#include "playlist.h"
#include "ui_playlist.h"

#include "playlist_evlisteners.h"

static void
playlist_evlistener_playlistwin_update_list(gpointer hook_data, gpointer user_data)
{
     Playlist *playlist = (Playlist *) hook_data;

     playlistwin_update_list(playlist);
}

void playlist_evlistener_init(void)
{
     hook_associate("playlistwin update list", playlist_evlistener_playlistwin_update_list, NULL);
}
