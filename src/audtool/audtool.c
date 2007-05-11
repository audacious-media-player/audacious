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

struct commandhandler handlers[] = {
	{"<sep>", NULL, "Vital information"},
	{"current-song", get_current_song, "returns current song title"},
	{"current-song-filename", get_current_song_filename, "returns current song filename"},
	{"current-song-length", get_current_song_length, "returns current song length"},
	{"current-song-length-seconds", get_current_song_length_seconds, "returns current song length in seconds"},
	{"current-song-length-frames", get_current_song_length_frames, "returns current song length in frames"},
	{"current-song-output-length", get_current_song_output_length, "returns current song output length"},
	{"current-song-output-length-seconds", get_current_song_output_length_seconds, "returns current song output length in seconds"},
	{"current-song-output-length-frames", get_current_song_output_length_frames, "returns current song output length in frames"},
	{"current-song-bitrate", get_current_song_bitrate, "returns current song bitrate in bits per second"},
	{"current-song-bitrate-kbps", get_current_song_bitrate_kbps, "returns current song bitrate in kilobits per second"},
	{"current-song-frequency", get_current_song_frequency, "returns current song frequency in hertz"},
	{"current-song-frequency-khz", get_current_song_frequency_khz, "returns current song frequency in kilohertz"},
	{"current-song-channels", get_current_song_channels, "returns current song channels"},
	{"current-song-tuple-data", get_current_song_tuple_field_data, "returns the value of a tuple field for the current song"},
	{"<sep>", NULL, "Playlist manipulation"},
	{"playlist-advance", playlist_advance, "go to the next song in the playlist"},
	{"playlist-reverse", playlist_reverse, "go to the previous song in the playlist"},
	{"playlist-addurl", playlist_add_url_string, "adds a url to the playlist"},
	{"playlist-delete", playlist_delete, "deletes a song from the playlist"},
	{"playlist-length", playlist_length, "returns the total length of the playlist"},
	{"playlist-song", playlist_song, "returns the title of a song in the playlist"},
	{"playlist-song-filename", playlist_song_filename, "returns the filename of a song in the playlist"},
	{"playlist-song-length", playlist_song_length, "returns the length of a song in the playlist"},
	{"playlist-song-length-seconds", playlist_song_length_seconds, "returns the length of a song in the playlist in seconds"},
	{"playlist-song-length-frames", playlist_song_length_frames, "returns the length of a song in the playlist in frames"},
	{"playlist-display", playlist_display, "returns the entire playlist"},
	{"playlist-position", playlist_position, "returns the position in the playlist"},
	{"playlist-jump", playlist_jump, "jumps to a position in the playlist"},
	{"playlist-clear", playlist_clear, "clears the playlist"},
	{"playlist-repeat-status", playlist_repeat_status, "returns the status of playlist repeat"},
	{"playlist-repeat-toggle", playlist_repeat_toggle, "toggles playlist repeat"},
	{"playlist-shuffle-status", playlist_shuffle_status, "returns the status of playlist shuffle"},
	{"playlist-shuffle-toggle", playlist_shuffle_toggle, "toggles playlist shuffle"},
	{"playlist-tuple-data", playlist_tuple_field_data, "returns the value of a tuple field for a song in the playlist"},
	{"<sep>", NULL, "Playqueue manipulation"},
	{"playqueue-add", playqueue_add, "adds a song to the playqueue"},
	{"playqueue-remove", playqueue_remove, "removes a song from the playqueue"},
	{"playqueue-is-queued", playqueue_is_queued, "returns OK if a song is queued"},
	{"playqueue-get-position", playqueue_get_position, "returns the queue position of a song in the playlist"},
	{"playqueue-get-qposition", playqueue_get_qposition, "returns the playlist position of a song in the queue"},
	{"playqueue-length", playqueue_length, "returns the length of the playqueue"},
	{"playqueue-display", playqueue_display, "returns a list of currently-queued songs"},
	{"playqueue-clear", playqueue_clear, "clears the playqueue"},
	{"<sep>", NULL, "Playback manipulation"},
	{"playback-play", playback_play, "starts/unpauses song playback"},
	{"playback-pause", playback_pause, "(un)pauses song playback"},
	{"playback-playpause", playback_playpause, "plays/(un)pauses song playback"},
	{"playback-stop", playback_stop, "stops song playback"},
	{"playback-playing", playback_playing, "returns OK if audacious is playing"},
	{"playback-paused", playback_paused, "returns OK if audacious is paused"},
	{"playback-stopped", playback_stopped, "returns OK if audacious is stopped"},
	{"playback-status", playback_status, "returns the playback status"},
	{"playback-seek", playback_seek, "performs an absolute seek"},
	{"playback-seek-relative", playback_seek_relative, "performs a seek relative to the current position"},
	{"<sep>", NULL, "Volume control"},
	{"get-volume", get_volume, "returns the current volume level in percent"},
	{"set-volume", set_volume, "sets the current volume level in percent"},
	{"<sep>", NULL, "Miscellaneous"},
	{"mainwin-show", mainwin_show, "shows/hides the main window"},
	{"playlist-show", playlist_show, "shows/hides the playlist window"},
	{"equalizer-show", equalizer_show, "shows/hides the equalizer window"},
	{"preferences", show_preferences_window, "shows/hides the preferences window"},
	{"jumptofile", show_jtf_window, "shows the jump to file window"},
	{"shutdown", shutdown_audacious_server, "shuts down audacious"},
	{"<sep>", NULL, "Help system"},
	{"list-handlers", get_handlers_list, "shows handlers list"},
	{"help", get_handlers_list, "shows handlers list"},
	{NULL, NULL, NULL}
};

mowgli_error_context_t *e = NULL;
DBusGProxy *dbus_proxy = NULL;
static DBusGConnection *connection = NULL;

static void audtool_connect(void)
{
	GError *error = NULL;

	mowgli_error_context_push(e, "While attempting to connect to the D-Bus session bus");
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
		mowgli_error_context_display_with_error(e, "\n  * ", g_strdup_printf("D-Bus Error: %s", error->message));

	mowgli_error_context_pop(e);

	dbus_proxy = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                           AUDACIOUS_DBUS_PATH,
                                           AUDACIOUS_DBUS_INTERFACE);
}

gint main(gint argc, gchar **argv)
{
	gint i;

	setlocale(LC_CTYPE, "");
	g_type_init();
	mowgli_init();

	e = mowgli_error_context_create();
	mowgli_error_context_push(e, "In program %s", argv[0]);

	audtool_connect();

	mowgli_error_context_push(e, "While processing the commandline");

	if (argc < 2)
		mowgli_error_context_display_with_error(e, "\n  * ", "not enough parameters, use audtool --help for more information.");

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if ((!g_strcasecmp(handlers[i].name, argv[1]) ||
		     !g_strcasecmp(g_strconcat("--", handlers[i].name, NULL), argv[1]))
		    && g_strcasecmp("<sep>", handlers[i].name))
  		{
 			handlers[i].handler(argc, argv);
			exit(0);
		}
	}

	mowgli_error_context_display_with_error(e, "\n  * ", g_strdup_printf("Unknown command `%s' encountered, use audtool --help for a command list.", argv[1]));

	return 0;
}
