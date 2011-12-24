/*
 * libaudgui/init.c
 * Copyright 2010-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static const char * const audgui_defaults[] = {
 "close_dialog_add", "FALSE",
 "close_dialog_open", "TRUE",
 "close_jtf_dialog", "TRUE",
 "playlist_manager_close_on_activate", "FALSE",
 "remember_jtf_entry", "TRUE",
 NULL};

AudAPITable * _aud_api_table = NULL;

static void playlist_set_playing_cb (void * unused, void * unused2)
{
    audgui_pixbuf_uncache ();
}

static void playlist_position_cb (void * list, void * unused)
{
    if (GPOINTER_TO_INT (list) == aud_playlist_get_playing ())
        audgui_pixbuf_uncache ();
}

void audgui_init (AudAPITable * table)
{
    _aud_api_table = table;
    aud_config_set_defaults ("audgui", audgui_defaults);

    hook_associate ("playlist set playing", playlist_set_playing_cb, NULL);
    hook_associate ("playlist position", playlist_position_cb, NULL);
}

void audgui_cleanup (void)
{
    hook_dissociate ("playlist set playing", playlist_set_playing_cb);
    hook_dissociate ("playlist position", playlist_position_cb);

    audgui_jump_to_track_hide ();
    audgui_pixbuf_uncache ();

    _aud_api_table = NULL;
}
