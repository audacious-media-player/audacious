/*  Audtool -- Audacious scripting tool
 *  Copyright (c) 2005-2007  Audacious development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
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

static DBusGProxy *dbus_proxy = NULL;
static DBusGConnection *connection = NULL;

static void audtool_connect()
{
	GError *error = NULL;
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		g_printerr("audtool: D-Bus error: %s", error->message);
		g_error_free(error);

		exit(EXIT_FAILURE);
	}

	dbus_proxy = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                           AUDACIOUS_DBUS_PATH,
                                           AUDACIOUS_DBUS_INTERFACE);
}

gint main(gint argc, gchar **argv)
{
	gint i;

	setlocale(LC_CTYPE, "");
	g_type_init();

	if (argc < 2)
	{
		g_print("%s: usage: %s <command>\n", argv[0], argv[0]);
		g_print("%s: use `%s help' to get a listing of available commands.\n",
			argv[0], argv[0]);
		exit(-2);
	}

    audtool_connect();

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

	g_print("%s: invalid command '%s'\n", argv[0], argv[1]);
	g_print("%s: use `%s help' to get a listing of available commands.\n", argv[0], argv[0]);

	return 0;
}

/*** MOVE TO HANDLERS.C ***/

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

	g_print("%s\n", audacious_remote_get_playlist_file(dbus_proxy, playpos));
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

	if (argc < 3)
	{
		g_print("%s: invalid parameters for current-song-tuple-data.\n", argv[0]);
		g_print("%s: syntax: %s current-song-tuple-data <fieldname>\n", argv[0], argv[0]);
		g_print("%s:   - fieldname example choices: performer, album_name,\n", argv[0]);
		g_print("%s:       track_name, track_number, year, date, genre, comment,\n", argv[0]);
		g_print("%s:       file_name, file_ext, file_path, length, formatter,\n", argv[0]);
		g_print("%s:       custom, mtime\n", argv[0]);
		return;
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[2], audacious_remote_get_playlist_pos(dbus_proxy))))
	{
		return;
	}
	
	if (!strcasecmp(argv[2], "track_number") || !strcasecmp(argv[2], "year") || !strcasecmp(argv[2], "length") || !strcasecmp(argv[2], "mtime"))
	{
		if (*(gint *)data > 0)
		{
			g_print("%d\n", *(gint *)data);
		}
		return;
	}

	g_print("%s\n", (gchar *)data);
}

void playlist_reverse(gint argc, gchar **argv)
{
	audacious_remote_playlist_prev(dbus_proxy);
}

void playlist_advance(gint argc, gchar **argv)
{
	audacious_remote_playlist_next(dbus_proxy);
}

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
		g_print("paused\n");
		return;
	}
	else if (audacious_remote_is_playing(dbus_proxy))
	{
		g_print("playing\n");
		return;
	}
	else
	{
		g_print("stopped\n");
		return;
	}
}

void playback_seek(gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playback-seek.\n", argv[0]);
		g_print("%s: syntax: %s playback-seek <position>\n", argv[0], argv[0]);
		return;
	}

	audacious_remote_jump_to_time(dbus_proxy, atoi(argv[2]) * 1000);
}

void playback_seek_relative(gint argc, gchar **argv)
{
	gint oldtime, newtime, diff;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playback-seek-relative.\n", argv[0]);
		g_print("%s: syntax: %s playback-seek <position>\n", argv[0], argv[0]);
		return;
	}

	oldtime = audacious_remote_get_output_time(dbus_proxy);
	diff = atoi(argv[2]) * 1000;
	newtime = oldtime + diff;

	audacious_remote_jump_to_time(dbus_proxy, newtime);
}

void playlist_add_url_string(gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-addurl.\n", argv[0]);
		g_print("%s: syntax: %s playlist-addurl <url>\n", argv[0], argv[0]);
		return;
	}

	audacious_remote_playlist_add_url_string(dbus_proxy, argv[2]);
}

void playlist_delete(gint argc, gchar **argv)
{
	gint playpos;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-delete.\n", argv[0]);
		g_print("%s: syntax: %s playlist-delete <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	audacious_remote_playlist_delete(dbus_proxy, playpos - 1);
}

void playlist_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	g_print("%d\n", i);
}

