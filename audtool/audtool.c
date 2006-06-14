/*  Audtool -- Audacious scripting tool
 *  Copyright (c) 2005-2006  George Averill, William Pitcock
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <glib.h>
#include <audacious/beepctrl.h>
#include "audtool.h"

struct commandhandler handlers[] = {
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
	{"<sep>", NULL, NULL},
	{"playlist-advance", playlist_advance, "go to the next song in the playlist"},
	{"playlist-reverse", playlist_reverse, "go to the previous song in the playlist"},
	{"playlist-addurl", playlist_add_url_string, "adds a url to the playlist"},
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
	{"<sep>", NULL, NULL},
	{"playqueue-add", playqueue_add, "adds a song to the playqueue"},
	{"playqueue-remove", playqueue_remove, "removes a song from the playqueue"},
	{"playqueue-is-queued", playqueue_is_queued, "returns OK if a song is queued"},
	{"playqueue-get-position", playqueue_get_position, "returns the queue position of a song in the playlist"},
	{"playqueue-get-qposition", playqueue_get_qposition, "returns the playlist position of a song in the queue"},
	{"playqueue-length", playqueue_length, "returns the length of the playqueue"},
	{"playqueue-display", playqueue_display, "returns a list of currently-queued songs"},
	{"playqueue-clear", playqueue_clear, "clears the playqueue"},
	{"<sep>", NULL, NULL},
	{"playback-play", playback_play, "starts/unpauses song playback"},
	{"playback-pause", playback_pause, "(un)pauses song playback"},
	{"playback-playpause", playback_playpause, "plays/(un)pauses song playback"},
	{"playback-stop", playback_stop, "stops song playback"},
	{"playback-playing", playback_playing, "returns OK if audacious is playing"},
	{"playback-paused", playback_paused, "returns OK if audacious is paused"},
	{"playback-stopped", playback_stopped, "returns OK if audacious is stopped"},
	{"playback-status", playback_status, "returns the playback status"},
	{"<sep>", NULL, NULL},
	{"get-volume", get_volume, "returns the current volume level in percent"},
	{"set-volume", set_volume, "sets the current volume level in percent"},
	{"<sep>", NULL, NULL},
	{"preferences", show_preferences_window, "shows/hides the preferences window"},
	{"jumptofile", show_jtf_window, "shows the jump to file window"},
	{"shutdown", shutdown_audacious_server, "shuts down audacious"},
	{"<sep>", NULL, NULL},
	{"list-handlers", get_handlers_list, "shows handlers list"},
	{"help", get_handlers_list, "shows handlers list"},
	{NULL, NULL, NULL}
};

gint main(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: usage: %s <command>\n", argv[0], argv[0]);
		g_print("%s: use `%s help' to get a listing of available commands.\n",
			argv[0], argv[0]);
		exit(0);
	}

	if (!xmms_remote_is_running(0) && g_strcasecmp("help", argv[1])
		&& g_strcasecmp("list-handlers", argv[1]))
	{
		g_print("%s: audacious server is not running!\n", argv[0]);
		exit(0);
	}

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if ((!g_strcasecmp(handlers[i].name, argv[1]) ||
		     !g_strcasecmp(g_strconcat("--", handlers[i].name, NULL), argv[1]))
		    && g_strcasecmp("<sep>", handlers[i].name))
  		{
 			handlers[i].handler(0, argc, argv);
			exit(0);
		}
	}

	g_print("%s: invalid command '%s'\n", argv[0], argv[1]);
	g_print("%s: use `%s help' to get a listing of available commands.\n", argv[0], argv[0]);

	return 0;
}

/*** MOVE TO HANDLERS.C ***/

void get_current_song(gint session, gint argc, gchar **argv)
{
	gint playpos = xmms_remote_get_playlist_pos(session);
	gchar *song = xmms_remote_get_playlist_title(session, playpos);

	if (!song)
	{
		g_print("No song playing.\n");
		return;
	}

	g_print("%s\n", song);
}

void get_current_song_filename(gint session, gint argc, gchar **argv)
{
	gint playpos = xmms_remote_get_playlist_pos(session);

	g_print("%s\n", xmms_remote_get_playlist_file(session, playpos));
}

