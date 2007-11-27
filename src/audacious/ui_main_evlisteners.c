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
#include <math.h>

#include "hook.h"
#include "playback.h"
#include "playlist.h"
#include "playlist_evmessages.h"
#include "playlist_evlisteners.h"

#include "ui_main.h"
#include "ui_equalizer.h"
#include "ui_skinned_textbox.h"
#include "ui_playlist.h"

static void
ui_main_evlistener_title_change(gpointer hook_data, gpointer user_data)
{
    gchar *text = (gchar *) hook_data;

    ui_skinned_textbox_set_text(mainwin_info, text);
    playlistwin_update_list(playlist_get_active());
    g_free(text);
}

static void
ui_main_evlistener_hide_seekbar(gpointer hook_data, gpointer user_data)
{
    mainwin_disable_seekbar();
}

static void
ui_main_evlistener_volume_change(gpointer hook_data, gpointer user_data)
{
    gint *h_vol = (gint *) hook_data;
    gint vl, vr, b, v;

    vl = CLAMP(h_vol[0], 0, 100);
    vr = CLAMP(h_vol[1], 0, 100);
    v = MAX(vl, vr);
    if (vl > vr)
        b = (gint) rint(((gdouble) vr / vl) * 100) - 100;
    else if (vl < vr)
        b = 100 - (gint) rint(((gdouble) vl / vr) * 100);
    else
        b = 0;

    mainwin_set_volume_slider(v);
    equalizerwin_set_volume_slider(v);
    mainwin_set_balance_slider(b);
    equalizerwin_set_balance_slider(b);
}

static void
ui_main_evlistener_playback_initiate(gpointer hook_data, gpointer user_data)
{
    playback_initiate();
}

void
ui_main_evlistener_init(void)
{
    hook_associate("title change", ui_main_evlistener_title_change, NULL);
    hook_associate("hide seekbar", ui_main_evlistener_hide_seekbar, NULL);
    hook_associate("volume set", ui_main_evlistener_volume_change, NULL);
    hook_associate("playback initiate", ui_main_evlistener_playback_initiate, NULL);
}