void playlist_song(gint argc, gchar **argv)
{
	gint playpos;
	gchar *song;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-title.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-title <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	song = audacious_remote_get_playlist_title(dbus_proxy, playpos - 1);

	g_print("%s\n", song);
}


void playlist_song_length(gint argc, gchar **argv)
{
	gint playpos, frames, length;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void playlist_song_length_seconds(gint argc, gchar **argv)
{
	gint playpos, frames, length;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length-seconds.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-seconds <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	g_print("%d\n", length);
}

void playlist_song_length_frames(gint argc, gchar **argv)
{
	gint playpos, frames;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length-frames.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-frames <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);

	g_print("%d\n", frames);
}

void playlist_display(gint argc, gchar **argv)
{
	gint i, ii, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	g_print("%d tracks.\n", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		songname = audacious_remote_get_playlist_title(dbus_proxy, ii);
		frames = audacious_remote_get_playlist_time(dbus_proxy, ii);
		length = frames / 1000;
		total += length;

		/* adjust width for multi byte characters */
		column = 60;
		if(songname){
			p = songname;
			while(*p){
				gint stride;
				stride = g_utf8_next_char(p) - p;
				if(g_unichar_iswide(g_utf8_get_char(p))
#if ( (GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION >= 12) )
				   || g_unichar_iswide_cjk(g_utf8_get_char(p))
#endif
                                ){
					column += (stride - 2);
				}
				else {
					column += (stride - 1);
				}
				p = g_utf8_next_char(p);
			}

		}

		fmt = g_strdup_printf("%%4d | %%-%ds | %%d:%%.2d\n", column);
		g_print(fmt, ii + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playlist_position(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_pos(dbus_proxy);

	g_print("%d\n", i + 1);
}

void playlist_song_filename(gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-filename.\n", argv[0]);
		g_print("%s: syntax: %s playlist-filename <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	g_print("%s\n", audacious_remote_get_playlist_file(dbus_proxy, i - 1));
}

void playlist_jump(gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-jump.\n", argv[0]);
		g_print("%s: syntax: %s playlist-jump <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	audacious_remote_set_playlist_pos(dbus_proxy, i - 1);
}

void playlist_clear(gint argc, gchar **argv)
{
	audacious_remote_playlist_clear(dbus_proxy);
}

void playlist_repeat_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_repeat(dbus_proxy))
	{
		g_print("on\n");
		return;
	}
	else
	{
		g_print("off\n");
		return;
	}
}

void playlist_repeat_toggle(gint argc, gchar **argv)
{
	audacious_remote_toggle_repeat(dbus_proxy);
}

void playlist_shuffle_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_shuffle(dbus_proxy))
	{
		g_print("on\n");
		return;
	}
	else
	{
		g_print("off\n");
		return;
	}
}

void playlist_shuffle_toggle(gint argc, gchar **argv)
{
	audacious_remote_toggle_shuffle(dbus_proxy);
}

void playlist_tuple_field_data(gint argc, gchar **argv)
{
	gint i;
	gpointer data;

	if (argc < 4)
	{
		g_print("%s: invalid parameters for playlist-tuple-data.\n", argv[0]);
		g_print("%s: syntax: %s playlist-tuple-data <fieldname> <position>\n", argv[0], argv[0]);
		g_print("%s:   - fieldname example choices: performer, album_name,\n", argv[0]);
		g_print("%s:       track_name, track_number, year, date, genre, comment,\n", argv[0]);
		g_print("%s:       file_name, file_ext, file_path, length, formatter,\n", argv[0]);
		g_print("%s:       custom, mtime\n", argv[0]);
		return;
	}

	i = atoi(argv[3]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[2], i - 1)))
	{
		return;
	}
	
	if (!strcasecmp(argv[2], "track_number") || !strcasecmp(argv[2], "year") || !strcasecmp(argv[2], "length") || !strcasecmp(argv[2], "mtime"))
	{
		if (*(gint *)data > 0)
		{
			g_print("%d\n", *(gint *)data);
		}
		return;
	}

	g_print("%s\n", (gchar *)data);
}

void playqueue_add(gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-add.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-add <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (!(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)))
		audacious_remote_playqueue_add(dbus_proxy, i - 1);
}

void playqueue_remove(gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-remove.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-remove <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (audacious_remote_playqueue_is_queued(dbus_proxy, i - 1))
		audacious_remote_playqueue_remove(dbus_proxy, i - 1);
}

