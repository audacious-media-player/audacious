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

/* extended handle for accessing internal playlist functions */
class PlaylistEx : public Playlist
{
public:
    PlaylistEx (Playlist::ID * id = nullptr) :
        Playlist (id) {}
    PlaylistEx (Playlist playlist) :
        Playlist (playlist) {}

    int stamp () const;

    static Playlist insert_with_stamp (int at, int stamp);

    bool get_modified () const;
    void set_modified (bool modified) const;

    bool insert_flat_playlist (const char * filename) const;
    void insert_flat_items (int at, Index<PlaylistAddItem> && items) const;
};

/* playlist.cc */
void playlist_init ();
void playlist_enable_scan (bool enable);
void playlist_clear_updates ();
void playlist_end ();

void playlist_load_state ();
void playlist_save_state ();

DecodeInfo playback_entry_read (int serial);
void playback_entry_set_tuple (int serial, Tuple && tuple);

/* playlist-cache.cc */
void playlist_cache_load (Index<PlaylistAddItem> & items);
void playlist_cache_clear (void * = nullptr);

/* playlist-files.cc */
bool playlist_load (const char * filename, String & title, Index<PlaylistAddItem> & items);

/* playlist-utils.cc */
void load_playlists ();
void save_playlists (bool exiting);

#endif
