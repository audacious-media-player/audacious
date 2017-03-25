/*
 * main.c
 * Copyright 2005-2013 George Averill, William Pitcock, Yoshiki Yazawa, and
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
#include <locale.h>

#include "audtool.h"

const struct commandhandler handlers[] =
{
    {"<sep>", NULL, "Current song information", 0},
    {"current-song", get_current_song, "print formatted song title", 0},
    {"current-song-filename", get_current_song_filename, "print song file name", 0},
    {"current-song-length", get_current_song_length, "print song length", 0},
    {"current-song-length-seconds", get_current_song_length_seconds, "print song length in seconds", 0},
    {"current-song-length-frames", get_current_song_length_frames, "print song length in milliseconds", 0},
    {"current-song-output-length", get_current_song_output_length, "print playback time", 0},
    {"current-song-output-length-seconds", get_current_song_output_length_seconds, "print playback time in seconds", 0},
    {"current-song-output-length-frames", get_current_song_output_length_frames, "print playback time in milliseconds", 0},
    {"current-song-bitrate", get_current_song_bitrate, "print bitrate in bits per second", 0},
    {"current-song-bitrate-kbps", get_current_song_bitrate_kbps, "print bitrate in kilobits per second", 0},
    {"current-song-frequency", get_current_song_frequency, "print sampling rate in hertz", 0},
    {"current-song-frequency-khz", get_current_song_frequency_khz, "print sampling rate in kilohertz", 0},
    {"current-song-channels", get_current_song_channels, "print number of channels", 0},
    {"current-song-tuple-data", get_current_song_tuple_field_data, "print value of named field", 1},
    {"current-song-info", get_current_song_info, "print bitrate, sampling rate, and channels", 0},

    {"<sep>", NULL, "Playback commands", 0},
    {"playback-play", playback_play, "start/restart/unpause playback", 0},
    {"playback-pause", playback_pause, "pause/unpause playback", 0},
    {"playback-playpause", playback_playpause, "start/pause/unpause playback", 0},
    {"playback-stop", playback_stop, "stop playback", 0},
    {"playback-playing", playback_playing, "exit code = 0 if playing", 0},
    {"playback-paused", playback_paused, "exit code = 0 if paused", 0},
    {"playback-stopped", playback_stopped, "exit code = 0 if not playing", 0},
    {"playback-status", playback_status, "print status (playing/paused/stopped)", 0},
    {"playback-seek", playback_seek, "seek to given time", 1},
    {"playback-seek-relative", playback_seek_relative, "seek to relative time offset", 1},
    {"playback-record", playback_record, "toggle stream recording", 0},
    {"playback-recording", playback_recording, "exit code = 0 if recording", 0},

    {"<sep>", NULL, "Playlist commands", 0},
    {"select-displayed", select_displayed, "apply commands to displayed playlist", 0},
    {"select-playing", select_playing, "apply commands to playing playlist", 0},
    {"playlist-advance", playlist_advance, "skip to next song", 0},
    {"playlist-reverse", playlist_reverse, "skip to previous song", 0},
    {"playlist-addurl", playlist_add_url_string, "add URI at end of playlist", 1},
    {"playlist-insurl", playlist_ins_url_string, "insert URI at given position", 2},
    {"playlist-addurl-to-new-playlist", playlist_enqueue_to_temp, "open URI in \"Now Playing\" playlist", 1},
    {"playlist-delete", playlist_delete, "remove song at given position", 1},
    {"playlist-length", playlist_length, "print number of songs in playlist", 0},
    {"playlist-song", playlist_song, "print formatted title of given song", 1},
    {"playlist-song-filename", playlist_song_filename, "print file name of given song", 1},
    {"playlist-song-length", playlist_song_length, "print length of given song", 1},
    {"playlist-song-length-seconds", playlist_song_length_seconds, "print length of given song in seconds", 1},
    {"playlist-song-length-frames", playlist_song_length_frames, "print length of given song in milliseconds", 1},
    {"playlist-tuple-data", playlist_tuple_field_data, "print value of named field for given song", 2},
    {"playlist-display", playlist_display, "print all songs in playlist", 0},
    {"playlist-position", playlist_position, "print position of current song", 0},
    {"playlist-jump", playlist_jump, "skip to given song", 1},
    {"playlist-clear", playlist_clear, "clear playlist", 0},
    {"playlist-auto-advance-status", playlist_auto_advance_status, "query playlist auto-advance", 0},
    {"playlist-auto-advance-toggle", playlist_auto_advance_toggle, "toggle playlist auto-advance", 0},
    {"playlist-repeat-status", playlist_repeat_status, "query playlist repeat", 0},
    {"playlist-repeat-toggle", playlist_repeat_toggle, "toggle playlist repeat", 0},
    {"playlist-shuffle-status", playlist_shuffle_status, "query playlist shuffle", 0},
    {"playlist-shuffle-toggle", playlist_shuffle_toggle, "toggle playlist shuffle", 0},
    {"playlist-stop-after-status", playlist_stop_after_status, "query if stopping after current song", 0},
    {"playlist-stop-after-toggle", playlist_stop_after_toggle, "toggle if stopping after current song", 0},

    {"<sep>", NULL, "More playlist commands", 0},
    {"number-of-playlists", number_of_playlists, "print number of playlists", 0},
    {"current-playlist", current_playlist, "print number of current playlist", 0},
    {"play-current-playlist", play_current_playlist, "play/resume current playlist", 0},
    {"set-current-playlist", set_current_playlist, "make given playlist current", 1},
    {"current-playlist-name", playlist_title, "print current playlist title", 0},
    {"set-current-playlist-name", set_playlist_title, "set current playlist title", 1},
    {"new-playlist", new_playlist, "insert new playlist", 0},
    {"delete-current-playlist", delete_current_playlist, "remove current playlist", 0},

    {"<sep>", NULL, "Playlist queue commands", 0},
    {"playqueue-add", playqueue_add, "add given song to queue", 1},
    {"playqueue-remove", playqueue_remove, "remove given song from queue", 1},
    {"playqueue-is-queued", playqueue_is_queued, "exit code = 0 if given song is queued", 1},
    {"playqueue-get-queue-position", playqueue_get_queue_position, "print queue position of given song", 1},
    {"playqueue-get-list-position", playqueue_get_list_position, "print n-th queued song", 1},
    {"playqueue-length", playqueue_length, "print number of songs in queue", 0},
    {"playqueue-display", playqueue_display, "print all songs in queue", 0},
    {"playqueue-clear", playqueue_clear, "clear queue", 0},

    {"<sep>", NULL, "Volume control and equalizer", 0},
    {"get-volume", get_volume, "print current volume level in percent", 0},
    {"set-volume", set_volume, "set current volume level in percent", 1},
    {"equalizer-activate", equalizer_active, "activate/deactivate equalizer", 1},
    {"equalizer-get", equalizer_get_eq, "print equalizer settings", 0},
    {"equalizer-set", equalizer_set_eq, "set equalizer settings", 11},
    {"equalizer-get-preamp", equalizer_get_eq_preamp, "print equalizer pre-amplification", 0},
    {"equalizer-set-preamp", equalizer_set_eq_preamp, "set equalizer pre-amplification", 1},
    {"equalizer-get-band", equalizer_get_eq_band, "print gain of given equalizer band", 1},
    {"equalizer-set-band", equalizer_set_eq_band, "set gain of given equalizer band", 2},

    {"<sep>", NULL, "Miscellaneous", 0},
    {"mainwin-show", mainwin_show, "show/hide Audacious", 1},
    {"filebrowser-show", show_filebrowser, "show/hide Add Files window", 1},
    {"jumptofile-show", show_jtf_window, "show/hide Jump to Song window", 1},
    {"preferences-show", show_preferences_window, "show/hide Settings window", 1},
    {"about-show", show_about_window, "show/hide About window", 1},

    {"version", get_version, "print Audacious version", 0},
    {"plugin-is-enabled", plugin_is_enabled, "exit code = 0 if plugin is enabled", 1},
    {"plugin-enable", plugin_enable, "enable/disable plugin", 2},
    {"shutdown", shutdown_audacious_server, "shut down Audacious", 0},

    {"help", get_handlers_list, "print this help", 0},

    {NULL, NULL, NULL, 0}
};

ObjAudacious * dbus_proxy = NULL;
static GDBusConnection * connection = NULL;

static void audtool_disconnect (void)
{
    g_object_unref (dbus_proxy);
    dbus_proxy = NULL;

    g_dbus_connection_close_sync (connection, NULL, NULL);
    connection = NULL;
}

static void audtool_connect (int instance)
{
    GError * error = NULL;

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, & error);

    if (! connection)
    {
        fprintf (stderr, "D-Bus error: %s\n", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

    char name[32];
    if (instance == 1)
        strcpy (name, "org.atheme.audacious");
    else
        sprintf (name, "org.atheme.audacious-%d", instance);

    dbus_proxy = obj_audacious_proxy_new_sync (connection, 0, name,
     "/org/atheme/audacious", NULL, & error);

    if (! dbus_proxy)
    {
        fprintf (stderr, "D-Bus error: %s\n", error->message);
        g_error_free (error);
        g_dbus_connection_close_sync (connection, NULL, NULL);
        exit (EXIT_FAILURE);
    }

    atexit (audtool_disconnect);
}

int main (int argc, char * * argv)
{
    int instance = 1;
    int i, j, k = 0;

    setlocale (LC_CTYPE, "");

#if ! GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init();
#endif

    // parse instance number (must come first)
    if (argc >= 2 && argv[1][0] == '-' && argv[1][1] >= '1' && argv[1][1] <= '9' && ! argv[1][2])
    {
        instance = argv[1][1] - '0';
        argc --;
        argv ++;
    }

    audtool_connect (instance);

    if (argc < 2)
    {
        fprintf (stderr, "Not enough parameters.  Try \"audtool help\".\n");
        exit (EXIT_FAILURE);
    }

    for (j = 1; j < argc; j ++)
    {
        for (i = 0; handlers[i].name != NULL; i++)
        {
            if ((! g_ascii_strcasecmp (handlers[i].name, argv[j]) ||
             ! g_ascii_strcasecmp (g_strconcat ("--", handlers[i].name, NULL),
             argv[j])) && g_ascii_strcasecmp ("<sep>", handlers[i].name))
            {
                int numargs = MIN (handlers[i].args + 1, argc - j);
                handlers[i].handler (numargs, & argv[j]);
                j += handlers[i].args;
                k ++;

                if (j >= argc)
                    break;
            }
        }
    }

    if (k == 0)
    {
        fprintf (stderr, "Unknown command \"%s\".  Try \"audtool help\".\n", argv[1]);
        exit (EXIT_FAILURE);
    }

    return 0;
}
