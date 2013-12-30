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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "debug.h"
#include "effect.h"
#include "general.h"
#include "interface.h"
#include "misc.h"
#include "output.h"
#include "plugin.h"
#include "plugins.h"
#include "ui_preferences.h"
#include "visualization.h"

static const struct {
    const char * name;
    bool_t is_single;

    union {
        struct {
            bool_t (* start) (PluginHandle * plugin);
            void (* stop) (PluginHandle * plugin);
        } m;

        struct {
            PluginHandle * (* probe) (void);
            PluginHandle * (* get_current) (void);
            bool_t (* set_current) (PluginHandle * plugin);
        } s;
    } u;
} table[PLUGIN_TYPES] = {
 [PLUGIN_TYPE_TRANSPORT] = {"transport",  FALSE, .u.m = {NULL, NULL}},
 [PLUGIN_TYPE_PLAYLIST] = {"playlist",  FALSE, .u.m = {NULL, NULL}},
 [PLUGIN_TYPE_INPUT] = {"input", FALSE, .u.m = {NULL, NULL}},
 [PLUGIN_TYPE_EFFECT] = {"effect", FALSE, .u.m = {effect_plugin_start, effect_plugin_stop}},
 [PLUGIN_TYPE_OUTPUT] = {"output", TRUE, .u.s = {output_plugin_probe,
  output_plugin_get_current, output_plugin_set_current}},
 [PLUGIN_TYPE_VIS] = {"visualization", FALSE, .u.m = {vis_plugin_start, vis_plugin_stop}},
 [PLUGIN_TYPE_GENERAL] = {"general", FALSE, .u.m = {general_plugin_start, general_plugin_stop}},
 [PLUGIN_TYPE_IFACE] = {"interface", TRUE, .u.s = {iface_plugin_probe,
  iface_plugin_get_current, iface_plugin_set_current}}};

static bool_t find_enabled_cb (PluginHandle * p, void * pp)
{
    * (PluginHandle * *) pp = p;
    return FALSE;
}

static PluginHandle * find_enabled (int type)
{
    PluginHandle * p = NULL;
    plugin_for_enabled (type, find_enabled_cb, & p);
    return p;
}

static void start_single (int type)
{
    PluginHandle * p;

    if ((p = find_enabled (type)) != NULL)
    {
        AUDDBG ("Starting selected %s plugin %s.\n", table[type].name,
         plugin_get_name (p));

        if (table[type].u.s.set_current (p))
            return;

        AUDDBG ("%s failed to start.\n", plugin_get_name (p));
        plugin_set_enabled (p, FALSE);
    }

    AUDDBG ("Probing for %s plugin.\n", table[type].name);

    if ((p = table[type].u.s.probe ()) == NULL)
    {
        fprintf (stderr, "FATAL: No %s plugin found.\n", table[type].name);
        exit (EXIT_FAILURE);
    }

    AUDDBG ("Starting %s.\n", plugin_get_name (p));
    plugin_set_enabled (p, TRUE);

    if (! table[type].u.s.set_current (p))
    {
        fprintf (stderr, "FATAL: %s failed to start.\n", plugin_get_name (p));
        plugin_set_enabled (p, FALSE);
        exit (EXIT_FAILURE);
    }
}

static bool_t start_multi_cb (PluginHandle * p, void * type)
{
    AUDDBG ("Starting %s.\n", plugin_get_name (p));

    if (! table[GPOINTER_TO_INT (type)].u.m.start (p))
    {
        AUDDBG ("%s failed to start; disabling.\n", plugin_get_name (p));
        plugin_set_enabled (p, FALSE);
    }

    return TRUE;
}

static void start_plugins (int type)
{
    if (type == PLUGIN_TYPE_IFACE && headless_mode ())
        return;

    if (table[type].is_single)
        start_single (type);
    else
    {
        if (table[type].u.m.start)
            plugin_for_enabled (type, start_multi_cb, GINT_TO_POINTER (type));
    }
}

