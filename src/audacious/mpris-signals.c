/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team.
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#ifdef USE_DBUS

#include "dbus-service.h"
#include "main.h"

static void mpris_status_cb (void * hook_data, void * user_data)
{
    mpris_emit_status_change (mpris, GPOINTER_TO_INT (user_data));
}
#endif

void mpris_signals_init (void)
{
#ifdef USE_DBUS
    hook_associate ("playback begin", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PLAY));
    hook_associate ("playback pause", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PAUSE));
    hook_associate ("playback unpause", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PLAY));
    hook_associate ("playback stop", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_STOP));

    hook_associate ("set shuffle", mpris_status_cb, GINT_TO_POINTER (MPRIS_STATUS_INVALID));
    hook_associate ("set repeat", mpris_status_cb, GINT_TO_POINTER (MPRIS_STATUS_INVALID));
    hook_associate ("set no_playlist_advance", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_INVALID));
#endif
}

void mpris_signals_cleanup (void)
{
#ifdef USE_DBUS
    hook_dissociate ("playback begin", mpris_status_cb);
    hook_dissociate ("playback pause", mpris_status_cb);
    hook_dissociate ("playback unpause", mpris_status_cb);
    hook_dissociate ("playback stop", mpris_status_cb);

    hook_dissociate ("set shuffle", mpris_status_cb);
    hook_dissociate ("set repeat", mpris_status_cb);
    hook_dissociate ("set no_playlist_advance", mpris_status_cb);
#endif
}
