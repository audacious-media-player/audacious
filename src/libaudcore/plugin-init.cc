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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "hook.h"
#include "interface.h"
#include "internal.h"
#include "output.h"
#include "plugin.h"
#include "runtime.h"

static bool_t general_plugin_start (PluginHandle * plugin)
{
    GeneralPlugin * gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
    g_return_val_if_fail (gp, FALSE);

    if (gp->init && ! gp->init ())
        return FALSE;

    return TRUE;
}

void general_plugin_stop (PluginHandle * plugin)
{
    GeneralPlugin * gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
    g_return_if_fail (gp);

    if (gp->cleanup)
        gp->cleanup ();
}

struct MultiFuncs
{
    bool_t (* start) (PluginHandle * plugin);
    void (* stop) (PluginHandle * plugin);
};

struct SingleFuncs
{
    PluginHandle * (* probe) (void);
    PluginHandle * (* get_current) (void);
    bool_t (* set_current) (PluginHandle * plugin);
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
    bool_t is_single;
    PluginFuncs f;

    constexpr PluginParams (const char * name, MultiFuncs multi) :
        name (name), is_single (FALSE), f (multi) {}
    constexpr PluginParams (const char * name, SingleFuncs single) :
        name (name), is_single (TRUE), f (single) {}
};

static const PluginParams table[PLUGIN_TYPES] = {
    PluginParams ("transport", MultiFuncs ({NULL, NULL})),
    PluginParams ("playlist", MultiFuncs ({NULL, NULL})),
    PluginParams ("input", MultiFuncs ({NULL, NULL})),
    PluginParams ("effect", MultiFuncs ({effect_plugin_start, effect_plugin_stop})),
    PluginParams ("output", SingleFuncs ({output_plugin_probe,
     output_plugin_get_current, output_plugin_set_current})),
    PluginParams ("visualization", MultiFuncs ({vis_plugin_start, vis_plugin_stop})),
    PluginParams ("general", MultiFuncs ({general_plugin_start, general_plugin_stop})),
    PluginParams ("interface", SingleFuncs ({iface_plugin_probe,
     iface_plugin_get_current, iface_plugin_set_current}))
};

static bool_t find_enabled_cb (PluginHandle * p, void * pp)
{
    * (PluginHandle * *) pp = p;
    return FALSE;
}

static PluginHandle * find_enabled (int type)
{
    PluginHandle * p = NULL;
    aud_plugin_for_enabled (type, find_enabled_cb, & p);
    return p;
}

static void start_single (int type)
{
    PluginHandle * p;

    if ((p = find_enabled (type)) != NULL)
    {
        AUDDBG ("Starting selected %s plugin %s.\n", table[type].name,
         aud_plugin_get_name (p));

        if (table[type].f.s.set_current (p))
            return;

        AUDDBG ("%s failed to start.\n", aud_plugin_get_name (p));
        plugin_set_enabled (p, FALSE);
    }

    AUDDBG ("Probing for %s plugin.\n", table[type].name);

    if ((p = table[type].f.s.probe ()) == NULL)
    {
        fprintf (stderr, "FATAL: No %s plugin found.\n", table[type].name);
        abort ();
    }

    AUDDBG ("Starting %s.\n", aud_plugin_get_name (p));
    plugin_set_enabled (p, TRUE);

    if (! table[type].f.s.set_current (p))
    {
        fprintf (stderr, "FATAL: %s failed to start.\n", aud_plugin_get_name (p));
        abort ();
    }
}

static bool_t start_multi_cb (PluginHandle * p, void * type)
{
    AUDDBG ("Starting %s.\n", aud_plugin_get_name (p));

    if (! table[GPOINTER_TO_INT (type)].f.m.start (p))
    {
        AUDDBG ("%s failed to start; disabling.\n", aud_plugin_get_name (p));
        plugin_set_enabled (p, FALSE);
    }

    return TRUE;
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
            aud_plugin_for_enabled (type, start_multi_cb, GINT_TO_POINTER (type));
    }
}

static const VFSConstructor * lookup_transport (const char * scheme)
{
    PluginHandle * plugin = transport_plugin_for_scheme (scheme);
    if (! plugin)
        return NULL;

    TransportPlugin * tp = (TransportPlugin *) aud_plugin_get_header (plugin);
    return tp ? tp->vtable : NULL;
}

