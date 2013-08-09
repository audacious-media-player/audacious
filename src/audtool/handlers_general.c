/*
 * handlers_general.c
 * Copyright 2005-2008 George Averill, William Pitcock, Giacomo Lozito, and
 *                     Matti Hämäläinen
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
#include <string.h>
#include <glib.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void get_volume (int argc, char * * argv)
{
    int i;

    i = audacious_remote_get_main_volume (dbus_proxy);

    audtool_report ("%d", i);
}

void set_volume (int argc, char * * argv)
{
    int i, current_volume;

    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<level>", argv[0]);
        exit (1);
    }

    current_volume = audacious_remote_get_main_volume (dbus_proxy);

    switch (argv[1][0])
    {
    case '+':
    case '-':
        i = current_volume + atoi (argv[1]);
        break;

    default:
        i = atoi (argv[1]);
        break;
    }

    audacious_remote_set_main_volume (dbus_proxy, i);
}

void mainwin_show (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        audacious_remote_main_win_toggle (dbus_proxy, TRUE);
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        audacious_remote_main_win_toggle (dbus_proxy, FALSE);
}

void show_preferences_window (int argc, char * * argv)
{
    gboolean show = TRUE;

    if (argc < 2)
    {
        audacious_remote_toggle_prefs_box (dbus_proxy, show);
        return;
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        show = TRUE;
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        show = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    audacious_remote_toggle_prefs_box (dbus_proxy, show);
}

void show_about_window (int argc, char * * argv)
{
    gboolean show = TRUE;

    if (argc < 2)
    {
        audacious_remote_toggle_about_box (dbus_proxy, show);
        return;
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        show = TRUE;
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        show = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    audacious_remote_toggle_about_box (dbus_proxy, show);
}

void show_jtf_window (int argc, char * * argv)
{
    gboolean show = TRUE;

    if (argc < 2)
    {
        audacious_remote_toggle_jtf_box (dbus_proxy, show);
        return;
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        show = TRUE;
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        show = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    audacious_remote_toggle_jtf_box (dbus_proxy, show);
}

void show_filebrowser (int argc, char * * argv)
{
    gboolean show = TRUE;

    if (argc < 2)
    {
        audacious_remote_toggle_filebrowser (dbus_proxy, show);
        return;
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        show = TRUE;
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        show = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    audacious_remote_toggle_filebrowser (dbus_proxy, show);
}

void shutdown_audacious_server (int argc, char * * argv)
{
    audacious_remote_quit (dbus_proxy);
}

void get_handlers_list (int argc, char * * argv)
{
    int i;

    for (i = 0; handlers[i].name != NULL; i++)
    {
        if (! g_ascii_strcasecmp ("<sep>", handlers[i].name))
            audtool_report ("%s%s:", i == 0 ? "" : "\n", handlers[i].desc);
        else
            audtool_report ("   %-34s - %s", handlers[i].name, handlers[i].desc);
    }

    audtool_report ("");
    audtool_report ("Handlers may be prefixed with `--' (GNU-style long-options) or not, your choice.");
    audtool_report ("Report bugs to http://redmine.audacious-media-player.org/");
}

void toggle_aot (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    if (! g_ascii_strcasecmp (argv[1], "on"))
        audacious_remote_toggle_aot (dbus_proxy, TRUE);
    else if (! g_ascii_strcasecmp (argv[1], "off"))
        audacious_remote_toggle_aot (dbus_proxy, FALSE);
}

void get_version (int argc, char * * argv)
{
    char * version = NULL;
    version = audacious_remote_get_version (dbus_proxy);

    if (version)
        audtool_report ("Audacious %s", version);

    g_free (version);
}
