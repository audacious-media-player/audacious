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
#include "playback.h"
#include "playlist.h"
#include "playback_evlisteners.h"

static void
playback_evlistener_playback_eof(gpointer hook_data, gpointer user_data)
{
    playlist_eof_reached (playlist_get_active ());
}

void
playback_evlistener_init(void)
{
    static int pb_evinit_done = 0;

    if (pb_evinit_done)
        return;

    hook_associate("playback eof", playback_evlistener_playback_eof, NULL);

    pb_evinit_done = 1;
}
