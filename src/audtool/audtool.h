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

#ifndef AUDTOOL_H
#define AUDTOOL_H

struct commandhandler {
	gchar *name;
	void (*handler)(DBusGProxy *session, gint argc, gchar **argv);
	gchar *desc;
}; 

extern void get_handlers_list(DBusGProxy *, gint, gchar **);
extern void get_current_song(DBusGProxy *, gint, gchar **);
extern void get_current_song_filename(DBusGProxy *, gint, gchar **);
extern void get_current_song_length(DBusGProxy *, gint, gchar **);
extern void get_current_song_length_seconds(DBusGProxy *, gint, gchar **);
extern void get_current_song_length_frames(DBusGProxy *, gint, gchar **);
extern void get_current_song_output_length(DBusGProxy *, gint, gchar **);
extern void get_current_song_output_length_seconds(DBusGProxy *, gint, gchar **);
extern void get_current_song_output_length_frames(DBusGProxy *, gint, gchar **);
extern void get_current_song_bitrate(DBusGProxy *, gint, gchar **);
extern void get_current_song_bitrate_kbps(DBusGProxy *, gint, gchar **);
extern void get_current_song_frequency(DBusGProxy *, gint, gchar **);
extern void get_current_song_frequency_khz(DBusGProxy *, gint, gchar **);
extern void get_current_song_channels(DBusGProxy *, gint, gchar **);
extern void get_current_song_tuple_field_data(DBusGProxy *, gint, gchar **argv);
extern void get_volume(DBusGProxy *, gint, gchar **);
extern void set_volume(DBusGProxy *, gint, gchar **);
extern void playlist_position(DBusGProxy *, gint, gchar **);
extern void playlist_advance(DBusGProxy *, gint, gchar **);
extern void playlist_reverse(DBusGProxy *, gint, gchar **);
extern void playlist_length(DBusGProxy *, gint, gchar **);
extern void playlist_song(DBusGProxy *, gint, gchar **);
extern void playlist_song_filename(DBusGProxy *, gint, gchar **);
extern void playlist_song_length(DBusGProxy *, gint, gchar **);
extern void playlist_song_length_seconds(DBusGProxy *, gint, gchar **);
extern void playlist_song_length_frames(DBusGProxy *, gint, gchar **);
extern void playlist_display(DBusGProxy *, gint, gchar **);
extern void playlist_position(DBusGProxy *, gint, gchar **);
extern void playlist_jump(DBusGProxy *, gint, gchar **);
extern void playlist_add_url_string(DBusGProxy *, gint, gchar **);
extern void playlist_delete(DBusGProxy *, gint, gchar **);
extern void playlist_clear(DBusGProxy *, gint, gchar **);
extern void playlist_repeat_status(DBusGProxy *, gint, gchar **);
extern void playlist_repeat_toggle(DBusGProxy *, gint, gchar **);
extern void playlist_shuffle_status(DBusGProxy *, gint, gchar **);
extern void playlist_shuffle_toggle(DBusGProxy *, gint, gchar **);
extern void playlist_tuple_field_data(DBusGProxy *, gint, gchar **argv);
extern void playqueue_add(DBusGProxy *, gint, gchar **);
extern void playqueue_remove(DBusGProxy *, gint, gchar **);
extern void playqueue_is_queued(DBusGProxy *, gint, gchar **);
extern void playqueue_get_position(DBusGProxy *, gint, gchar **);
extern void playqueue_get_qposition(DBusGProxy *, gint, gchar **);
extern void playqueue_display(DBusGProxy *, gint, gchar **);
extern void playqueue_length(DBusGProxy *, gint, gchar **);
extern void playqueue_clear(DBusGProxy *, gint, gchar **);
extern void playback_play(DBusGProxy *, gint, gchar **);
extern void playback_pause(DBusGProxy *, gint, gchar **);
extern void playback_playpause(DBusGProxy *, gint, gchar **);
extern void playback_stop(DBusGProxy *, gint, gchar **);
extern void playback_playing(DBusGProxy *, gint, gchar **);
extern void playback_paused(DBusGProxy *, gint, gchar **);
extern void playback_stopped(DBusGProxy *, gint, gchar **);
extern void playback_status(DBusGProxy *, gint, gchar **);
extern void playback_seek(DBusGProxy *, gint, gchar **);
extern void playback_seek_relative(DBusGProxy *, gint, gchar **);
extern void mainwin_show(DBusGProxy *, gint, gchar **);
extern void playlist_show(DBusGProxy *, gint, gchar **);
extern void equalizer_show(DBusGProxy *, gint, gchar **);
extern void show_preferences_window(DBusGProxy *, gint, gchar **);
extern void show_jtf_window(DBusGProxy *, gint, gchar **);
extern void shutdown_audacious_server(DBusGProxy *, gint, gchar **);

#endif
