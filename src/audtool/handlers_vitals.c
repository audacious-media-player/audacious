/*
 * handlers_vitals.c
 * Copyright 2005-2007 George Averill and William Pitcock
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
#include <glib/gi18n.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void get_current_song(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gchar *song = audacious_remote_get_playlist_title(dbus_proxy, playpos);

	if (!song)
	{
		audtool_report("No song playing.");
		return;
	}

	audtool_report("%s", song);
}

void get_current_song_filename (gint argc, gchar * * argv)
{
    gint playpos = audacious_remote_get_playlist_pos (dbus_proxy);
    gchar * uri, * filename;

    uri = audacious_remote_get_playlist_file (dbus_proxy, playpos);
    filename = (uri != NULL) ? g_filename_from_uri (uri, NULL, NULL) : NULL;

    audtool_report ("%s", (filename != NULL) ? filename : (uri != NULL) ? uri :
     _("No song playing."));

    g_free (uri);
    g_free (filename);
}

void get_current_song_output_length(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);
	gint length = frames / 1000;

	audtool_report("%d:%.2d", length / 60, length % 60);
}

void get_current_song_output_length_seconds(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);
	gint length = frames / 1000;

	audtool_report("%d", length);
}

void get_current_song_output_length_frames(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);

	audtool_report("%d", frames);
}

void get_current_song_length(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);
	gint length = frames / 1000;

	audtool_report("%d:%.2d", length / 60, length % 60);
}

void get_current_song_length_seconds(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);
	gint length = frames / 1000;

	audtool_report("%d", length);
}

void get_current_song_length_frames(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);

	audtool_report("%d", frames);
}

void get_current_song_bitrate(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	audtool_report("%d", rate);
}

void get_current_song_bitrate_kbps(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	audtool_report("%d", rate / 1000);
}

void get_current_song_frequency(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	audtool_report("%d", freq);
}

void get_current_song_frequency_khz(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	audtool_report("%0.1f", (gfloat) freq / 1000);
}

void get_current_song_channels(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	audtool_report("%d", nch);
}

void get_current_song_tuple_field_data(gint argc, gchar **argv)
{
	gchar *data;

	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<fieldname>");
		audtool_whine_tuple_fields();
		exit(1);
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[1], audacious_remote_get_playlist_pos(dbus_proxy))))
	{
		return;
	}

	audtool_report("%s", data);

	g_free(data);
}

void get_current_song_info(gint argc, gchar **argv)
{
    gint rate, freq, nch;

    audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);
    audtool_report("rate = %d freq = %d nch = %d", rate, freq, nch);
}

