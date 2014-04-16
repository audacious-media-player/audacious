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

#include <glib.h>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "drct.h"
#include "interface.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"
#include "visualization.h"

typedef struct {
    const char * name;
    const char * icon;
    MenuFunc func;
} MenuItem;

static PluginHandle * current_plugin = NULL;
static PluginHandle * next_plugin = NULL;

static GMainLoop * mainloop = NULL;
static IfacePlugin * current_interface = NULL;

static GList * menu_items[AUD_MENU_COUNT]; /* of MenuItem */

static void add_menu_items (void)
{
    if (! PLUGIN_HAS_FUNC (current_interface, plugin_menu_add))
        return;

    for (int id = 0; id < AUD_MENU_COUNT; id ++)
    {
        for (GList * node = menu_items[id]; node; node = node->next)
        {
            MenuItem * item = node->data;
            current_interface->plugin_menu_add (id, item->func, item->name, item->icon);
        }
    }
}

static void remove_menu_items (void)
{
    if (! PLUGIN_HAS_FUNC (current_interface, plugin_menu_remove))
        return;

    for (int id = 0; id < AUD_MENU_COUNT; id ++)
    {
        for (GList * node = menu_items[id]; node; node = node->next)
        {
            MenuItem * item = node->data;
            current_interface->plugin_menu_remove (id, item->func);
        }
    }
}

static bool_t interface_load (PluginHandle * plugin)
{
    IfacePlugin * i = plugin_get_header (plugin);
    g_return_val_if_fail (i, FALSE);

    AUDDBG ("Loading %s.\n", plugin_get_name (plugin));

    if (PLUGIN_HAS_FUNC (i, init) && ! i->init ())
        return FALSE;

    current_plugin = plugin;
    current_interface = i;

    add_menu_items ();

    if (PLUGIN_HAS_FUNC (current_interface, show) && aud_get_bool (NULL, "show_interface"))
        current_interface->show (TRUE);

    return TRUE;
}

static void interface_unload (void)
{
    g_return_if_fail (current_plugin && current_interface);

    AUDDBG ("Unloading %s.\n", plugin_get_name (current_plugin));

    if (PLUGIN_HAS_FUNC (current_interface, show) && aud_get_bool (NULL, "show_interface"))
        current_interface->show (FALSE);

    remove_menu_items ();

    if (PLUGIN_HAS_FUNC (current_interface, cleanup))
        current_interface->cleanup ();

    current_plugin = NULL;
    current_interface = NULL;
}

void ui_show (bool_t show)
{
    if (! current_interface)
        return;

    aud_set_bool (NULL, "show_interface", show);

    if (PLUGIN_HAS_FUNC (current_interface, show))
        current_interface->show (show);

    vis_activate (show);
}

bool_t ui_is_shown (void)
{
    if (! current_interface)
        return FALSE;

    return aud_get_bool (NULL, "show_interface");
}

void ui_show_error (const char * message)
{
    if (aud_get_headless_mode ())
        fprintf (stderr, "ERROR: %s\n", message);
    else
        event_queue_full (0, "ui show error", str_get (message), (GDestroyNotify) str_unref);
}

static bool_t probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    * pp = p;  /* just pick the first one */
    return FALSE;
}

PluginHandle * iface_plugin_probe (void)
{
    PluginHandle * p = NULL;
    plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) probe_cb, & p);
    return p;
}

PluginHandle * iface_plugin_get_current (void)
{
    return current_plugin;
}

bool_t iface_plugin_set_current (PluginHandle * plugin)
{
    next_plugin = plugin;

    /* restart main loop, if running */
    if (current_interface || mainloop)
        drct_quit ();

    return TRUE;
}

static void run_glib_mainloop (void)
{
    mainloop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (mainloop);
    g_main_loop_unref (mainloop);
    mainloop = NULL;
}

static void run_iface_plugins (void)
{
    vis_activate (aud_get_bool (NULL, "show_interface"));

    while (next_plugin)
    {
        if (! interface_load (next_plugin))
            return;

        next_plugin = NULL;

        if (PLUGIN_HAS_FUNC (current_interface, run))
            current_interface->run ();
        else
            run_glib_mainloop ();

        /* tell interface to save layout */
        hook_call ("config save", NULL);

        interface_unload ();
    }
}

void iface_run (void)
{
    if (aud_get_headless_mode ())
        run_glib_mainloop ();
    else
        run_iface_plugins ();
}

void drct_quit (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, quit))
        current_interface->quit ();
    else
    {
        g_return_if_fail (mainloop);
        g_main_loop_quit (mainloop);
    }
}

void plugin_menu_add (int id, MenuFunc func, const char * name, const char * icon)
{
    g_return_if_fail (id >= 0 && id < AUD_MENU_COUNT);

    MenuItem * item = g_slice_new (MenuItem);
    item->name = name;
    item->icon = icon;
    item->func = func;

    menu_items[id] = g_list_append (menu_items[id], item);

    if (current_interface && PLUGIN_HAS_FUNC (current_interface, plugin_menu_add))
        current_interface->plugin_menu_add (id, func, name, icon);
}

void plugin_menu_remove (int id, MenuFunc func)
{
    g_return_if_fail (id >= 0 && id < AUD_MENU_COUNT);

    if (current_interface && PLUGIN_HAS_FUNC (current_interface, plugin_menu_remove))
        current_interface->plugin_menu_remove (id, func);

    GList * next;
    for (GList * node = menu_items[id]; node; node = next)
    {
        MenuItem * item = node->data;
        next = node->next;

        if (item->func == func)
        {
            menu_items[id] = g_list_delete_link (menu_items[id], node);
            g_slice_free (MenuItem, item);
        }
    }
}

void ui_show_about_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_about_window))
        current_interface->show_about_window ();
}

void ui_hide_about_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_about_window))
        current_interface->hide_about_window ();
}

void ui_show_filebrowser (bool_t open)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_filebrowser))
        current_interface->show_filebrowser (open);
}

void ui_hide_filebrowser (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_filebrowser))
        current_interface->hide_filebrowser ();
}

void ui_show_jump_to_song (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_jump_to_song))
        current_interface->show_jump_to_song ();
}

void ui_hide_jump_to_song (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_jump_to_song))
        current_interface->hide_jump_to_song ();
}

void ui_show_prefs_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_prefs_window))
        current_interface->show_prefs_window ();
}

void ui_hide_prefs_window (void)
{
    if (current_interface && PLUGIN_HAS_FUNC (current_interface, hide_prefs_window))
        current_interface->hide_prefs_window ();
}
