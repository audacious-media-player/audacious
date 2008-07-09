/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void get_volume(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_main_volume(dbus_proxy);

	audtool_report("%d", i);
}

void set_volume(gint argc, gchar **argv)
{
	gint i, current_volume;

	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<level>", argv[0]);
		exit(1);
	}

	current_volume = audacious_remote_get_main_volume(dbus_proxy);
	switch (argv[1][0]) 
	{
		case '+':
		case '-':
			i = current_volume + atoi(argv[1]);
			break;
		default:
			i = atoi(argv[1]);
			break;
	}

	audacious_remote_set_main_volume(dbus_proxy, i);
}

void mainwin_show(gint argc, gchar **argv)
{
	if (argc < 2) {
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1], "on")) {
        audacious_remote_main_win_toggle(dbus_proxy, TRUE);
        return;
    }
    else if (!g_ascii_strcasecmp(argv[1], "off")) {
        audacious_remote_main_win_toggle(dbus_proxy, FALSE);
        return;
    }
}

void playlist_show(gint argc, gchar **argv)
{
	if (argc < 2) {
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1], "on")) {
        audacious_remote_pl_win_toggle(dbus_proxy, TRUE);
        return;
    }
    else if (!g_ascii_strcasecmp(argv[1], "off")) {
        audacious_remote_pl_win_toggle(dbus_proxy, FALSE);
        return;
    }
}

void equalizer_show(gint argc, gchar **argv)
{
	if (argc < 2) {
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1] ,"on")) {
        audacious_remote_eq_win_toggle(dbus_proxy, TRUE);
        return;
    }
    else if (!g_ascii_strcasecmp(argv[1] ,"off")) {
        audacious_remote_eq_win_toggle(dbus_proxy, FALSE);
        return;
    }
}

void show_preferences_window(gint argc, gchar **argv)
{
    gboolean show = TRUE;

	if (argc < 2) {
#if 0
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
#else
        audacious_remote_toggle_prefs_box(dbus_proxy, show);
        return;
#endif
    }

    if (!g_ascii_strcasecmp(argv[1], "on"))
        show = TRUE;
    else if (!g_ascii_strcasecmp(argv[1], "off"))
        show = FALSE;
    else {
        audtool_whine_args(argv[0], "<on/off>");
        exit (1);
    }

	audacious_remote_toggle_prefs_box(dbus_proxy, show);
}

void show_about_window(gint argc, gchar **argv)
{
    gboolean show = TRUE;

	if (argc < 2) {
#if 0
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
#else
        audacious_remote_toggle_about_box(dbus_proxy, show);
        return;
#endif
    }

    if (!g_ascii_strcasecmp(argv[1], "on"))
        show = TRUE;
    else if (!g_ascii_strcasecmp(argv[1], "off"))
        show = FALSE;
    else {
        audtool_whine_args(argv[0], "<on/off>");
        exit (1);
    }

	audacious_remote_toggle_about_box(dbus_proxy, show);
}

void show_jtf_window(gint argc, gchar **argv)
{
    gboolean show = TRUE;

	if (argc < 2) {
#if 0
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
#else
        audacious_remote_toggle_jtf_box(dbus_proxy, show);
        return;
#endif
    }
    if (!g_ascii_strcasecmp(argv[1], "on"))
        show = TRUE;
    else if (!g_ascii_strcasecmp(argv[1], "off"))
        show = FALSE;
    else {
        audtool_whine_args(argv[0], "<on/off>");
        exit (1);
    }

	audacious_remote_toggle_jtf_box(dbus_proxy, show);
}

void show_filebrowser(gint argc, gchar **argv)
{
    gboolean show = TRUE;

	if (argc < 2) {
#if 0
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
#else
        audacious_remote_toggle_filebrowser(dbus_proxy, show);
        return;
#endif
    }

    if (!g_ascii_strcasecmp(argv[1], "on"))
        show = TRUE;
    else if (!g_ascii_strcasecmp(argv[1], "off"))
        show = FALSE;
    else {
        audtool_whine_args(argv[0], "<on/off>");
        exit (1);
    }

	audacious_remote_toggle_filebrowser(dbus_proxy, show);
}

void shutdown_audacious_server(gint argc, gchar **argv)
{
	audacious_remote_quit(dbus_proxy);
}

void get_handlers_list(gint argc, gchar **argv)
{
	gint i;

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if (!g_ascii_strcasecmp("<sep>", handlers[i].name))
			audtool_report("%s%s:", i == 0 ? "" : "\n", handlers[i].desc);
		else
			audtool_report("   %-34s - %s", handlers[i].name, handlers[i].desc);
	}

    audtool_report("");
	audtool_report("Handlers may be prefixed with `--' (GNU-style long-options) or not, your choice.");
	audtool_report("Report bugs to http://bugzilla.atheme.org/");
}

void activate(gint argc, gchar **argv)
{
    audacious_remote_activate(dbus_proxy);
}

void toggle_aot(gint argc, gchar **argv)
{
    if (argc < 2)
    {
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1], "on")) {
        audacious_remote_toggle_aot(dbus_proxy, TRUE);
        return;
    }
    else if (!g_ascii_strcasecmp(argv[1], "off")) {
        audacious_remote_toggle_aot(dbus_proxy, FALSE);
        return;
    }
}

void get_version(gint argc, gchar **argv)
{
    gchar *version = NULL;
    version = audacious_remote_get_version(dbus_proxy);
    if(version)
        audtool_report("Audacious %s", version);
    g_free(version);
}
