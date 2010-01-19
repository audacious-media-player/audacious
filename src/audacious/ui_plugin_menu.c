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

#include <glib.h>
#include <gtk/gtk.h>

#include "ui_plugin_menu.h"

/* A StaticMenu is simply a GtkMenu that is created with a (normal, not
 * floating) reference count of 1 and resists the "destroy" signal. Thus, it can
 * be added into an interface and remain after the interface is destroyed. */

typedef struct
{
    GtkMenuClass parent;
}
StaticMenuClass;

typedef struct
{
    GtkMenu parent;
}
StaticMenu;

G_DEFINE_TYPE (StaticMenu, static_menu, GTK_TYPE_MENU)

static void static_menu_destroy (GtkObject * object)
{
    /* Do NOT chain to the parent class! */
}

static void static_menu_class_init (StaticMenuClass * class)
{
    ((GtkObjectClass *) class)->destroy = static_menu_destroy;
}

static void static_menu_init (StaticMenu * menu)
{
    g_object_ref (menu);
}

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
        menus[id] = g_object_new (static_menu_get_type (), NULL);
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