void get_current_song_output_length(gint session, gint argc, gchar **argv)
{
	gint frames = xmms_remote_get_output_time(session);
	gint length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void get_current_song_output_length_seconds(gint session, gint argc, gchar **argv)
{
	gint frames = xmms_remote_get_output_time(session);
	gint length = frames / 1000;

	g_print("%d\n", length);
}

void get_current_song_output_length_frames(gint session, gint argc, gchar **argv)
{
	gint frames = xmms_remote_get_output_time(session);

	g_print("%d\n", frames);
}

void get_current_song_length(gint session, gint argc, gchar **argv)
{
	gint playpos = xmms_remote_get_playlist_pos(session);
	gint frames = xmms_remote_get_playlist_time(session, playpos);
	gint length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void get_current_song_length_seconds(gint session, gint argc, gchar **argv)
{
	gint playpos = xmms_remote_get_playlist_pos(session);
	gint frames = xmms_remote_get_playlist_time(session, playpos);
	gint length = frames / 1000;

	g_print("%d\n", length);
}

void get_current_song_length_frames(gint session, gint argc, gchar **argv)
{
	gint playpos = xmms_remote_get_playlist_pos(session);
	gint frames = xmms_remote_get_playlist_time(session, playpos);

	g_print("%d\n", frames);
}

void get_current_song_bitrate(gint session, gint argc, gchar **argv)
{
	gint rate, freq, nch;

	xmms_remote_get_info(session, &rate, &freq, &nch);

	g_print("%d\n", rate);
}

void get_current_song_bitrate_kbps(gint session, gint argc, gchar **argv)
{
	gint rate, freq, nch;

	xmms_remote_get_info(session, &rate, &freq, &nch);

	g_print("%d\n", rate / 1000);
}

void get_current_song_frequency(gint session, gint argc, gchar **argv)
{
	gint rate, freq, nch;

	xmms_remote_get_info(session, &rate, &freq, &nch);

	g_print("%d\n", freq);
}

void get_current_song_frequency_khz(gint session, gint argc, gchar **argv)
{
	gint rate, freq, nch;

	xmms_remote_get_info(session, &rate, &freq, &nch);

	g_print("%0.1f\n", (gfloat) freq / 1000);
}

void get_current_song_channels(gint session, gint argc, gchar **argv)
{
	gint rate, freq, nch;

	xmms_remote_get_info(session, &rate, &freq, &nch);

	g_print("%d\n", nch);
}

void playlist_reverse(gint session, gint argc, gchar **argv)
{
	xmms_remote_playlist_prev(session);
}

void playlist_advance(gint session, gint argc, gchar **argv)
{
	xmms_remote_playlist_next(session);
}

void playback_play(gint session, gint argc, gchar **argv)
{
	xmms_remote_play(session);
}

void playback_pause(gint session, gint argc, gchar **argv)
{
	xmms_remote_pause(session);
}

void playback_playpause(gint session, gint argc, gchar **argv)
{
	if (xmms_remote_is_playing(session))
	{
		xmms_remote_pause(session);
	}
	else
	{
		xmms_remote_play(session);
	}
}

void playback_stop(gint session, gint argc, gchar **argv)
{
	xmms_remote_stop(session);
}

void playback_playing(gint session, gint argc, gchar **argv)
{
	if (!xmms_remote_is_paused(session))
	{
		exit(!xmms_remote_is_playing(session));
	}
	else
	{
		exit(1);
	}
}

void playback_paused(gint session, gint argc, gchar **argv)
{
	exit(!xmms_remote_is_paused(session));
}

void playback_stopped(gint session, gint argc, gchar **argv)
{
	if (!xmms_remote_is_playing(session) && !xmms_remote_is_paused(session))
	{
		exit(0);
	}
	else
	{
		exit(1);
	}
}

void playback_status(gint session, gint argc, gchar **argv)
{
	if (xmms_remote_is_paused(session))
	{
		g_print("paused\n");
		return;
	}
	else if (xmms_remote_is_playing(session))
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

void playlist_add_url_string(gint session, gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-addurl.\n", argv[0]);
		g_print("%s: syntax: %s playlist-addurl <url>\n", argv[0], argv[0]);
		return;
	}

	xmms_remote_playlist_add_url_string(session, argv[2]);
}

void playlist_length(gint session, gint argc, gchar **argv)
{
	gint i;

	i = xmms_remote_get_playlist_length(session);

	g_print("%d\n", i);
}

void playlist_song(gint session, gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-title.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-title <position>\n", argv[0], argv[0]);
		return;
	}

	gint playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	gchar *song = xmms_remote_get_playlist_title(session, playpos - 1);

	g_print("%s\n", song);
}


void playlist_song_length(gint session, gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length <position>\n", argv[0], argv[0]);
		return;
	}

	gint playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	gint frames = xmms_remote_get_playlist_time(session, playpos - 1);
	gint length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void playlist_song_length_seconds(gint session, gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length-seconds.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-seconds <position>\n", argv[0], argv[0]);
		return;
	}

	gint playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	gint frames = xmms_remote_get_playlist_time(session, playpos - 1);
	gint length = frames / 1000;

	g_print("%d\n", length);
}

void playlist_song_length_frames(gint session, gint argc, gchar **argv)
{
	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-song-length-frames.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-frames <position>\n", argv[0], argv[0]);
		return;
	}

	gint playpos = atoi(argv[2]);

	if (playpos < 1 || playpos > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	gint frames = xmms_remote_get_playlist_time(session, playpos - 1);

	g_print("%d\n", frames);
}