static VFSConstructor * lookup_transport (const char * scheme)
{
    PluginHandle * plugin = transport_plugin_for_scheme (scheme);
    if (! plugin)
        return NULL;

    TransportPlugin * tp = plugin_get_header (plugin);
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

static bool_t misc_cleanup_cb (PluginHandle * p, void * unused)
{
    plugin_misc_cleanup (p);
    return TRUE;
}

static bool_t stop_multi_cb (PluginHandle * p, void * type)
{
    AUDDBG ("Shutting down %s.\n", plugin_get_name (p));
    table[GPOINTER_TO_INT (type)].u.m.stop (p);
    return TRUE;
}

static void stop_plugins (int type)
{
    if (type == PLUGIN_TYPE_IFACE && headless_mode ())
        return;

    plugin_for_enabled (type, misc_cleanup_cb, GINT_TO_POINTER (type));

    if (table[type].is_single)
    {
        AUDDBG ("Shutting down %s.\n", plugin_get_name
         (table[type].u.s.get_current ()));
        table[type].u.s.set_current (NULL);
    }
    else
    {
        if (table[type].u.m.stop)
            plugin_for_enabled (type, stop_multi_cb, GINT_TO_POINTER (type));
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

PluginHandle * plugin_get_current (int type)
{
    g_return_val_if_fail (table[type].is_single, NULL);
    return table[type].u.s.get_current ();
}

static bool_t enable_single (int type, PluginHandle * p)
{
    PluginHandle * old = table[type].u.s.get_current ();

    plugin_misc_cleanup (old);

    AUDDBG ("Switching from %s to %s.\n", plugin_get_name (old),
     plugin_get_name (p));
    plugin_set_enabled (old, FALSE);
    plugin_set_enabled (p, TRUE);

    if (table[type].u.s.set_current (p))
        return TRUE;

    fprintf (stderr, "%s failed to start; falling back to %s.\n",
     plugin_get_name (p), plugin_get_name (old));
    plugin_set_enabled (p, FALSE);
    plugin_set_enabled (old, TRUE);

    if (table[type].u.s.set_current (old))
        return FALSE;

    fprintf (stderr, "FATAL: %s failed to start.\n", plugin_get_name (old));
    plugin_set_enabled (old, FALSE);
    exit (EXIT_FAILURE);
}

static bool_t enable_multi (int type, PluginHandle * p, bool_t enable)
{
    if (! enable)
        plugin_misc_cleanup (p);

    AUDDBG ("%sabling %s.\n", enable ? "En" : "Dis", plugin_get_name (p));
    plugin_set_enabled (p, enable);

    if (enable)
    {
        if (table[type].u.m.start && ! table[type].u.m.start (p))
        {
            fprintf (stderr, "%s failed to start.\n", plugin_get_name (p));
            plugin_set_enabled (p, FALSE);
            return FALSE;
        }
    }
    else
    {
        if (table[type].u.m.stop)
            table[type].u.m.stop (p);
    }

    return TRUE;
}

bool_t plugin_enable (PluginHandle * plugin, bool_t enable)
{
    if (! enable == ! plugin_get_enabled (plugin))
        return TRUE;

    int type = plugin_get_type (plugin);

    if (table[type].is_single)
    {
        g_return_val_if_fail (enable, FALSE);
        return enable_single (type, plugin);
    }

    return enable_multi (type, plugin, enable);
}

/* Miscellaneous plugin-related functions ... */

PluginHandle * plugin_by_widget (/* GtkWidget * */ void * widget)
{
    PluginHandle * p;
    if ((p = vis_plugin_by_widget (widget)))
        return p;
    if ((p = general_plugin_by_widget (widget)))
        return p;
    return NULL;
}

int plugin_send_message (PluginHandle * plugin, const char * code, const void * data, int size)
{
    if (! plugin_get_enabled (plugin))
        return ENOSYS;

    Plugin * header = plugin_get_header (plugin);
    if (! header || ! PLUGIN_HAS_FUNC (header, take_message))
        return ENOSYS;

    return header->take_message (code, data, size);
}

void plugin_do_about (PluginHandle * plugin)
{
    g_return_if_fail (plugin_get_enabled (plugin));
    Plugin * header = plugin_get_header (plugin);
    g_return_if_fail (header);

    if (PLUGIN_HAS_FUNC (header, about))
        header->about ();
    else if (PLUGIN_HAS_FUNC (header, about_text))
        plugin_make_about_window (plugin);
}

void plugin_do_configure (PluginHandle * plugin)
{
    g_return_if_fail (plugin_get_enabled (plugin));
    Plugin * header = plugin_get_header (plugin);
    g_return_if_fail (header);

    if (PLUGIN_HAS_FUNC (header, configure))
        header->configure ();
    else if (PLUGIN_HAS_FUNC (header, prefs))
        plugin_make_config_window (plugin);
}
