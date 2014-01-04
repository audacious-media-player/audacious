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

#include <libaudgui/menu.h>

#include "i18n.h"
#include "misc.h"

static GList * items[AUD_MENU_COUNT]; /* of AudguiMenuItem */
static GtkWidget * menus[AUD_MENU_COUNT];

static void configure_plugins (void)
{
    show_prefs_for_plugin_type (PLUGIN_TYPE_GENERAL);
}

static const AudguiMenuItem main_items[] = {
    {N_("_Plugins ..."), .func = configure_plugins},
    {.sep = TRUE}
};

static void add_to_menu (GtkWidget * menu, const AudguiMenuItem * item)
{
    GtkWidget * widget = audgui_menu_item_new_with_domain (item, NULL, NULL);
    g_object_set_data ((GObject *) widget, "func", (void *) item->func);
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

        if (id == AUD_MENU_MAIN)
            audgui_menu_init (menus[id], main_items, ARRAY_LEN (main_items), NULL);

        for (GList * node = items[id]; node; node = node->next)
            add_to_menu (menus[id], node->data);
    }

    return menus[id];
}

void plugin_menu_add (int id, MenuFunc func, const char * name,
 const char * icon)
{
    AudguiMenuItem * item = g_slice_new0 (AudguiMenuItem);
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

        if (((AudguiMenuItem *) node->data)->func == func)
        {
            g_slice_free (AudguiMenuItem, node->data);
            items[id] = g_list_delete_link (items[id], node);
        }
    }
}
