/*
 * plugin-init.c
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

#include "plugins-internal.h"

#include <assert.h>
#include <stdlib.h>

#include "hook.h"
#include "interface.h"
#include "internal.h"
#include "output.h"
#include "plugin.h"
#include "runtime.h"

static bool general_plugin_start (PluginHandle * plugin)
{
    auto gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
    return gp && gp->init ();
}

void general_plugin_stop (PluginHandle * plugin)
{
    GeneralPlugin * gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
    if (gp)
        gp->cleanup ();
}

struct MultiFuncs
{
    bool (* start) (PluginHandle * plugin);
    void (* stop) (PluginHandle * plugin);
};

struct SingleFuncs
{
    PluginHandle * (* get_current) ();
    bool (* set_current) (PluginHandle * plugin);
};

union PluginFuncs
{
    MultiFuncs m;
    SingleFuncs s;

    constexpr PluginFuncs (MultiFuncs multi) : m (multi) {}
    constexpr PluginFuncs (SingleFuncs single) : s (single) {}
};

struct PluginParams
{
    const char * name;
    bool is_single;
    PluginFuncs f;

    constexpr PluginParams (const char * name, MultiFuncs multi) :
        name (name), is_single (false), f (multi) {}
    constexpr PluginParams (const char * name, SingleFuncs single) :
        name (name), is_single (true), f (single) {}
};

static constexpr aud::array<PluginType, PluginParams> table = {
    PluginParams ("transport", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("playlist", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("input", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("effect", MultiFuncs ({effect_plugin_start, effect_plugin_stop})),
    PluginParams ("output", SingleFuncs ({output_plugin_get_current, output_plugin_set_current})),
    PluginParams ("visualization", MultiFuncs ({vis_plugin_start, vis_plugin_stop})),
    PluginParams ("general", MultiFuncs ({general_plugin_start, general_plugin_stop})),
    PluginParams ("interface", SingleFuncs ({iface_plugin_get_current, iface_plugin_set_current}))
};

static void start_single (PluginType type)
{
    PluginHandle * skip = nullptr;

    for (PluginHandle * p : aud_plugin_list (type))
    {
        if (! aud_plugin_get_enabled (p))
            continue;

        AUDINFO ("Starting selected %s plugin %s.\n", table[type].name,
         aud_plugin_get_name (p));

        if (table[type].f.s.set_current (p))
            return;

        AUDWARN ("%s failed to start.\n", aud_plugin_get_name (p));
        plugin_set_failed (p);
        plugin_set_enabled (p, false);
        skip = p;
        break;
    }

    AUDINFO ("Probing for %s plugin.\n", table[type].name);

    for (PluginHandle * p : aud_plugin_list (type))
    {
        if (p == skip)
            continue;

        AUDINFO ("Trying to start %s.\n", aud_plugin_get_name (p));

        if (! table[type].f.s.set_current (p))
        {
            AUDWARN ("%s failed to start.\n", aud_plugin_get_name (p));
            plugin_set_failed (p);
            continue;
        }

        plugin_set_enabled (p, true);
        return;
    }

    AUDERR ("No %s plugin found.\n"
     "(Did you forget to install audacious-plugins?)\n", table[type].name);
    abort ();
}

static void start_multi (PluginType type)
{
    for (PluginHandle * p : aud_plugin_list (type))
    {
        if (! aud_plugin_get_enabled (p))
            continue;

        AUDINFO ("Starting %s.\n", aud_plugin_get_name (p));

        if (! table[type].f.m.start (p))
        {
            AUDWARN ("%s failed to start.\n", aud_plugin_get_name (p));
            plugin_set_failed (p);
        }
    }
}

static void start_plugins (PluginType type)
{
    /* no interface plugin in headless mode */
    if (type == PluginType::Iface && aud_get_headless_mode ())
        return;

    if (table[type].is_single)
        start_single (type);
    else if (table[type].f.m.start)
        start_multi (type);
}

void start_plugins_one ()
{
    plugin_system_init ();

    start_plugins (PluginType::Transport);
    start_plugins (PluginType::Playlist);
    start_plugins (PluginType::Input);
    start_plugins (PluginType::Effect);
    start_plugins (PluginType::Output);
}

void start_plugins_two ()
{
    start_plugins (PluginType::Vis);
    start_plugins (PluginType::General);
    start_plugins (PluginType::Iface);
}

