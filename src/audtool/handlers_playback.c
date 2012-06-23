/*
 * handlers_playback.c
 * Copyright 2005-2008 George Averill, William Pitcock, and Matti Hämäläinen
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

void playback_play(gint argc, gchar **argv)
{
	audacious_remote_play(dbus_proxy);
}

void playback_pause(gint argc, gchar **argv)
{
	audacious_remote_pause(dbus_proxy);
}

void playback_playpause(gint argc, gchar **argv)
{
	if (audacious_remote_is_playing(dbus_proxy))
	{
		audacious_remote_pause(dbus_proxy);
	}
	else
	{
		audacious_remote_play(dbus_proxy);
	}
}

void playback_stop(gint argc, gchar **argv)
{
	audacious_remote_stop(dbus_proxy);
}

void playback_playing(gint argc, gchar **argv)
{
	if (!audacious_remote_is_paused(dbus_proxy))
	{
		exit(!audacious_remote_is_playing(dbus_proxy));
	}
	else
	{
		exit(1);
	}
}

void playback_paused(gint argc, gchar **argv)
{
	exit(!audacious_remote_is_paused(dbus_proxy));
}

void playback_stopped(gint argc, gchar **argv)
{
	if (!audacious_remote_is_playing(dbus_proxy) && !audacious_remote_is_paused(dbus_proxy))
	{
		exit(0);
	}
	else
	{
		exit(1);
	}
}

void playback_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_paused(dbus_proxy))
	{
		audtool_report("paused");
		return;
	}
	else if (audacious_remote_is_playing(dbus_proxy))
	{
		audtool_report("playing");
		return;
	}
	else
	{
		audtool_report("stopped");
		return;
	}
}

void playback_seek(gint argc, gchar **argv)
{
	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<position>");
		exit(1);
	}

	audacious_remote_jump_to_time(dbus_proxy, atoi(argv[1]) * 1000);
}

void playback_seek_relative(gint argc, gchar **argv)
{
	gint oldtime, newtime, diff;

	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<position>");
		exit(1);
	}

	oldtime = audacious_remote_get_output_time(dbus_proxy);
	diff = atoi(argv[1]) * 1000;
	newtime = oldtime + diff;

	audacious_remote_jump_to_time(dbus_proxy, newtime);
}
