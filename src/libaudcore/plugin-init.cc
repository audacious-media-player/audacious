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
#include <errno.h>
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
    if (! gp)
        return false;

    if (gp->init && ! gp->init ())
        return false;

    return true;
}

void general_plugin_stop (PluginHandle * plugin)
{
    GeneralPlugin * gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
    if (! gp)
        return;

    if (gp->cleanup)
        gp->cleanup ();
}

struct MultiFuncs
{
    bool (* start) (PluginHandle * plugin);
    void (* stop) (PluginHandle * plugin);
};

struct SingleFuncs
{
    PluginHandle * (* get_current) (void);
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

static const PluginParams table[PLUGIN_TYPES] = {
    PluginParams ("transport", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("playlist", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("input", MultiFuncs ({nullptr, nullptr})),
    PluginParams ("effect", MultiFuncs ({effect_plugin_start, effect_plugin_stop})),
    PluginParams ("output", SingleFuncs ({output_plugin_get_current, output_plugin_set_current})),
    PluginParams ("visualization", MultiFuncs ({vis_plugin_start, vis_plugin_stop})),
    PluginParams ("general", MultiFuncs ({general_plugin_start, general_plugin_stop})),
    PluginParams ("interface", SingleFuncs ({iface_plugin_get_current, iface_plugin_set_current}))
};

static bool find_enabled_cb (PluginHandle * p, void * pp)
{
    * (PluginHandle * *) pp = p;
    return false;
}

static PluginHandle * find_enabled (int type)
{
    PluginHandle * p = nullptr;
    aud_plugin_for_enabled (type, find_enabled_cb, & p);
    return p;
}

static bool probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    int type = aud_plugin_get_type (p);

    AUDINFO ("Trying to start %s.\n", aud_plugin_get_name (p));

    if (! table[type].f.s.set_current (p))
    {
        AUDWARN ("%s failed to start.\n", aud_plugin_get_name (p));
        return true; /* keep searching */
    }

    * pp = p;
    plugin_set_enabled (p, true);
    return false; /* stop searching */
}

static void start_single (int type)
{
    PluginHandle * p;

    if ((p = find_enabled (type)) != nullptr)
    {
        AUDINFO ("Starting selected %s plugin %s.\n", table[type].name,
         aud_plugin_get_name (p));

        if (table[type].f.s.set_current (p))
            return;

        AUDWARN ("%s failed to start.\n", aud_plugin_get_name (p));
        plugin_set_enabled (p, false);
    }

    AUDINFO ("Probing for %s plugin.\n", table[type].name);
    aud_plugin_for_each (type, (PluginForEachFunc) probe_cb, & p);

    if (! p)
    {
        AUDERR ("No %s plugin found.\n"
         "(Did you forget to install audacious-plugins?)\n", table[type].name);
        abort ();
    }
}

static bool start_multi_cb (PluginHandle * p, void * data)
{
    AUDINFO ("Starting %s.\n", aud_plugin_get_name (p));

    if (! ((PluginParams *) data)->f.m.start (p))
    {
        AUDWARN ("%s failed to start; disabling.\n", aud_plugin_get_name (p));
        plugin_set_enabled (p, false);
    }

    return true;
}

static void start_plugins (int type)
{
    /* no interface plugin in headless mode */
    if (type == PLUGIN_TYPE_IFACE && aud_get_headless_mode ())
        return;

    if (table[type].is_single)
        start_single (type);
    else
    {
        if (table[type].f.m.start)
            aud_plugin_for_enabled (type, start_multi_cb, (void *) & table[type]);
    }
}

static VFSFile::OpenFunc lookup_transport (const char * scheme)
{
    PluginHandle * plugin = transport_plugin_for_scheme (scheme);
    if (! plugin)
        return nullptr;

    TransportPlugin * tp = (TransportPlugin *) aud_plugin_get_header (plugin);
    return tp ? tp->fopen_impl : nullptr;
}

void start_plugins_one (void)
{
    plugin_system_init ();
    VFSFile::set_lookup_func (lookup_transport);

    for (int i = 0; i < PLUGIN_TYPE_GENERAL; i ++)
        start_plugins (i);
}

void start_plugins_two (void)
{
    for (int i = PLUGIN_TYPE_GENERAL; i < PLUGIN_TYPES; i ++)
        start_plugins (i);
}

static bool stop_multi_cb (PluginHandle * p, void * data)
{
    AUDINFO ("Shutting down %s.\n", aud_plugin_get_name (p));
    ((PluginParams *) data)->f.m.stop (p);
    return true;
}

static void stop_plugins (int type)
{
    /* interface plugin is already shut down */
    if (type == PLUGIN_TYPE_IFACE)
        return;

    if (table[type].is_single)
    {
        AUDINFO ("Shutting down %s.\n", aud_plugin_get_name
         (table[type].f.s.get_current ()));
        table[type].f.s.set_current (nullptr);
    }
    else
    {
        if (table[type].f.m.stop)
            aud_plugin_for_enabled (type, stop_multi_cb, (void *) & table[type]);
    }
}

void stop_plugins_two (void)
{
    for (int i = PLUGIN_TYPES - 1; i >= PLUGIN_TYPE_GENERAL; i --)
        stop_plugins (i);
}

void stop_plugins_one (void)
{
    for (int i = PLUGIN_TYPE_GENERAL - 1; i >= 0; i --)
        stop_plugins (i);

    VFSFile::set_lookup_func (nullptr);
    plugin_system_cleanup ();
}

EXPORT PluginHandle * aud_plugin_get_current (int type)
{
    assert (table[type].is_single);
    return table[type].f.s.get_current ();
}

static bool enable_single (int type, PluginHandle * p)
{
    PluginHandle * old = table[type].f.s.get_current ();

    AUDINFO ("Switching from %s to %s.\n", aud_plugin_get_name (old),
     aud_plugin_get_name (p));

    if (table[type].f.s.set_current (p))
    {
        // check that the switch was not queued for later
        if (table[type].f.s.get_current () == p)
        {
            plugin_set_enabled (old, false);
            plugin_set_enabled (p, true);
        }

        return true;
    }

    AUDERR ("%s failed to start; falling back to %s.\n",
     aud_plugin_get_name (p), aud_plugin_get_name (old));

    if (table[type].f.s.set_current (old))
        return false;

    AUDERR ("%s failed to start.\n", aud_plugin_get_name (old));
    abort ();
}

static bool enable_multi (int type, PluginHandle * p, bool enable)
{
    AUDINFO ("%sabling %s.\n", enable ? "En" : "Dis", aud_plugin_get_name (p));

    if (enable)
    {
        if (table[type].f.m.start && ! table[type].f.m.start (p))
        {
            AUDERR ("%s failed to start.\n", aud_plugin_get_name (p));
            return false;
        }

        plugin_set_enabled (p, true);

        if (type == PLUGIN_TYPE_VIS || type == PLUGIN_TYPE_GENERAL)
            hook_call ("dock plugin enabled", p);
    }
    else
    {
        plugin_set_enabled (p, false);

        if (type == PLUGIN_TYPE_VIS || type == PLUGIN_TYPE_GENERAL)
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

    int type = aud_plugin_get_type (plugin);

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
        return ENOSYS;

    Plugin * header = (Plugin *) aud_plugin_get_header (plugin);
    if (! header || ! PLUGIN_HAS_FUNC (header, take_message))
        return ENOSYS;

    return header->take_message (code, data, size);
}

EXPORT void * aud_plugin_get_widget (PluginHandle * plugin)
{
    if (! aud_plugin_get_enabled (plugin))
        return nullptr;

    int type = aud_plugin_get_type (plugin);

    if (type == PLUGIN_TYPE_GENERAL)
    {
        auto gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
        if (gp && PLUGIN_HAS_FUNC (gp, get_widget))
            return gp->get_widget ();
    }

    if (type == PLUGIN_TYPE_VIS)
    {
        auto vp = (VisPlugin *) aud_plugin_get_header (plugin);
        if (vp && PLUGIN_HAS_FUNC (vp, get_widget))
            return vp->get_widget ();
    }

    return nullptr;
}
