/*
 * interface.c
 * Copyright 2010-2013 John Lindgren
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
#include <pthread.h>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudgui/init.h>
#include <libaudgui/libaudgui-gtk.h>

#include "interface.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"
#include "ui_preferences.h"
#include "visualization.h"

extern AudAPITable api_table;

static IfacePlugin * current_interface = NULL;

static pthread_mutex_t error_mutex = PTHREAD_MUTEX_INITIALIZER;
static GQueue error_queue = G_QUEUE_INIT;
static int error_source;
static GtkWidget * error_win;

bool_t interface_load (PluginHandle * plugin)
{
    IfacePlugin * i = plugin_get_header (plugin);
    g_return_val_if_fail (i, FALSE);

    audgui_init (& api_table, _AUD_PLUGIN_VERSION);

    if (PLUGIN_HAS_FUNC (i, init) && ! i->init ())
    {
        audgui_cleanup ();
        return FALSE;
    }

    current_interface = i;
    return TRUE;
}

void interface_unload (void)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, cleanup))
        current_interface->cleanup ();

    current_interface = NULL;

    hide_prefs_window ();
    plugin_preferences_cleanup ();
    audgui_cleanup ();
}

void interface_show (bool_t show)
{
    g_return_if_fail (current_interface);

    aud_set_bool (NULL, "show_interface", show);

    if (PLUGIN_HAS_FUNC (current_interface, show))
        current_interface->show (show);

    vis_activate (show);
}

bool_t interface_is_shown (void)
{
    g_return_val_if_fail (current_interface, FALSE);

    return aud_get_bool (NULL, "show_interface");
}

static bool_t error_idle_func (void * unused)
{
    pthread_mutex_lock (& error_mutex);

    char * message;
    while ((message = g_queue_pop_head (& error_queue)))
    {
        pthread_mutex_unlock (& error_mutex);

        if (aud_get_headless_mode ())
            fprintf (stderr, "ERROR: %s\n", message);
        else
            audgui_simple_message (& error_win, GTK_MESSAGE_ERROR, _("Error"), message);

        str_unref (message);

        pthread_mutex_lock (& error_mutex);
    }

    error_source = 0;

    pthread_mutex_unlock (& error_mutex);
    return FALSE;
}

void interface_show_error (const char * message)
{
    pthread_mutex_lock (& error_mutex);

    g_queue_push_tail (& error_queue, str_get (message));

    if (! error_source)
        error_source = g_idle_add (error_idle_func, NULL);

    pthread_mutex_unlock (& error_mutex);
}

static bool_t probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    * pp = p;
    return FALSE;
}

PluginHandle * iface_plugin_probe (void)
{
    PluginHandle * p = NULL;
    plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) probe_cb, & p);
    return p;
}

static PluginHandle * current_plugin = NULL;

PluginHandle * iface_plugin_get_current (void)
{
    return current_plugin;
}

bool_t iface_plugin_set_current (PluginHandle * plugin)
{
    hook_call ("config save", NULL); /* tell interface to save layout */

    bool_t shown = aud_get_bool (NULL, "show_interface");

    if (current_plugin)
    {
        if (shown && current_interface && PLUGIN_HAS_FUNC (current_interface, show))
            current_interface->show (FALSE);

        AUDDBG ("Unloading %s.\n", plugin_get_name (current_plugin));
        interface_unload ();

        current_plugin = NULL;
    }

    if (plugin)
    {
        AUDDBG ("Loading %s.\n", plugin_get_name (plugin));

        if (! interface_load (plugin))
            return FALSE;

        current_plugin = plugin;

        if (shown && current_interface && PLUGIN_HAS_FUNC (current_interface, show))
            current_interface->show (TRUE);
    }

    vis_activate (shown && current_interface);

    return TRUE;
}
