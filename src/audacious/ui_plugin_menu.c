/*
 * ui_plugin_menu.c
 * Copyright 2009-2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <gtk/gtk.h>

#include <libaudgui/libaudgui-gtk.h>

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
    GtkWidget * widget = audgui_menu_item_new (item->name, item->icon);
    g_object_set_data ((GObject *) widget, "func", (void *) item->func);
    g_signal_connect (widget, "activate", item->func, NULL);
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
