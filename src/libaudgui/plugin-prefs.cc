/*
 * plugin-prefs.c
 * Copyright 2012-2013 John Lindgren
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

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GList * about_windows;
static GList * config_windows;

static int find_cb (GtkWidget * window, PluginHandle * plugin)
{
    return (g_object_get_data ((GObject *) window, "plugin-id") != plugin);
}

static bool watch_cb (PluginHandle * plugin, void * window);

/* window destroyed before plugin disabled */
static void destroy_cb (GtkWidget * window, PluginHandle * plugin)
{
    GList * * list = & config_windows;
    GList * node = g_list_find (* list, window);

    if (! node)
    {
        list = & about_windows;
        node = g_list_find (* list, nullptr);  /* set to nullptr by audgui_simple_message() */
        g_return_if_fail (node);
    }

    aud_plugin_remove_watch (plugin, watch_cb, window);

    * list = g_list_delete_link (* list, node);
}

/* plugin disabled before window destroyed */
static bool watch_cb (PluginHandle * plugin, void * window)
{
    if (aud_plugin_get_enabled (plugin))
        return true;

    GList * * list = & about_windows;
    GList * node = g_list_find (* list, window);

    if (! node)
    {
        list = & config_windows;
        node = g_list_find (* list, window);
        g_return_val_if_fail (node, false);
    }

    g_signal_handlers_disconnect_by_func (window, (void *) destroy_cb, plugin);
    gtk_widget_destroy ((GtkWidget *) window);

    * list = g_list_delete_link (* list, node);

    return false;
}

EXPORT void audgui_show_plugin_about (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (about_windows, plugin, (GCompareFunc) find_cb);

    if (node)
    {
        gtk_window_present ((GtkWindow *) node->data);
        return;
    }

    Plugin * header = (Plugin *) aud_plugin_get_header (plugin);
    g_return_if_fail (header);

    const char * name = header->info.name;
    const char * text = header->info.about;
    if (! text)
        return;

    if (header->info.domain)
    {
        name = dgettext (header->info.domain, name);
        text = dgettext (header->info.domain, text);
    }

    about_windows = node = g_list_prepend (about_windows, nullptr);

    audgui_simple_message ((GtkWidget * *) & node->data, GTK_MESSAGE_INFO,
     str_printf (_("About %s"), name), text);
    g_object_set_data ((GObject *) node->data, "plugin-id", plugin);

    g_signal_connect_after (node->data, "destroy", (GCallback) destroy_cb, plugin);
    aud_plugin_add_watch (plugin, watch_cb, node->data);
}

static void response_cb (GtkWidget * window, int response, const PluginPreferences * p)
{
    if (response == GTK_RESPONSE_OK && p->apply)
        p->apply ();

    gtk_widget_destroy (window);
}

static void cleanup_cb (GtkWidget * window, const PluginPreferences * p)
{
    if (p->cleanup)
        p->cleanup ();
}

EXPORT void audgui_show_plugin_prefs (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (config_windows, plugin, (GCompareFunc) find_cb);

    if (node)
    {
        gtk_window_present ((GtkWindow *) node->data);
        return;
    }

    Plugin * header = (Plugin *) aud_plugin_get_header (plugin);
    g_return_if_fail (header);

    const PluginPreferences * p = header->info.prefs;
    if (! p)
        return;

    if (p->init)
        p->init ();

    const char * name = header->info.name;
    if (header->info.domain)
        name = dgettext (header->info.domain, name);

    GtkWidget * window = gtk_dialog_new ();
    gtk_window_set_title ((GtkWindow *) window, str_printf (_("%s Settings"), name));

    if (p->apply)
    {
        GtkWidget * button1 = audgui_button_new (_("_Set"), "system-run", nullptr, nullptr);
        GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", nullptr, nullptr);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button2, GTK_RESPONSE_CANCEL);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button1, GTK_RESPONSE_OK);
    }
    else
    {
        GtkWidget * button = audgui_button_new (_("_Close"), "window-close", nullptr, nullptr);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button, GTK_RESPONSE_CLOSE);
    }

    GtkWidget * content = gtk_dialog_get_content_area ((GtkDialog *) window);
    GtkWidget * box = gtk_vbox_new (false, 0);
    audgui_create_widgets_with_domain (box, p->widgets, header->info.domain);
    gtk_box_pack_start ((GtkBox *) content, box, true, true, 0);

    g_signal_connect (window, "response", (GCallback) response_cb, (void *) p);
    g_signal_connect (window, "destroy", (GCallback) cleanup_cb, (void *) p);

    gtk_widget_show_all (window);

    g_object_set_data ((GObject *) window, "plugin-id", plugin);
    config_windows = g_list_prepend (config_windows, window);

    g_signal_connect_after (window, "destroy", (GCallback) destroy_cb, plugin);
    aud_plugin_add_watch (plugin, watch_cb, window);
}

void plugin_prefs_cleanup ()
{
    g_list_foreach (about_windows, (GFunc) gtk_widget_destroy, nullptr);
    g_list_foreach (config_windows, (GFunc) gtk_widget_destroy, nullptr);
}
