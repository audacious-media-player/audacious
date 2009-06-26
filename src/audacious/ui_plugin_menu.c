// ui_plugin_menu.c
// Copyright 2009 John Lindgren

// This file is part of Audacious.

// Audacious is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, version 3 of the License.

// Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with
// Audacious. If not, see <http://www.gnu.org/licenses/>.

// The Audacious team does not consider modular code linking to Audacious or
// using our public API to be a derived work.

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "ui_plugin_menu.h"

// Note: These functions were originally intended as temporary for Audacious
// 2.0. They are quick and dirty, and they are only useful for a GTK-based
// interface plugin. -jlindgren, 2009-6-26

GtkWidget * get_plugin_menu (int id)
{
    static char initted = 0;
    static GtkWidget * menus [TOTAL_PLUGIN_MENUS];

    if (! initted)
    {
        memset (menus, 0, sizeof menus);
        initted = 1;
    }

    if (! menus [id])
    {
        menus [id] = gtk_menu_new ();
        g_object_ref (G_OBJECT (menus [id]));
        gtk_widget_show (menus [id]);
    }

    return menus [id];
}

gint menu_plugin_item_add (gint menu_id, GtkWidget * item)
{
    gtk_menu_shell_append ((GtkMenuShell *) get_plugin_menu (menu_id), item);
    return 0;
}

gint menu_plugin_item_remove (gint menu_id, GtkWidget * item)
{
    gtk_container_remove ((GtkContainer *) get_plugin_menu (menu_id), item);
    return 0;
}
