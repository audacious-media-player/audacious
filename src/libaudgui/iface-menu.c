/*
 * iface-menu.c
 * Copyright 2010 John Lindgren
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
#include <audacious/plugins.h>

#include "libaudgui-gtk.h"

static void switch_cb (GtkMenuItem * item, PluginHandle * plugin)
{
    if (gtk_check_menu_item_get_active ((GtkCheckMenuItem *) item))
        aud_plugin_enable (plugin, TRUE);
}

typedef struct {
    GtkWidget * menu;
    GSList * group;
} IfaceMenuAddState;

static bool_t add_item_cb (PluginHandle * plugin, IfaceMenuAddState * state)
{
    GtkWidget * item = gtk_radio_menu_item_new_with_label (state->group,
     aud_plugin_get_name (plugin));
    state->group = gtk_radio_menu_item_get_group ((GtkRadioMenuItem *) item);
    if (aud_plugin_get_enabled (plugin))
        gtk_check_menu_item_set_active ((GtkCheckMenuItem *) item, TRUE);
    gtk_menu_shell_append ((GtkMenuShell *) state->menu, item);
    g_signal_connect (item, "activate", (GCallback) switch_cb, plugin);
    gtk_widget_show (item);
    return TRUE;
}

EXPORT GtkWidget * audgui_create_iface_menu (void)
{
    IfaceMenuAddState state = {gtk_menu_new (), NULL};
    aud_plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) add_item_cb,
     & state);
    return state.menu;
}
