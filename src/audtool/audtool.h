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

#include <mowgli.h>
#include <audacious/dbus.h>

extern mowgli_error_context_t *e;
extern DBusGProxy *dbus_proxy;

struct commandhandler {
	gchar *name;
	void (*handler)(gint argc, gchar **argv);
	gchar *desc;
	gint args;
}; 

extern struct commandhandler handlers[];

extern void get_handlers_list(gint, gchar **);
extern void get_current_song(gint, gchar **);
extern void get_current_song_filename(gint, gchar **);
extern void get_current_song_length(gint, gchar **);
extern void get_current_song_length_seconds(gint, gchar **);
extern void get_current_song_length_frames(gint, gchar **);
extern void get_current_song_output_length(gint, gchar **);
extern void get_current_song_output_length_seconds(gint, gchar **);
extern void get_current_song_output_length_frames(gint, gchar **);
extern void get_current_song_bitrate(gint, gchar **);
extern void get_current_song_bitrate_kbps(gint, gchar **);
extern void get_current_song_frequency(gint, gchar **);
extern void get_current_song_frequency_khz(gint, gchar **);
extern void get_current_song_channels(gint, gchar **);
extern void get_current_song_tuple_field_data(gint, gchar **argv);
extern void get_volume(gint, gchar **);
extern void set_volume(gint, gchar **);
extern void playlist_position(gint, gchar **);
extern void playlist_advance(gint, gchar **);
extern void playlist_reverse(gint, gchar **);
extern void playlist_length(gint, gchar **);
extern void playlist_song(gint, gchar **);
extern void playlist_song_filename(gint, gchar **);
extern void playlist_song_length(gint, gchar **);
extern void playlist_song_length_seconds(gint, gchar **);
extern void playlist_song_length_frames(gint, gchar **);
extern void playlist_display(gint, gchar **);
extern void playlist_position(gint, gchar **);
extern void playlist_jump(gint, gchar **);
extern void playlist_add_url_string(gint, gchar **);
extern void playlist_delete(gint, gchar **);
extern void playlist_clear(gint, gchar **);
extern void playlist_repeat_status(gint, gchar **);
extern void playlist_repeat_toggle(gint, gchar **);
extern void playlist_shuffle_status(gint, gchar **);
extern void playlist_shuffle_toggle(gint, gchar **);
extern void playlist_tuple_field_data(gint, gchar **argv);
extern void playqueue_add(gint, gchar **);
extern void playqueue_remove(gint, gchar **);
extern void playqueue_is_queued(gint, gchar **);
extern void playqueue_get_position(gint, gchar **);
extern void playqueue_get_qposition(gint, gchar **);
extern void playqueue_display(gint, gchar **);
extern void playqueue_length(gint, gchar **);
extern void playqueue_clear(gint, gchar **);
extern void playback_play(gint, gchar **);
extern void playback_pause(gint, gchar **);
extern void playback_playpause(gint, gchar **);
extern void playback_stop(gint, gchar **);
extern void playback_playing(gint, gchar **);
extern void playback_paused(gint, gchar **);
extern void playback_stopped(gint, gchar **);
extern void playback_status(gint, gchar **);
extern void playback_seek(gint, gchar **);
extern void playback_seek_relative(gint, gchar **);
extern void mainwin_show(gint, gchar **);
extern void playlist_show(gint, gchar **);
extern void equalizer_show(gint, gchar **);
extern void show_preferences_window(gint, gchar **);
extern void show_jtf_window(gint, gchar **);
extern void shutdown_audacious_server(gint, gchar **);

#endif
