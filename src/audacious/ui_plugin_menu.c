/*
 * ui_plugin_menu.c
 * Copyright 2009-2010 John Lindgren
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

#include <glib.h>
#include <gtk/gtk.h>

#include "misc.h"

static void destroy_warning (void)
{
    fprintf (stderr, "Interface destroyed a plugin services menu!\n");
}

/* GtkWidget * get_plugin_menu (gint id) */
void * get_plugin_menu (gint id)
{
    static gboolean initted = FALSE;
    static GtkWidget * menus[TOTAL_PLUGIN_MENUS];

    if (! initted)
    {
        memset (menus, 0, sizeof menus);
        initted = TRUE;
    }

    if (menus[id] == NULL)
    {
        menus[id] = gtk_menu_new ();
        g_object_ref ((GObject *) menus[id]);
        g_signal_connect (menus[id], "destroy", (GCallback) destroy_warning,
         NULL);
        gtk_widget_show (menus[id]);
    }

    return menus[id];
}

/* gint menu_plugin_item_add (gint id, GtkWidget * item) */
gint menu_plugin_item_add (gint id, void * item)
{
    gtk_menu_shell_append ((GtkMenuShell *) get_plugin_menu (id), item);
    return 0;
}

/* gint menu_plugin_item_remove (gint id, GtkWidget * item) */
gint menu_plugin_item_remove (gint id, void * item)
{
    gtk_container_remove ((GtkContainer *) get_plugin_menu (id), item);
    return 0;
}
