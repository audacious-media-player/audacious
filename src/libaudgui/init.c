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

static const gchar * const audgui_defaults[] = {
 "close_dialog_open", "TRUE",
 "close_jtf_dialog", "TRUE",
 "playlist_manager_close_on_activate", "FALSE",
 "remember_jtf_entry", "TRUE",
 NULL};

AudAPITable * _aud_api_table = NULL;

void audgui_init (AudAPITable * table)
{
    _aud_api_table = table;
    aud_config_set_defaults ("audgui", audgui_defaults);
}
