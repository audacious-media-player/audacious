/*
 * effects-menu.c
 * Copyright 2010-2011 John Lindgren
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

#include "config.h"

#include <audacious/i18n.h>
#include <audacious/plugin.h>
#include <audacious/plugins.h>

#include "libaudgui-gtk.h"

static bool_t watch_cb (PluginHandle * plugin, GtkCheckMenuItem * item)
{
    bool_t enabled = aud_plugin_get_enabled (plugin);
    gtk_check_menu_item_set_active (item, enabled);

    GtkWidget * settings = g_object_get_data ((GObject *) item, "settings");
    if (settings != NULL)
        gtk_widget_set_sensitive (settings, enabled);

    return TRUE;
}

static void enable_cb (GtkCheckMenuItem * item, PluginHandle * plugin)
{
    aud_plugin_enable (plugin, gtk_check_menu_item_get_active (item));
}

static void destroy_cb (GtkCheckMenuItem * item, PluginHandle * plugin)
{
    aud_plugin_remove_watch (plugin, (PluginForEachFunc) watch_cb, item);
}

static void settings_cb (GtkMenuItem * settings, PluginHandle * plugin)
{
    if (! aud_plugin_get_enabled (plugin))
        return;

    Plugin * header = aud_plugin_get_header (plugin);
    g_return_if_fail (header != NULL);
    header->configure ();
}

static bool_t add_item_cb (PluginHandle * plugin, GtkWidget * menu)
{
    GtkWidget * item = gtk_check_menu_item_new_with_label (aud_plugin_get_name
     (plugin));
    gtk_check_menu_item_set_active ((GtkCheckMenuItem *) item,
     aud_plugin_get_enabled (plugin));
    gtk_menu_shell_append ((GtkMenuShell *) menu, item);
    aud_plugin_add_watch (plugin, (PluginForEachFunc) watch_cb, item);
    g_signal_connect (item, "toggled", (GCallback) enable_cb, plugin);
    g_signal_connect (item, "destroy", (GCallback) destroy_cb, plugin);
    gtk_widget_show (item);

    if (aud_plugin_has_configure (plugin))
    {
        GtkWidget * settings = gtk_menu_item_new_with_label (_("settings ..."));
        gtk_widget_set_sensitive (settings, aud_plugin_get_enabled (plugin));
        g_object_set_data ((GObject *) item, "settings", settings);
        gtk_menu_shell_append ((GtkMenuShell *) menu, settings);
        g_signal_connect (settings, "activate", (GCallback) settings_cb, plugin);
        gtk_widget_show (settings);
    }

    return TRUE;
}

EXPORT GtkWidget * audgui_create_effects_menu (void)
{
    GtkWidget * menu = gtk_menu_new ();
    aud_plugin_for_each (PLUGIN_TYPE_EFFECT, (PluginForEachFunc) add_item_cb,
     menu);
    return menu;
}

EXPORT GtkWidget * audgui_create_vis_menu (void)
{
    GtkWidget * menu = gtk_menu_new ();
    aud_plugin_for_each (PLUGIN_TYPE_VIS, (PluginForEachFunc) add_item_cb, menu);
    return menu;
}
