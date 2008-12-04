/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2008 Tomasz Moń <desowin@gmail.com>
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

#ifndef UI_PLAYLIST_WIDGET_H
#define UI_PLAYLIST_WIDGET_H

#include <gtk/gtk.h>
#include "playlist.h"

void ui_playlist_widget_set_current(GtkWidget *treeview, gint pos);
void ui_playlist_widget_update(GtkWidget *widget);
GtkWidget * ui_playlist_widget_new(Playlist *playlist);

#endif
