/*
 * plugin-menu.c
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

#include <gtk/gtk.h>

#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugins.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "menu.h"

static aud::array<AudMenuID, GList *> items; /* of AudguiMenuItem */
static aud::array<AudMenuID, GtkWidget *> menus;

static void configure_plugins ()
{
    audgui_show_prefs_for_plugin_type (PluginType::General);
}

static const AudguiMenuItem main_items[] = {
    MenuCommand (N_("_Plugins ..."), 0, 0, (GdkModifierType) 0, configure_plugins),
    MenuSep ()
};

static void add_to_menu (GtkWidget * menu, const AudguiMenuItem * item)
{
    GtkWidget * widget = audgui_menu_item_new_with_domain (item, nullptr, nullptr);
    g_object_set_data ((GObject *) widget, "func", (void *) item->func);
    gtk_widget_show (widget);
    gtk_menu_shell_append ((GtkMenuShell *) menu, widget);
}

EXPORT GtkWidget * audgui_get_plugin_menu (AudMenuID id)
{
    if (! menus[id])
    {
        menus[id] = gtk_menu_new ();
        g_signal_connect (menus[id], "destroy", (GCallback)
         gtk_widget_destroyed, & menus[id]);

        if (id == AudMenuID::Main)
            audgui_menu_init (menus[id], main_items, nullptr);

        for (GList * node = items[id]; node; node = node->next)
            add_to_menu (menus[id], (const AudguiMenuItem *) node->data);
    }

    return menus[id];
}

EXPORT void audgui_plugin_menu_add (AudMenuID id, void (* func) (),
 const char * name, const char * icon)
{
    AudguiMenuItem * item = g_slice_new0 (AudguiMenuItem);
    item->name = name;
    item->icon = icon;
    item->func = func;

    items[id] = g_list_append (items[id], item);

    if (menus[id])
        add_to_menu (menus[id], item);
}

static void remove_cb (GtkWidget * widget, void (* func) ())
{
    if (g_object_get_data ((GObject *) widget, "func") == (void *) func)
        gtk_widget_destroy (widget);
}

EXPORT void audgui_plugin_menu_remove (AudMenuID id, void (* func) ())
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

void plugin_menu_cleanup ()
{
    for (AudMenuID id : aud::range<AudMenuID> ())
    {
        g_warn_if_fail (! items[id]);

        if (menus[id])
            gtk_widget_destroy (menus[id]);
    }
}
