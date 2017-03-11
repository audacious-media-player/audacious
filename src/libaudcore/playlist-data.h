/*
 * playlist-data.h
 * Copyright 2017 John Lindgren
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

#ifndef PLAYLIST_DATA_H
#define PLAYLIST_DATA_H

#include "playlist.h"

class TupleCompiler;

struct PlaylistEntry
{
    PlaylistEntry (PlaylistAddItem && item);
    ~PlaylistEntry ();

    void format ();
    void set_tuple (Tuple && new_tuple);

    static void update_formatting ();
    static void cleanup ();

    String filename;
    PluginHandle * decoder;
    Tuple tuple;
    String error;
    int number;
    int length;
    int shuffle_num;
    bool selected, queued;

private:
    static TupleCompiler s_compiler;
    static bool s_use_fallbacks;
};

struct PlaylistData
{
    PlaylistData (Playlist::ID * id, const char * title);
    ~PlaylistData ();

    void set_entry_tuple (PlaylistEntry * entry, Tuple && tuple);
    void number_entries (int at, int length);
    PlaylistEntry * lookup_entry (int i);

    void set_position (PlaylistEntry * entry, bool update_shuffle);
    PlaylistEntry * find_unselected_focus ();

    Playlist::ID * id;
    int number;
    bool modified, scanning, scan_ending;
    String filename, title;
    Index<SmartPtr<PlaylistEntry>> entries;
    PlaylistEntry * position, * focus;
    int selected_count;
    int last_shuffle_num;
    Index<PlaylistEntry *> queued;
    int64_t total_length, selected_length;
    Playlist::Update next_update, last_update;
    int resume_time;
};

/* callbacks or "signals" (in the QObject sense) */
void pl_signal_entry_deleted (PlaylistEntry * entry);
void pl_signal_playlist_deleted (Playlist::ID * id);

#endif // PLAYLIST_DATA_H