void playlist_display(gint session, gint argc, gchar **argv)
{
	gint i, ii, frames, length, total;
	gchar *songname;
	
	i = xmms_remote_get_playlist_length(session);

	g_print("%d tracks.\n", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		songname = xmms_remote_get_playlist_title(session, ii);
		frames = xmms_remote_get_playlist_time(session, ii);
		length = frames / 1000;
		total += length;

		g_print("%4d | %-60s | %d:%.2d\n",
			ii + 1, songname, length / 60, length % 60);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playlist_position(gint session, gint argc, gchar **argv)
{
	gint i;

	i = xmms_remote_get_playlist_pos(session);

	g_print("%d\n", i + 1);
}

void playlist_song_filename(gint session, gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-filename.\n", argv[0]);
		g_print("%s: syntax: %s playlist-filename <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	g_print("%s\n", xmms_remote_get_playlist_file(session, i - 1));
}

void playlist_jump(gint session, gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-jump.\n", argv[0]);
		g_print("%s: syntax: %s playlist-jump <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	xmms_remote_set_playlist_pos(session, i - 1);
}

void playlist_clear(gint session, gint argc, gchar **argv)
{
	xmms_remote_playlist_clear(session);
}

void playlist_repeat_status(gint session, gint argc, gchar **argv)
{
	if (xmms_remote_is_repeat(session))
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

void playlist_repeat_toggle(gint session, gint argc, gchar **argv)
{
	xmms_remote_toggle_repeat(session);
}

void playlist_shuffle_status(gint session, gint argc, gchar **argv)
{
	if (xmms_remote_is_shuffle(session))
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

void playlist_shuffle_toggle(gint session, gint argc, gchar **argv)
{
	xmms_remote_toggle_shuffle(session);
}

void playqueue_add(gint session, gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-add.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-add <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (!(xmms_remote_playqueue_is_queued(session, i - 1)))
		xmms_remote_playqueue_add(session, i - 1);
}

void playqueue_remove(gint session, gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-remove.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-remove <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (xmms_remote_playqueue_is_queued(session, i - 1))
		xmms_remote_playqueue_remove(session, i - 1);
}

void playqueue_is_queued(gint session, gint argc, gchar **argv)
{
	gint i;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-is-queued.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-is-queued <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	exit(!(xmms_remote_playqueue_is_queued(session, i - 1)));
}

void playqueue_get_position(gint session, gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-get-position.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-position <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playlist_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = xmms_remote_get_playqueue_position(session, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_get_qposition(gint session, gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playqueue-get-qposition.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-qposition <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > xmms_remote_get_playqueue_length(session))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = xmms_remote_get_playqueue_queue_position(session, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_display(gint session, gint argc, gchar **argv)
{
	gint i, ii, position, frames, length, total;
	gchar *songname;
	
	i = xmms_remote_get_playqueue_length(session);

	g_print("%d queued tracks.\n", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		position = xmms_remote_get_playqueue_queue_position(session, ii);
		songname = xmms_remote_get_playlist_title(session, position);
		frames = xmms_remote_get_playlist_time(session, position);
		length = frames / 1000;
		total += length;

		g_print("%4d | %4d | %-60s | %d:%.2d\n",
			ii + 1, position + 1, songname, length / 60, length % 60);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playqueue_length(gint session, gint argc, gchar **argv)
{
	gint i;

	i = xmms_remote_get_playqueue_length(session);

	g_print("%d\n", i);
}

void playqueue_clear(gint session, gint argc, gchar **argv)
{
	xmms_remote_playqueue_clear(session);
}

void get_volume(gint session, gint argc, gchar **argv)
{
	gint i;

	i = xmms_remote_get_main_volume(session);

	g_print("%d\n", i);
}

void set_volume(gint session, gint argc, gchar **argv)
{
	gint i, current_volume;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for set-volume.\n", argv[0]);
		g_print("%s: syntax: %s set-volume <level>\n", argv[0], argv[0]);
		return;
	}

	current_volume = xmms_remote_get_main_volume(session);
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

	xmms_remote_set_main_volume(session, i);
}

void show_preferences_window(gint session, gint argc, gchar **argv)
{
	xmms_remote_show_prefs_box(session);
}

void show_jtf_window(gint session, gint argc, gchar **argv)
{
	xmms_remote_show_jtf_box(session);
}

void shutdown_audacious_server(gint session, gint argc, gchar **argv)
{
	xmms_remote_quit(session);
}

void get_handlers_list(gint session, gint argc, gchar **argv)
{
	gint i;

	g_print("Available handlers:\n\n");

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if (!g_strcasecmp("<sep>", handlers[i].name))
			g_print("\n");
		else
			g_print("   %-34s - %s\n", handlers[i].name, handlers[i].desc);
	}

	g_print("\nHandlers may be prefixed with `--' (GNU-style long-options) or not, your choice.\n");
	g_print("Report bugs to http://bugs.audacious-media-player.org.\n");
}
