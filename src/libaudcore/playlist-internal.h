/*
 * playlist-internal.h
 * Copyright 2014 John Lindgren
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

#ifndef LIBAUDCORE_PLAYLIST_INTERNAL_H
#define LIBAUDCORE_PLAYLIST_INTERNAL_H

#include "playlist.h"

/* playlist.c */
void playlist_init (void);
void playlist_end (void);

void playlist_insert_with_id (int at, int id);
void playlist_set_modified (int playlist, bool_t modified);
bool_t playlist_get_modified (int playlist);

void playlist_load_state (void);
void playlist_save_state (void);

void playlist_entry_insert_batch_raw (int playlist, int at, Index<PlaylistAddItem> && items);

bool_t playlist_prev_song (int playlist);
bool_t playlist_next_song (int playlist, bool_t repeat);

int playback_entry_get_position (void);
String playback_entry_get_filename (void);
PluginHandle * playback_entry_get_decoder (void);
Tuple * playback_entry_get_tuple (void);
String playback_entry_get_title (void);
int playback_entry_get_length (void);

void playback_entry_set_tuple (Tuple * tuple);

/* playlist-files.c */
bool_t playlist_load (const char * filename, String & title, Index<PlaylistAddItem> & items);
bool_t playlist_insert_playlist_raw (int list, int at, const char * filename);

/* playlist-utils.c */
void load_playlists (void);
void save_playlists (bool_t exiting);

#endif