static void stop_plugins (PluginType type)
{
    if (table[type].is_single)
    {
        AUDINFO ("Shutting down %s.\n", aud_plugin_get_name
         (table[type].f.s.get_current ()));
        table[type].f.s.set_current (nullptr);
    }
    else if (table[type].f.m.stop)
    {
        for (PluginHandle * p : aud_plugin_list (type))
        {
            if (aud_plugin_get_enabled (p))
            {
                AUDINFO ("Shutting down %s.\n", aud_plugin_get_name (p));
                table[type].f.m.stop (p);
            }
        }
    }
}

void stop_plugins_two ()
{
    /* interface plugin is already shut down */
    stop_plugins (PluginType::General);
    stop_plugins (PluginType::Vis);
}

void stop_plugins_one ()
{
    stop_plugins (PluginType::Output);
    stop_plugins (PluginType::Effect);
    stop_plugins (PluginType::Input);
    stop_plugins (PluginType::Playlist);
    stop_plugins (PluginType::Transport);

    plugin_system_cleanup ();
}

EXPORT PluginHandle * aud_plugin_get_current (PluginType type)
{
    assert (table[type].is_single);
    return table[type].f.s.get_current ();
}

static bool enable_single (PluginType type, PluginHandle * p)
{
    PluginHandle * old = table[type].f.s.get_current ();

    AUDINFO ("Switching from %s to %s.\n", aud_plugin_get_name (old),
     aud_plugin_get_name (p));

    plugin_set_enabled (old, false);
    plugin_set_enabled (p, true);

    if (table[type].f.s.set_current (p))
    {
        // check that the switch was not queued for later
        if (table[type].f.s.get_current () == old)
        {
            plugin_set_enabled (p, false);
            plugin_set_enabled (old, true);
        }

        return true;
    }

    AUDERR ("%s failed to start; falling back to %s.\n",
     aud_plugin_get_name (p), aud_plugin_get_name (old));

    plugin_set_failed (p);
    plugin_set_enabled (p, false);
    plugin_set_enabled (old, true);

    if (table[type].f.s.set_current (old))
        return false;

    AUDERR ("%s failed to start.\n", aud_plugin_get_name (old));
    abort ();
}

static bool enable_multi (PluginType type, PluginHandle * p, bool enable)
{
    AUDINFO ("%sabling %s.\n", enable ? "En" : "Dis", aud_plugin_get_name (p));

    if (enable)
    {
        plugin_set_enabled (p, true);

        if (table[type].f.m.start && ! table[type].f.m.start (p))
        {
            AUDERR ("%s failed to start.\n", aud_plugin_get_name (p));
            plugin_set_failed (p);
            plugin_set_enabled (p, false);
            return false;
        }

        if (type == PluginType::Vis || type == PluginType::General)
            hook_call ("dock plugin enabled", p);
    }
    else
    {
        plugin_set_enabled (p, false);

        if (type == PluginType::Vis || type == PluginType::General)
            hook_call ("dock plugin disabled", p);

        if (table[type].f.m.stop)
            table[type].f.m.stop (p);
    }

    return true;
}

EXPORT bool aud_plugin_enable (PluginHandle * plugin, bool enable)
{
    if (! enable == ! aud_plugin_get_enabled (plugin))
        return true;

    PluginType type = aud_plugin_get_type (plugin);

    if (table[type].is_single)
    {
        assert (enable);
        return enable_single (type, plugin);
    }

    return enable_multi (type, plugin, enable);
}

/* Miscellaneous plugin-related functions ... */

EXPORT int aud_plugin_send_message (PluginHandle * plugin, const char * code, const void * data, int size)
{
    if (! aud_plugin_get_enabled (plugin))
        return -1;

    Plugin * header = (Plugin *) aud_plugin_get_header (plugin);
    if (! header)
        return -1;

    return header->take_message (code, data, size);
}

EXPORT void * aud_plugin_get_gtk_widget (PluginHandle * plugin)
{
    if (! aud_plugin_get_enabled (plugin))
        return nullptr;

    PluginType type = aud_plugin_get_type (plugin);
    if (type != PluginType::General && type != PluginType::Vis)
        return nullptr;

    auto dp = (DockablePlugin *) aud_plugin_get_header (plugin);
    return dp ? dp->get_gtk_widget () : nullptr;
}

EXPORT void * aud_plugin_get_qt_widget (PluginHandle * plugin)
{
    if (! aud_plugin_get_enabled (plugin))
        return nullptr;

    PluginType type = aud_plugin_get_type (plugin);
    if (type != PluginType::General && type != PluginType::Vis)
        return nullptr;

    auto dp = (DockablePlugin *) aud_plugin_get_header (plugin);
    return dp ? dp->get_qt_widget () : nullptr;
}
