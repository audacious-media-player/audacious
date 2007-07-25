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
#include "playlist_evmessages.h"
#include "playlist_evlisteners.h"

#include "ui_main.h"
#include "ui_skinned_textbox.h"

static void
ui_main_evlistener_title_change(gpointer hook_data, gpointer user_data)
{
    gchar *text = (gchar *) hook_data;

    ui_skinned_textbox_set_text(mainwin_info, text);

    g_free(text);
}

void
ui_main_evlistener_init(void)
{
    hook_associate("title change", ui_main_evlistener_title_change, NULL);
}
