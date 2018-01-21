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

#include <assert.h>
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
    void (* func) ();
};

static PluginHandle * current_plugin;
static IfacePlugin * current_interface;

static aud::array<AudMenuID, Index<MenuItem>> menu_items;

static void add_menu_items ()
{
    for (AudMenuID id : aud::range<AudMenuID> ())
    {
        for (MenuItem & item : menu_items[id])
            current_interface->plugin_menu_add (id, item.func, item.name, item.icon);
    }
}

static void remove_menu_items ()
{
    for (AudMenuID id : aud::range<AudMenuID> ())
    {
        for (MenuItem & item : menu_items[id])
            current_interface->plugin_menu_remove (id, item.func);
    }
}

static bool interface_load (PluginHandle * plugin)
{
    auto i = (IfacePlugin *) aud_plugin_get_header (plugin);
    if (! i)
        return false;

    AUDINFO ("Loading %s.\n", aud_plugin_get_name (plugin));

    if (! i->init ())
        return false;

    current_interface = i;

    add_menu_items ();

    if (aud_get_bool (0, "show_interface"))
        current_interface->show (true);

    return true;
}

static void interface_unload ()
{
    AUDINFO ("Unloading %s.\n", aud_plugin_get_name (current_plugin));

    // call before unloading interface
    hook_call ("config save", nullptr);

    if (aud_get_bool (0, "show_interface"))
        current_interface->show (false);

    remove_menu_items ();

    current_interface->cleanup ();
    current_interface = nullptr;
}

EXPORT void aud_ui_show (bool show)
{
    if (! current_interface)
        return;

    aud_set_bool (0, "show_interface", show);

    current_interface->show (show);

    vis_activate (show);
}

EXPORT bool aud_ui_is_shown ()
{
    if (! current_interface)
        return false;

    return aud_get_bool (0, "show_interface");
}

EXPORT void aud_ui_startup_notify (const char * id)
{
    if (current_interface)
        current_interface->startup_notify (id);
}

EXPORT void aud_ui_show_error (const char * message)
{
    if (aud_get_headless_mode ())
        AUDERR ("%s\n", message);
    else
        event_queue ("ui show error", g_strdup (message), g_free);
}

PluginHandle * iface_plugin_get_current ()
{
    return current_plugin;
}

bool iface_plugin_set_current (PluginHandle * plugin)
{
    if (current_interface)
        interface_unload ();

    if (! interface_load (plugin))
        return false;

    current_plugin = plugin;
    return true;
}

void interface_run ()
{
    if (aud_get_headless_mode ())
    {
        mainloop_run ();

        // call before shutting down
        hook_call ("config save", nullptr);
    }
    else if (current_interface)
    {
        vis_activate (aud_get_bool (0, "show_interface"));

        current_interface->run ();
        interface_unload ();
    }
}

EXPORT void aud_quit ()
{
    // Qt is very sensitive to things being deleted in the correct order
    // to avoid upsetting it, we'll stop all queued callbacks right now
    QueuedFunc::inhibit_all ();

    if (current_interface)
        current_interface->quit ();
    else
        mainloop_quit ();
}

EXPORT void aud_plugin_menu_add (AudMenuID id, void (* func) (), const char * name, const char * icon)
{
    menu_items[id].append (name, icon, func);

    if (current_interface)
        current_interface->plugin_menu_add (id, func, name, icon);
}

EXPORT void aud_plugin_menu_remove (AudMenuID id, void (* func) ())
{
    if (current_interface)
        current_interface->plugin_menu_remove (id, func);

    auto is_match = [=] (const MenuItem & item)
        { return item.func == func; };

    menu_items[id].remove_if (is_match, true);
}

EXPORT void aud_ui_show_about_window ()
{
    if (current_interface)
        current_interface->show_about_window ();
}

EXPORT void aud_ui_hide_about_window ()
{
    if (current_interface)
        current_interface->hide_about_window ();
}

EXPORT void aud_ui_show_filebrowser (bool open)
{
    if (current_interface)
        current_interface->show_filebrowser (open);
}

EXPORT void aud_ui_hide_filebrowser ()
{
    if (current_interface)
        current_interface->hide_filebrowser ();
}

EXPORT void aud_ui_show_jump_to_song ()
{
    if (current_interface)
        current_interface->show_jump_to_song ();
}

EXPORT void aud_ui_hide_jump_to_song ()
{
    if (current_interface)
        current_interface->hide_jump_to_song ();
}

EXPORT void aud_ui_show_prefs_window ()
{
    if (current_interface)
        current_interface->show_prefs_window ();
}

EXPORT void aud_ui_hide_prefs_window ()
{
    if (current_interface)
        current_interface->hide_prefs_window ();
}