void playqueue_is_queued(gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-is-queued.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-is-queued <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	exit(!(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)));
}

void playqueue_get_position(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-get-position.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-position <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = audacious_remote_get_playqueue_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_get_qposition(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-get-qposition.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-qposition <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playqueue_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = audacious_remote_get_playqueue_queue_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_display(gint argc, gchar **argv)
{
	gint i, ii, position, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;
	
	i = audacious_remote_get_playqueue_length(dbus_proxy);

	g_print("%d queued tracks.\n", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		position = audacious_remote_get_playqueue_queue_position(dbus_proxy, ii);
		songname = audacious_remote_get_playlist_title(dbus_proxy, position);
		frames = audacious_remote_get_playlist_time(dbus_proxy, position);
		length = frames / 1000;
		total += length;

		/* adjust width for multi byte characters */
		column = 60;
		if(songname) {
			p = songname;
			while(*p){
				gint stride;
				stride = g_utf8_next_char(p) - p;
				if(g_unichar_iswide(g_utf8_get_char(p))
#if ( (GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION >= 12) )
				   || g_unichar_iswide_cjk(g_utf8_get_char(p))
#endif
				){
					column += (stride - 2);
				}
				else {
					column += (stride - 1);
				}
				p = g_utf8_next_char(p);
			}
		}

		fmt = g_strdup_printf("%%4d | %%4d | %%-%ds | %%d:%%.2d\n", column);
		g_print(fmt, ii + 1, position + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playqueue_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playqueue_length(dbus_proxy);

	g_print("%d\n", i);
}

void playqueue_clear(gint argc, gchar **argv)
{
	audacious_remote_playqueue_clear(dbus_proxy);
}

void get_volume(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_main_volume(dbus_proxy);

	g_print("%d\n", i);
}

void set_volume(gint argc, gchar **argv)
{
	gint i, current_volume;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for set-volume.\n", argv[0]);
		g_print("%s: syntax: %s set-volume <level>\n", argv[0], argv[0]);
		return;
	}

	current_volume = audacious_remote_get_main_volume(dbus_proxy);
	switch (argv[2][0]) 
	{
		case '+':
		case '-':
			i = current_volume + atoi(argv[2]);
			break;
		default:
			i = atoi(argv[2]);
			break;
	}

	audacious_remote_set_main_volume(dbus_proxy, i);
}

void mainwin_show(gint argc, gchar **argv)
{
	if (argc > 2)
	{
		if (!strncmp(argv[2],"on",2)) {
			audacious_remote_main_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[2],"off",3)) {
			audacious_remote_main_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for mainwin-show.\n",argv[0]);
	g_print("%s: syntax: %s mainwin-show <on/off>\n",argv[0],argv[0]);
}

void playlist_show(gint argc, gchar **argv)
{
	if (argc > 2)
	{
		if (!strncmp(argv[2],"on",2)) {
			audacious_remote_pl_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[2],"off",3)) {
			audacious_remote_pl_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for playlist-show.\n",argv[0]);
	g_print("%s: syntax: %s playlist-show <on/off>\n",argv[0],argv[0]);
}

void equalizer_show(gint argc, gchar **argv)
{
	if (argc > 2)
	{
		if (!strncmp(argv[2],"on",2)) {
			audacious_remote_eq_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[2],"off",3)) {
			audacious_remote_eq_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for equalizer-show.\n",argv[0]);
	g_print("%s: syntax: %s equalizer-show <on/off>\n",argv[0],argv[0]);
}

void show_preferences_window(gint argc, gchar **argv)
{
	audacious_remote_show_prefs_box(dbus_proxy);
}

void show_jtf_window(gint argc, gchar **argv)
{
	audacious_remote_show_jtf_box(dbus_proxy);
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
		if (!g_strcasecmp("<sep>", handlers[i].name))
			g_print("%s%s:\n", i == 0 ? "" : "\n", handlers[i].desc);
		else
			g_print("   %-34s - %s\n", handlers[i].name, handlers[i].desc);
	}

	g_print("\nHandlers may be prefixed with `--' (GNU-style long-options) or not, your choice.\n");
	g_print("Report bugs to http://bugs-meta.atheme.org/\n");
}
