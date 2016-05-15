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
#include "vfs.h"

class InputPlugin;

struct DecodeInfo
{
    String filename;
    InputPlugin * ip = nullptr;
    VFSFile file;
    String error;
};

/* playlist.cc */
void playlist_init ();
void playlist_enable_scan (bool enable);
void playlist_end ();

void playlist_insert_with_id (int at, int id);
void playlist_set_modified (int playlist, bool modified);
bool playlist_get_modified (int playlist);

void playlist_load_state ();
void playlist_save_state ();

void playlist_entry_insert_batch_raw (int playlist, int at, Index<PlaylistAddItem> && items);

bool playlist_prev_song (int playlist);
bool playlist_next_song (int playlist, bool repeat);

DecodeInfo playback_entry_read (int serial);
void playback_entry_set_tuple (int serial, Tuple && tuple);

/* playlist-cache.cc */
void playlist_cache_load (Index<PlaylistAddItem> & items);
void playlist_cache_clear ();

/* playlist-files.cc */
bool playlist_load (const char * filename, String & title, Index<PlaylistAddItem> & items);
bool playlist_insert_playlist_raw (int list, int at, const char * filename);

/* playlist-utils.cc */
void load_playlists ();
void save_playlists (bool exiting);

#endif
