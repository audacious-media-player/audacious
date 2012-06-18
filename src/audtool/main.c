/*
 * main.c
 * Copyright 2005-2011 George Averill, William Pitcock, Yoshiki Yazawa, and
 *                     John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

struct commandhandler handlers[] = {
	{"<sep>", NULL, "Vital information", 0},
	{"current-song", get_current_song, "returns current song title", 0},
	{"current-song-filename", get_current_song_filename, "returns current song filename", 0},
	{"current-song-length", get_current_song_length, "returns current song length", 0},
	{"current-song-length-seconds", get_current_song_length_seconds, "returns current song length in seconds", 0},
	{"current-song-length-frames", get_current_song_length_frames, "returns current song length in frames", 0},
	{"current-song-output-length", get_current_song_output_length, "returns current song output length", 0},
	{"current-song-output-length-seconds", get_current_song_output_length_seconds, "returns current song output length in seconds", 0},
	{"current-song-output-length-frames", get_current_song_output_length_frames, "returns current song output length in frames", 0},
	{"current-song-bitrate", get_current_song_bitrate, "returns current song bitrate in bits per second", 0},
	{"current-song-bitrate-kbps", get_current_song_bitrate_kbps, "returns current song bitrate in kilobits per second", 0},
	{"current-song-frequency", get_current_song_frequency, "returns current song frequency in hertz", 0},
	{"current-song-frequency-khz", get_current_song_frequency_khz, "returns current song frequency in kilohertz", 0},
	{"current-song-channels", get_current_song_channels, "returns current song channels", 0},
	{"current-song-tuple-data", get_current_song_tuple_field_data, "returns the value of a tuple field for the current song", 1},
    {"current-song-info", get_current_song_info, "returns current song bitrate, frequency and channels", 0},


	{"<sep>", NULL, "Playlist manipulation", 0},
	{"playlist-advance", playlist_advance, "go to the next song in the playlist", 0},
	{"playlist-auto-advance-status", playlist_auto_advance_status, "returns the status of playlist auto-advance", 0},
	{"playlist-auto-advance-toggle", playlist_auto_advance_toggle, "toggles playlist auto-advance", 0},
	{"playlist-reverse", playlist_reverse, "go to the previous song in the playlist", 0},
	{"playlist-addurl", playlist_add_url_string, "adds a URL to the playlist", 1},
    {"playlist-insurl", playlist_ins_url_string, "inserts a URL at specified position in the playlist", 2},
	{"playlist-addurl-to-new-playlist", playlist_enqueue_to_temp, "adds a URL to the newly created playlist", 1},
	{"playlist-delete", playlist_delete, "deletes a song from the playlist", 1},
	{"playlist-length", playlist_length, "returns the total length of the playlist", 0},
	{"playlist-song", playlist_song, "returns the title of a song in the playlist", 1},
	{"playlist-song-filename", playlist_song_filename, "returns the filename of a song in the playlist", 1},
	{"playlist-song-length", playlist_song_length, "returns the length of a song in the playlist", 1},
	{"playlist-song-length-seconds", playlist_song_length_seconds, "returns the length of a song in the playlist in seconds", 1},
	{"playlist-song-length-frames", playlist_song_length_frames, "returns the length of a song in the playlist in frames", 1},
	{"playlist-display", playlist_display, "returns the entire playlist", 0},
	{"playlist-position", playlist_position, "returns the position in the playlist", 0},
	{"playlist-jump", playlist_jump, "jumps to a position in the playlist", 1},
	{"playlist-clear", playlist_clear, "clears the playlist", 0},
	{"playlist-repeat-status", playlist_repeat_status, "returns the status of playlist repeat", 0},
	{"playlist-repeat-toggle", playlist_repeat_toggle, "toggles playlist repeat", 0},
	{"playlist-shuffle-status", playlist_shuffle_status, "returns the status of playlist shuffle", 0},
	{"playlist-shuffle-toggle", playlist_shuffle_toggle, "toggles playlist shuffle", 0},
	{"playlist-stop-after-status", playlist_stop_after_status, "queries if stopping after current song", 0},
	{"playlist-stop-after-toggle", playlist_stop_after_toggle, "toggles if stopping after current song", 0},
	{"playlist-tuple-data", playlist_tuple_field_data, "returns the value of a tuple field for a song in the playlist", 2},
	{"current-playlist-name", playlist_title, "returns the playlist title of the active playlist", 0},

	{"<sep>", NULL, "Playqueue manipulation", 0},
	{"playqueue-add", playqueue_add, "adds a song to the playqueue", 1},
	{"playqueue-remove", playqueue_remove, "removes a song from the playqueue", 1},
	{"playqueue-is-queued", playqueue_is_queued, "returns OK if a song is queued", 1},
	{"playqueue-get-queue-position", playqueue_get_queue_position, "returns the playqueue position of a song in the given position in the playlist", 1},
	{"playqueue-get-list-position", playqueue_get_list_position, "returns the playlist position of a song in the given position in the playqueue", 1},
	{"playqueue-length", playqueue_length, "returns the length of the playqueue", 0},
	{"playqueue-display", playqueue_display, "returns a list of currently-queued songs", 0},
	{"playqueue-clear", playqueue_clear, "clears the playqueue", 0},


	{"<sep>", NULL, "Playback manipulation", 0},
	{"playback-play", playback_play, "starts/unpauses song playback", 0},
	{"playback-pause", playback_pause, "(un)pauses song playback", 0},
	{"playback-playpause", playback_playpause, "plays/(un)pauses song playback", 0},
	{"playback-stop", playback_stop, "stops song playback", 0},
	{"playback-playing", playback_playing, "returns OK if audacious is playing", 0},
	{"playback-paused", playback_paused, "returns OK if audacious is paused", 0},
	{"playback-stopped", playback_stopped, "returns OK if audacious is stopped", 0},
	{"playback-status", playback_status, "returns the playback status", 0},
	{"playback-seek", playback_seek, "performs an absolute seek", 1},
	{"playback-seek-relative", playback_seek_relative, "performs a seek relative to the current position", 1},


	{"<sep>", NULL, "Volume control", 0},
	{"get-volume", get_volume, "returns the current volume level in percent", 0},
	{"set-volume", set_volume, "sets the current volume level in percent", 1},


	{"<sep>", NULL, "Equalizer manipulation", 0},
    {"equalizer-activate", equalizer_active, "activates/deactivates the equalizer", 1},
    {"equalizer-get", equalizer_get_eq, "gets the equalizer settings", 0},
    {"equalizer-set", equalizer_set_eq, "sets the equalizer settings", 11},
    {"equalizer-get-preamp", equalizer_get_eq_preamp, "gets the equalizer pre-amplification", 0},
    {"equalizer-set-preamp", equalizer_set_eq_preamp, "sets the equalizer pre-amplification", 1},
    {"equalizer-get-band", equalizer_get_eq_band, "gets the equalizer value in specified band", 1},
    {"equalizer-set-band", equalizer_set_eq_band, "sets the equalizer value in the specified band", 2},


	{"<sep>", NULL, "Miscellaneous", 0},
	{"mainwin-show", mainwin_show, "shows/hides the main window", 1},
	{"filebrowser-show", show_filebrowser, "shows/hides the filebrowser", 1},
	{"jumptofile-show", show_jtf_window, "shows/hides the jump to file window", 1},
	{"preferences-show", show_preferences_window, "shows/hides the preferences window", 1},
	{"about-show", show_about_window, "shows/hides the about window", 1},

	{"always-on-top", toggle_aot, "on/off always on top", 1},
    {"version", get_version, "shows Audacious version", 0},
	{"shutdown", shutdown_audacious_server, "shuts down Audacious", 0},


	{"<sep>", NULL, "Help system", 0},
	{"list-handlers", get_handlers_list, "shows handlers list", 0},
	{"help", get_handlers_list, "shows handlers list", 0},


	{NULL, NULL, NULL, 0}
};

DBusGProxy *dbus_proxy = NULL;
static DBusGConnection *connection = NULL;

static void
audtool_connect(void)
{
	GError *error = NULL;

	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		fprintf (stderr, "D-Bus Error: %s\n", error->message);
		exit (EXIT_FAILURE);
	}

	dbus_proxy = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                           AUDACIOUS_DBUS_PATH,
                                           AUDACIOUS_DBUS_INTERFACE);
}

static void
audtool_disconnect(void)
{
	g_object_unref(dbus_proxy);
	dbus_proxy = NULL;
}

gint
main(gint argc, gchar **argv)
{
	gint i, j = 0, k = 0;

	setlocale(LC_CTYPE, "");
	g_type_init();

	audtool_connect();

	if (argc < 2)
	{
		fprintf (stderr, "Not enough parameters.  Try \"audtool help\".\n");
		exit (EXIT_FAILURE);
	}

	for (j = 1; j < argc; j++)
	{
		for (i = 0; handlers[i].name != NULL; i++)
		{
			if ((!g_ascii_strcasecmp(handlers[i].name, argv[j]) ||
			     !g_ascii_strcasecmp(g_strconcat("--", handlers[i].name, NULL), argv[j]))
			    && g_ascii_strcasecmp("<sep>", handlers[i].name))
  			{
				int numargs = handlers[i].args + 1 < argc - j ? handlers[i].args + 1 : argc - j;
				handlers[i].handler(numargs, &argv[j]);
				j += handlers[i].args;
				k++;
				if(j >= argc)
					break;
			}
		}
	}

	if (k == 0)
	{
		fprintf (stderr, "Unknown command \"%s\".  Try \"audtool help\".\n", argv[1]);
		exit (EXIT_FAILURE);
	}

	audtool_disconnect();

	return 0;
}
