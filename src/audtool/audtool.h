/*
 * audtool.h
 * Copyright 2005-2011 William Pitcock, George Averill, Giacomo Lozito,
 *                     Yoshiki Yazawa, Matti Hämäläinen, and John Lindgren
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

#ifndef AUDTOOL_H
#define AUDTOOL_H

#include <audacious/dbus.h>

extern DBusGProxy *dbus_proxy;

struct commandhandler {
	gchar *name;
	void (*handler)(gint argc, gchar **argv);
	gchar *desc;
	gint args;
};

extern struct commandhandler handlers[];

extern void audtool_report(const gchar *str, ...);
extern void audtool_whine(const gchar *str, ...);
extern void audtool_whine_args(const gchar *name, const gchar *str, ...);
extern void audtool_whine_tuple_fields(void);

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
extern void get_current_song_info(gint argc, gchar **argv);

extern void get_volume(gint, gchar **);
extern void set_volume(gint, gchar **);

extern void playlist_position(gint, gchar **);
extern void playlist_advance(gint, gchar **);
extern void playlist_auto_advance_status(gint, gchar **);
extern void playlist_auto_advance_toggle(gint, gchar **);
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
void playlist_stop_after_status (gint argc, gchar * * argv);
void playlist_stop_after_toggle (gint argc, gchar * * argv);
extern void playlist_tuple_field_data(gint, gchar **argv);
extern void playlist_enqueue_to_temp(gint argc, gchar **argv);
extern void playlist_ins_url_string(gint argc, gchar **argv);
extern void playlist_title(gint, gchar **);

extern void playqueue_add(gint, gchar **);
extern void playqueue_remove(gint, gchar **);
extern void playqueue_is_queued(gint, gchar **);
extern void playqueue_get_queue_position(gint, gchar **);
extern void playqueue_get_list_position(gint, gchar **);
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
extern void show_preferences_window(gint, gchar **);
extern void show_jtf_window(gint, gchar **);
extern void show_filebrowser(gint, gchar **);
extern void shutdown_audacious_server(gint, gchar **);
extern void show_about_window(gint, gchar **);

extern void toggle_aot(gint argc, gchar **argv);
extern void get_skin(gint argc, gchar **argv);
extern void set_skin(gint argc, gchar **argv);
extern void get_version(gint argc, gchar **argv);

extern void equalizer_get_eq(gint argc, gchar **argv);
extern void equalizer_get_eq_preamp(gint argc, gchar **argv);
extern void equalizer_get_eq_band(gint argc, gchar **argv);
extern void equalizer_set_eq(gint argc, gchar **argv);
extern void equalizer_set_eq_preamp(gint argc, gchar **argv);
extern void equalizer_set_eq_band(gint argc, gchar **argv);
extern void equalizer_active(gint argc, gchar **argv);

extern gint check_args_playlist_pos(gint argc, gchar **argv);

#endif
