/*
 * ui_plugin_menu.c
 * Copyright 2009-2011 John Lindgren
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

#include <glib.h>
#include <gtk/gtk.h>

#include "misc.h"

struct Item {
    MenuFunc func;
    const char * name;
    const char * icon;
};

static GList * items[AUD_MENU_COUNT];
static GtkWidget * menus[AUD_MENU_COUNT];

static void add_to_menu (GtkWidget * menu, struct Item * item)
{
    GtkWidget * widget = gtk_image_menu_item_new_with_mnemonic (item->name);
    g_object_set_data ((GObject *) widget, "func", (void *) item->func);
    g_signal_connect (widget, "activate", item->func, NULL);

    if (item->icon)
        gtk_image_menu_item_set_image ((GtkImageMenuItem *) widget,
         gtk_image_new_from_stock (item->icon, GTK_ICON_SIZE_MENU));

    gtk_widget_show (widget);
    gtk_menu_shell_append ((GtkMenuShell *) menu, widget);
}

/* GtkWidget * get_plugin_menu (int id) */
void * get_plugin_menu (int id)
{
    if (! menus[id])
    {
        menus[id] = gtk_menu_new ();
        g_signal_connect (menus[id], "destroy", (GCallback)
         gtk_widget_destroyed, & menus[id]);

        for (GList * node = items[id]; node; node = node->next)
            add_to_menu (menus[id], node->data);
    }

    return menus[id];
}

void plugin_menu_add (int id, MenuFunc func, const char * name,
 const char * icon)
{
    struct Item * item = g_slice_new (struct Item);
    item->name = name;
    item->icon = icon;
    item->func = func;

    items[id] = g_list_append (items[id], item);

    if (menus[id])
        add_to_menu (menus[id], item);
}

static void remove_cb (GtkWidget * widget, MenuFunc func)
{
    if ((MenuFunc) g_object_get_data ((GObject *) widget, "func") == func)
        gtk_widget_destroy (widget);
}

void plugin_menu_remove (int id, MenuFunc func)
{
    if (menus[id])
        gtk_container_foreach ((GtkContainer *) menus[id], (GtkCallback)
         remove_cb, (void *) func);

    GList * next;
    for (GList * node = items[id]; node; node = next)
    {
        next = node->next;

        if (((struct Item *) node->data)->func == func)
        {
            g_slice_free (struct Item, node->data);
            items[id] = g_list_delete_link (items[id], node);
        }
    }
}
