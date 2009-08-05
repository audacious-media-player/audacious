/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team.
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

#include <gtk/gtk.h>
#include "audacious/images/audacious_player.xpm"

void
audgui_set_default_icon(void)
{
    GdkPixbuf *icon;

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) audacious_player_xpm);
    gtk_window_set_default_icon(icon);
    g_object_unref(icon);
}

