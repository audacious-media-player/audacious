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

void get_current_song(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gchar *song = audacious_remote_get_playlist_title(dbus_proxy, playpos);

	if (!song)
	{
		g_print("No song playing.\n");
		return;
	}

	g_print("%s\n", song);
}

void get_current_song_filename(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gchar *file = audacious_remote_get_playlist_file(dbus_proxy, playpos);

	if (!file)
	{
		g_print("No song playing.\n");
		return;
	}
        
	g_print("%s\n", file);
}

void get_current_song_output_length(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);
	gint length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void get_current_song_output_length_seconds(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);
	gint length = frames / 1000;

	g_print("%d\n", length);
}

void get_current_song_output_length_frames(gint argc, gchar **argv)
{
	gint frames = audacious_remote_get_output_time(dbus_proxy);

	g_print("%d\n", frames);
}

void get_current_song_length(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);
	gint length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void get_current_song_length_seconds(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);
	gint length = frames / 1000;

	g_print("%d\n", length);
}

void get_current_song_length_frames(gint argc, gchar **argv)
{
	gint playpos = audacious_remote_get_playlist_pos(dbus_proxy);
	gint frames = audacious_remote_get_playlist_time(dbus_proxy, playpos);

	g_print("%d\n", frames);
}

void get_current_song_bitrate(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	g_print("%d\n", rate);
}

void get_current_song_bitrate_kbps(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	g_print("%d\n", rate / 1000);
}

void get_current_song_frequency(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	g_print("%d\n", freq);
}

void get_current_song_frequency_khz(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	g_print("%0.1f\n", (gfloat) freq / 1000);
}

void get_current_song_channels(gint argc, gchar **argv)
{
	gint rate, freq, nch;

	audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);

	g_print("%d\n", nch);
}

void get_current_song_tuple_field_data(gint argc, gchar **argv)
{
	gpointer data;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for current-song-tuple-data.\n", argv[0]);
		g_print("%s: syntax: %s current-song-tuple-data <fieldname>\n", argv[0], argv[0]);
		g_print("%s:   - fieldname example choices: performer, album_name,\n", argv[0]);
		g_print("%s:       track_name, track_number, year, date, genre, comment,\n", argv[0]);
		g_print("%s:       file_name, file_ext, file_path, length, formatter,\n", argv[0]);
		g_print("%s:       custom, mtime\n", argv[0]);
		return;
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[1], audacious_remote_get_playlist_pos(dbus_proxy))))
	{
		return;
	}
	
	if (!strcasecmp(argv[1], "track_number") || !strcasecmp(argv[1], "year") || !strcasecmp(argv[1], "length") || !strcasecmp(argv[1], "mtime"))
	{
		if (*(gint *)data > 0)
		{
			g_print("%d\n", *(gint *)data);
		}
		return;
	}

	g_print("%s\n", (gchar *)data);
}