void start_plugins_one (void)
{
    plugin_system_init ();
    vfs_set_lookup_func (lookup_transport);

    for (int i = 0; i < PLUGIN_TYPE_GENERAL; i ++)
        start_plugins (i);
}

void start_plugins_two (void)
{
    for (int i = PLUGIN_TYPE_GENERAL; i < PLUGIN_TYPES; i ++)
        start_plugins (i);
}

static bool_t stop_multi_cb (PluginHandle * p, void * type)
{
    AUDDBG ("Shutting down %s.\n", aud_plugin_get_name (p));
    table[GPOINTER_TO_INT (type)].f.m.stop (p);
    return TRUE;
}

static void stop_plugins (int type)
{
    /* interface plugin is already shut down */
    if (type == PLUGIN_TYPE_IFACE)
        return;

    if (table[type].is_single)
    {
        AUDDBG ("Shutting down %s.\n", aud_plugin_get_name
         (table[type].f.s.get_current ()));
        table[type].f.s.set_current (NULL);
    }
    else
    {
        if (table[type].f.m.stop)
            aud_plugin_for_enabled (type, stop_multi_cb, GINT_TO_POINTER (type));
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

    vfs_set_lookup_func (NULL);
    plugin_system_cleanup ();
}

EXPORT PluginHandle * aud_plugin_get_current (int type)
{
    g_return_val_if_fail (table[type].is_single, NULL);
    return table[type].f.s.get_current ();
}

static bool_t enable_single (int type, PluginHandle * p)
{
    PluginHandle * old = table[type].f.s.get_current ();

    AUDDBG ("Switching from %s to %s.\n", aud_plugin_get_name (old),
     aud_plugin_get_name (p));
    plugin_set_enabled (old, FALSE);
    plugin_set_enabled (p, TRUE);

    if (table[type].f.s.set_current (p))
        return TRUE;

    fprintf (stderr, "%s failed to start; falling back to %s.\n",
     aud_plugin_get_name (p), aud_plugin_get_name (old));
    plugin_set_enabled (p, FALSE);
    plugin_set_enabled (old, TRUE);

    if (table[type].f.s.set_current (old))
        return FALSE;

    fprintf (stderr, "FATAL: %s failed to start.\n", aud_plugin_get_name (old));
    abort ();
}

static bool_t enable_multi (int type, PluginHandle * p, bool_t enable)
{
    AUDDBG ("%sabling %s.\n", enable ? "En" : "Dis", aud_plugin_get_name (p));
    plugin_set_enabled (p, enable);

    if (enable)
    {
        if (table[type].f.m.start && ! table[type].f.m.start (p))
        {
            fprintf (stderr, "%s failed to start.\n", aud_plugin_get_name (p));
            plugin_set_enabled (p, FALSE);
            return FALSE;
        }

        if (type == PLUGIN_TYPE_VIS || type == PLUGIN_TYPE_GENERAL)
            hook_call ("dock plugin enabled", p);
    }
    else
    {
        if (type == PLUGIN_TYPE_VIS || type == PLUGIN_TYPE_GENERAL)
            hook_call ("dock plugin disabled", p);

        if (table[type].f.m.stop)
            table[type].f.m.stop (p);
    }

    return TRUE;
}

EXPORT bool_t aud_plugin_enable (PluginHandle * plugin, bool_t enable)
{
    if (! enable == ! aud_plugin_get_enabled (plugin))
        return TRUE;

    int type = aud_plugin_get_type (plugin);

    if (table[type].is_single)
    {
        g_return_val_if_fail (enable, FALSE);
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
    g_return_val_if_fail (aud_plugin_get_enabled (plugin), NULL);
    int type = aud_plugin_get_type (plugin);

    if (type == PLUGIN_TYPE_GENERAL)
    {
        GeneralPlugin * gp = (GeneralPlugin *) aud_plugin_get_header (plugin);
        g_return_val_if_fail (gp != NULL, NULL);

        if (PLUGIN_HAS_FUNC (gp, get_widget))
            return gp->get_widget ();
    }

    if (type == PLUGIN_TYPE_VIS)
    {
        VisPlugin * vp = (VisPlugin *) aud_plugin_get_header (plugin);
        g_return_val_if_fail (vp != NULL, NULL);

        if (PLUGIN_HAS_FUNC (vp, get_widget))
            return vp->get_widget ();
    }

    return NULL;
}
