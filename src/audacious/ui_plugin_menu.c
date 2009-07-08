/*
 * ui_plugin_menu.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "ui_plugin_menu.h"

/*
 * Note: These functions were originally intended as temporary for Audacious
 * 2.0. They are quick and dirty, and they are only useful for a GTK-based
 * interface plugin. -jlindgren
 */

GtkWidget * get_plugin_menu (gint id)
{
    static gboolean initted = FALSE;
    static GtkWidget * menus[TOTAL_PLUGIN_MENUS];

    if (! initted)
    {
        gint count;

        for (count = 0; count < TOTAL_PLUGIN_MENUS; count ++)
            menus[count] = NULL;

        initted = TRUE;
    }

    if (menus[id] == NULL)
    {
        menus[id] = gtk_menu_new ();
        g_object_ref ((GObject *) menus[id]);
        gtk_widget_show (menus[id]);
    }

    return menus[id];
}

gint menu_plugin_item_add (gint id, GtkWidget * item)
{
    gtk_menu_shell_append ((GtkMenuShell *) get_plugin_menu (id), item);
    return 0;
}

gint menu_plugin_item_remove (gint id, GtkWidget * item)
{
    gtk_container_remove ((GtkContainer *) get_plugin_menu (id), item);
    return 0;
}
