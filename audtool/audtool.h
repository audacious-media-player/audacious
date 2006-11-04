/*  Audtool -- Audacious scripting tool
 *  Copyright (c) 2005-2006  William Pitcock, George Averill
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

#ifndef AUDTOOL_H
#define AUDTOOL_H

struct commandhandler {
	gchar *name;
	void (*handler)(gint session, gint argc, gchar **argv);
	gchar *desc;
}; 

extern void get_handlers_list(gint, gint, gchar **);
extern void get_current_song(gint, gint, gchar **);
extern void get_current_song_filename(gint, gint, gchar **);
extern void get_current_song_length(gint, gint, gchar **);
extern void get_current_song_length_seconds(gint, gint, gchar **);
extern void get_current_song_length_frames(gint, gint, gchar **);
extern void get_current_song_output_length(gint, gint, gchar **);
extern void get_current_song_output_length_seconds(gint, gint, gchar **);
extern void get_current_song_output_length_frames(gint, gint, gchar **);
extern void get_current_song_bitrate(gint, gint, gchar **);
extern void get_current_song_bitrate_kbps(gint, gint, gchar **);
extern void get_current_song_frequency(gint, gint, gchar **);
extern void get_current_song_frequency_khz(gint, gint, gchar **);
extern void get_current_song_channels(gint, gint, gchar **);
extern void get_volume(gint, gint, gchar **);
extern void set_volume(gint, gint, gchar **);
extern void playlist_position(gint, gint, gchar **);
extern void playlist_advance(gint, gint, gchar **);
extern void playlist_reverse(gint, gint, gchar **);
extern void playlist_length(gint, gint, gchar **);
extern void playlist_song(gint, gint, gchar **);
extern void playlist_song_filename(gint, gint, gchar **);
extern void playlist_song_length(gint, gint, gchar **);
extern void playlist_song_length_seconds(gint, gint, gchar **);
extern void playlist_song_length_frames(gint, gint, gchar **);
extern void playlist_display(gint, gint, gchar **);
extern void playlist_position(gint, gint, gchar **);
extern void playlist_jump(gint, gint, gchar **);
extern void playlist_add_url_string(gint, gint, gchar **);
extern void playlist_delete(gint, gint, gchar **);
extern void playlist_clear(gint, gint, gchar **);
extern void playlist_repeat_status(gint, gint, gchar **);
extern void playlist_repeat_toggle(gint, gint, gchar **);
extern void playlist_shuffle_status(gint, gint, gchar **);
extern void playlist_shuffle_toggle(gint, gint, gchar **);
extern void playqueue_add(gint, gint, gchar **);
extern void playqueue_remove(gint, gint, gchar **);
extern void playqueue_is_queued(gint, gint, gchar **);
extern void playqueue_get_position(gint, gint, gchar **);
extern void playqueue_get_qposition(gint, gint, gchar **);
extern void playqueue_display(gint, gint, gchar **);
extern void playqueue_length(gint, gint, gchar **);
extern void playqueue_clear(gint, gint, gchar **);
extern void playback_play(gint, gint, gchar **);
extern void playback_pause(gint, gint, gchar **);
extern void playback_playpause(gint, gint, gchar **);
extern void playback_stop(gint, gint, gchar **);
extern void playback_playing(gint, gint, gchar **);
extern void playback_paused(gint, gint, gchar **);
extern void playback_stopped(gint, gint, gchar **);
extern void playback_status(gint, gint, gchar **);
extern void playback_seek(gint, gint, gchar **);
extern void playback_seek_relative(gint, gint, gchar **);
extern void show_preferences_window(gint, gint, gchar **);
extern void show_jtf_window(gint, gint, gchar **);
extern void shutdown_audacious_server(gint, gint, gchar **);

#endif
