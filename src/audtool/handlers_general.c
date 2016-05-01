/*
 * handlers_general.c
 * Copyright 2005-2013 George Averill, William Pitcock, Giacomo Lozito,
 *                     Matti Hämäläinen, and John Lindgren
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

#include <stdlib.h>

#include "audtool.h"
#include "wrappers.h"

static int get_main_volume (void)
{
    int left = 0, right = 0;
    obj_audacious_call_volume_sync (dbus_proxy, & left, & right, NULL, NULL);
    return MAX (left, right);
}

void get_volume (int argc, char * * argv)
{
    audtool_report ("%d", get_main_volume ());
}

void set_volume (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<level>");
        exit (1);
    }

    int vol = atoi (argv[1]);

    switch (argv[1][0])
    {
    case '+':
    case '-':
        vol += get_main_volume ();
        break;
    }

    obj_audacious_call_set_volume_sync (dbus_proxy, vol, vol, NULL, NULL);
}

void mainwin_show (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_show_main_win_sync);
}

void show_preferences_window (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_show_prefs_box_sync);
}

void show_about_window (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_show_about_box_sync);
}

void show_jtf_window (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_show_jtf_box_sync);
}

void show_filebrowser (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_show_filebrowser_sync);
}

void shutdown_audacious_server (int argc, char * * argv)
{
    obj_audacious_call_quit_sync (dbus_proxy, NULL, NULL);
}

void get_handlers_list (int argc, char * * argv)
{
    audtool_report ("Usage: audtool [-#] COMMAND ...");
    audtool_report ("       where # (1-9) selects the instance of Audacious to control");
    audtool_report ("");

    for (int i = 0; handlers[i].name; i ++)
    {
        if (! g_ascii_strcasecmp ("<sep>", handlers[i].name))
            audtool_report ("%s%s:", i == 0 ? "" : "\n", handlers[i].desc);
        else
            audtool_report ("   %-34s - %s", handlers[i].name, handlers[i].desc);
    }

    audtool_report ("");
    audtool_report ("Commands may be prefixed with '--' (GNU-style long options) or not, your choice.");
    audtool_report ("Show/hide and enable/disable commands take an optional 'on' or 'off' argument.");
    audtool_report ("Report bugs to http://redmine.audacious-media-player.org/projects/audacious");
}

void get_version (int argc, char * * argv)
{
    char * version = NULL;
    obj_audacious_call_version_sync (dbus_proxy, & version, NULL, NULL);

    if (! version)
        exit (1);

    audtool_report ("Audacious %s", version);
    g_free (version);
}

void plugin_is_enabled (int argc, char * * argv)
{
    if (argc != 2)
    {
        audtool_whine_args (argv[0], "<plugin>");
        exit (1);
    }

    gboolean enabled = FALSE;
    obj_audacious_call_plugin_is_enabled_sync (dbus_proxy, argv[1], & enabled, NULL, NULL);

    exit (! enabled);
}

void plugin_enable (int argc, char * * argv)
{
    gboolean enable = TRUE;

    if (argc == 2)
        enable = TRUE;
    else if (argc == 3 && ! g_ascii_strcasecmp (argv[2], "on"))
        enable = TRUE;
    else if (argc == 3 && ! g_ascii_strcasecmp (argv[2], "off"))
        enable = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<plugin> <on/off>");
        exit (1);
    }

    obj_audacious_call_plugin_enable_sync (dbus_proxy, argv[1], enable, NULL, NULL);
}
