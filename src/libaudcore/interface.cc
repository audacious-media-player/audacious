/*
 * interface.c
 * Copyright 2010-2014 John Lindgren
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

#include "interface.h"
#include "internal.h"

#include <glib.h>

#include "drct.h"
#include "hook.h"
#include "mainloop.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

struct MenuItem {
    const char * name;
    const char * icon;
    void (* func) (void);
};

static PluginHandle * current_plugin;
static PluginHandle * next_plugin;

static IfacePlugin * current_interface;

static Index<MenuItem> menu_items[AUD_MENU_COUNT];

static void add_menu_items (void)
{
    if (! PLUGIN_HAS_FUNC (current_interface, plugin_menu_add))
        return;

    for (int id = 0; id < AUD_MENU_COUNT; id ++)
    {
        for (MenuItem & item : menu_items[id])
            current_interface->plugin_menu_add (id, item.func, item.name, item.icon);
    }
}

static void remove_menu_items (void)
{
    if (! PLUGIN_HAS_FUNC (current_interface, plugin_menu_remove))
        return;

    for (int id = 0; id < AUD_MENU_COUNT; id ++)
    {
        for (MenuItem & item : menu_items[id])
            current_interface->plugin_menu_remove (id, item.func);
    }
}

static bool interface_load (PluginHandle * plugin)
{
    IfacePlugin * i = (IfacePlugin *) aud_plugin_get_header (plugin);
    g_return_val_if_fail (i, false);

    AUDDBG ("Loading %s.\n", aud_plugin_get_name (plugin));

    if (PLUGIN_HAS_FUNC (i, init) && ! i->init ())
        return false;

    current_plugin = plugin;
    current_interface = i;

    add_menu_items ();

    if (PLUGIN_HAS_FUNC (current_interface, show) && aud_get_bool (0, "show_interface"))
        current_interface->show (true);

    return true;
}

static void interface_unload (void)
{
    g_return_if_fail (current_plugin && current_interface);

    AUDDBG ("Unloading %s.\n", aud_plugin_get_name (current_plugin));

    if (PLUGIN_HAS_FUNC (current_interface, show) && aud_get_bool (0, "show_interface"))
        current_interface->show (false);

    remove_menu_items ();

    if (PLUGIN_HAS_FUNC (current_interface, cleanup))
        current_interface->cleanup ();

    current_plugin = nullptr;
    current_interface = nullptr;
}

EXPORT void aud_ui_show (bool_t show)
{
    if (! current_interface)
        return;

    aud_set_bool (0, "show_interface", show);

    if (PLUGIN_HAS_FUNC (current_interface, show))
        current_interface->show (show);

    vis_activate (show);
}

EXPORT bool_t aud_ui_is_shown (void)
{
    if (! current_interface)
        return false;

    return aud_get_bool (0, "show_interface");
}

EXPORT void aud_ui_show_error (const char * message)
{
    if (aud_get_headless_mode ())
        fprintf (stderr, "ERROR: %s\n", message);
    else
        event_queue_full ("ui show error", str_get (message), (GDestroyNotify) str_unref);
}

static bool_t probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    * pp = p;  /* just pick the first one */
    return false;
}

PluginHandle * iface_plugin_probe (void)
{
    PluginHandle * p = nullptr;
    aud_plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) probe_cb, & p);
    return p;
}

PluginHandle * iface_plugin_get_current (void)
{
    return current_plugin;
}

bool iface_plugin_set_current (PluginHandle * plugin)
{
    next_plugin = plugin;

    /* restart main loop, if running */
    aud_quit ();

    return true;
}

static void run_iface_plugins (void)
{
    vis_activate (aud_get_bool (0, "show_interface"));

    while (next_plugin)
    {
        if (! interface_load (next_plugin))
            return;

        next_plugin = nullptr;

        if (PLUGIN_HAS_FUNC (current_interface, run))
            current_interface->run ();
        else
            mainloop_run ();

        /* call before unloading interface */
        hook_call ("config save", nullptr);

        interface_unload ();
    }
}

void interface_run (void)
{
    if (aud_get_headless_mode ())
    {
        mainloop_run ();

        /* call before shutting down */
        hook_call ("config save", nullptr);
    }
    else
        run_iface_plugins ();
}

EXPORT void aud_quit (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, quit))
        current_interface->quit ();
    else
        mainloop_quit ();
}

EXPORT void aud_plugin_menu_add (int id, void (* func) (void), const char * name, const char * icon)
{
    g_return_if_fail (id >= 0 && id < AUD_MENU_COUNT);

    menu_items[id].append ({name, icon, func});

    if (current_interface && PLUGIN_HAS_FUNC (current_interface, plugin_menu_add))
        current_interface->plugin_menu_add (id, func, name, icon);
}

EXPORT void aud_plugin_menu_remove (int id, void (* func) (void))
{
    g_return_if_fail (id >= 0 && id < AUD_MENU_COUNT);

    if (current_interface && PLUGIN_HAS_FUNC (current_interface, plugin_menu_remove))
        current_interface->plugin_menu_remove (id, func);

    Index<MenuItem> & list = menu_items[id];

    for (int i = 0; i < list.len ();)
    {
        if (list[i].func == func)
            list.remove (i, 1);
        else
            i ++;
    }
}

EXPORT void aud_ui_show_about_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_about_window))
        current_interface->show_about_window ();
}

EXPORT void aud_ui_hide_about_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_about_window))
        current_interface->hide_about_window ();
}

EXPORT void aud_ui_show_filebrowser (bool_t open)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_filebrowser))
        current_interface->show_filebrowser (open);
}

EXPORT void aud_ui_hide_filebrowser (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_filebrowser))
        current_interface->hide_filebrowser ();
}

EXPORT void aud_ui_show_jump_to_song (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_jump_to_song))
        current_interface->show_jump_to_song ();
}

EXPORT void aud_ui_hide_jump_to_song (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_jump_to_song))
        current_interface->hide_jump_to_song ();
}

EXPORT void aud_ui_show_prefs_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_prefs_window))
        current_interface->show_prefs_window ();
}

EXPORT void aud_ui_hide_prefs_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_prefs_window))
        current_interface->hide_prefs_window ();
}
