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

#include "aud-dbus.h"

struct commandhandler
{
    char * name;
    void (* handler) (int argc, char * * argv);
    char * desc;
    int args;
};

extern const struct commandhandler handlers[];
extern ObjAudacious * dbus_proxy;

void audtool_report (const char * str, ...);
void audtool_whine (const char * str, ...);
void audtool_whine_args (const char * name, const char * str, ...);
void audtool_whine_tuple_fields (void);

void get_handlers_list (int, char * *);
void get_current_song (int, char * *);
void get_current_song_filename (int, char * *);
void get_current_song_length (int, char * *);
void get_current_song_length_seconds (int, char * *);
void get_current_song_length_frames (int, char * *);
void get_current_song_output_length (int, char * *);
void get_current_song_output_length_seconds (int, char * *);
void get_current_song_output_length_frames (int, char * *);
void get_current_song_bitrate (int, char * *);
void get_current_song_bitrate_kbps (int, char * *);
void get_current_song_frequency (int, char * *);
void get_current_song_frequency_khz (int, char * *);
void get_current_song_channels (int, char * *);
void get_current_song_tuple_field_data (int, char * * argv);
void get_current_song_info (int argc, char * * argv);

void get_volume (int, char * *);
void set_volume (int, char * *);

void select_displayed (int, char * *);
void select_playing (int, char * *);
void playlist_position (int, char * *);
void playlist_advance (int, char * *);
void playlist_auto_advance_status (int, char * *);
void playlist_auto_advance_toggle (int, char * *);
void playlist_reverse (int, char * *);
void playlist_length (int, char * *);
void playlist_song (int, char * *);
void playlist_song_filename (int, char * *);
void playlist_song_length (int, char * *);
void playlist_song_length_seconds (int, char * *);
void playlist_song_length_frames (int, char * *);
void playlist_display (int, char * *);
void playlist_position (int, char * *);
void playlist_jump (int, char * *);
void playlist_add_url_string (int, char * *);
void playlist_delete (int, char * *);
void playlist_clear (int, char * *);
void playlist_repeat_status (int, char * *);
void playlist_repeat_toggle (int, char * *);
void playlist_shuffle_status (int, char * *);
void playlist_shuffle_toggle (int, char * *);
void playlist_stop_after_status (int argc, char * * argv);
void playlist_stop_after_toggle (int argc, char * * argv);
void playlist_tuple_field_data (int, char * * argv);
void playlist_enqueue_to_temp (int argc, char * * argv);
void playlist_ins_url_string (int argc, char * * argv);

void number_of_playlists (int argc, char * * argv);
void current_playlist (int argc, char * * argv);
void set_current_playlist (int argc, char * * argv);
void playlist_title (int argc, char * * argv);
void set_playlist_title (int argc, char * * argv);
void new_playlist (int argc, char * * argv);
void delete_current_playlist (int argc, char * * argv);
void play_current_playlist (int argc, char * * argv);

void playqueue_add (int, char * *);
void playqueue_remove (int, char * *);
void playqueue_is_queued (int, char * *);
void playqueue_get_queue_position (int, char * *);
void playqueue_get_list_position (int, char * *);
void playqueue_display (int, char * *);
void playqueue_length (int, char * *);
void playqueue_clear (int, char * *);

void playback_play (int, char * *);
void playback_pause (int, char * *);
void playback_playpause (int, char * *);
void playback_stop (int, char * *);
void playback_playing (int, char * *);
void playback_paused (int, char * *);
void playback_stopped (int, char * *);
void playback_status (int, char * *);
void playback_seek (int, char * *);
void playback_seek_relative (int, char * *);
void playback_record (int, char * *);
void playback_recording (int, char * *);

void mainwin_show (int, char * *);
void show_preferences_window (int, char * *);
void show_jtf_window (int, char * *);
void show_filebrowser (int, char * *);
void shutdown_audacious_server (int, char * *);
void show_about_window (int, char * *);

void get_version (int argc, char * * argv);
void plugin_is_enabled (int argc, char * * argv);
void plugin_enable (int argc, char * * argv);

void equalizer_get_eq (int argc, char * * argv);
void equalizer_get_eq_preamp (int argc, char * * argv);
void equalizer_get_eq_band (int argc, char * * argv);
void equalizer_set_eq (int argc, char * * argv);
void equalizer_set_eq_preamp (int argc, char * * argv);
void equalizer_set_eq_band (int argc, char * * argv);
void equalizer_active (int argc, char * * argv);

int check_args_playlist_pos (int argc, char * * argv);

#endif
