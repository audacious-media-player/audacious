/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team.
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

#ifndef AUDACIOUS_UI_PLUGIN_MENU_H
#define AUDACIOUS_UI_PLUGIN_MENU_H

#include <glib.h>

#define AUDACIOUS_MENU_MAIN             0
#define AUDACIOUS_MENU_PLAYLIST         1
#define AUDACIOUS_MENU_PLAYLIST_RCLICK  2
#define AUDACIOUS_MENU_PLAYLIST_ADD     3
#define AUDACIOUS_MENU_PLAYLIST_REMOVE  4
#define AUDACIOUS_MENU_PLAYLIST_SELECT  5
#define AUDACIOUS_MENU_PLAYLIST_MISC    6
#define TOTAL_PLUGIN_MENUS 7

/* GtkWidget * get_plugin_menu (gint id); */
void * get_plugin_menu (gint id);
/* gint menu_plugin_item_add (gint id, GtkWidget * item); */
gint menu_plugin_item_add (gint id, void * item);
/* gint menu_plugin_item_remove (gint id, GtkWidget * item); */
gint menu_plugin_item_remove (gint id, void * item);

#endif /* AUDACIOUS_UI_PLUGIN_MENU_H */
