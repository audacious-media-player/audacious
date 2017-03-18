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
    /* update flags */
    enum {
        QueueChanged  = (1 << 0),
        DelayedUpdate = (1 << 1)
    };

    /* scan status */
    enum ScanStatus {
        NotScanning,
        ScanActive,
        ScanEnding
    };

    struct CompareData {
        Playlist::StringCompareFunc filename_compare;
        Playlist::TupleCompareFunc tuple_compare;
    };

    PlaylistData (Playlist::ID * m_id, const char * title);
    ~PlaylistData ();

    void set_entry_tuple (PlaylistEntry * entry, Tuple && tuple);
    void number_entries (int at, int length);

    PlaylistEntry * lookup_entry (int i);
    const PlaylistEntry * lookup_entry (int i) const;

    void queue_update (Playlist::UpdateLevel level, int at, int count, int flags = 0);
    void cancel_updates ();
    void swap_updates ();

    void insert_items (int at, Index<PlaylistAddItem> && items);
    void remove_entries (int at, int number, bool & position_changed);

    int position () const;
    int focus () const;

    bool entry_selected (int entry_num) const;
    int n_selected (int at, int number) const;

    void set_focus (int entry_num);

    void select_entry (int entry_num, bool selected);
    void select_all (bool selected);
    int shift_entries (int entry_num, int distance);
    void remove_selected (bool & position_changed, int & next_song_hint);

    void sort (const CompareData & data);
    void sort_selected (const CompareData & data);

    void reverse_order ();
    void randomize_order ();
    void reverse_selected ();
    void randomize_selected ();

    int queue_get_entry (int at) const;
    int queue_find_entry (int entry_num) const;

    void queue_insert (int at, int entry_num);
    void queue_insert_selected (int pos);
    void queue_remove (int at, int number);
    void queue_remove_selected ();

    PlaylistEntry * queue_pop ();
    void set_position (PlaylistEntry * entry, bool update_shuffle);

    bool shuffle_prev ();
    bool shuffle_next ();
    void shuffle_reset ();

    PlaylistEntry * find_unselected_focus ();

    Playlist::ID * id () const { return m_id; }

    int n_entries () const { return entries.len (); }
    int n_queued () const { return m_queued.len (); }

    int64_t total_length () const { return m_total_length; }
    int64_t selected_length () const { return m_selected_length; }

    const Playlist::Update & last_update () const { return m_last_update; }
    bool update_pending () const { return m_next_update.level != Playlist::NoUpdate; }

    Playlist::ID * m_id;
    bool modified;
    ScanStatus scan_status;
    String filename, title;
    Index<SmartPtr<PlaylistEntry>> entries;
    PlaylistEntry * m_position, * m_focus;
    int m_selected_count;
    int m_last_shuffle_num;
    Index<PlaylistEntry *> m_queued;
    int64_t m_total_length, m_selected_length;
    Playlist::Update m_last_update, m_next_update;
    int resume_time;
};

/* callbacks or "signals" (in the QObject sense) */
void pl_signal_entry_deleted (PlaylistEntry * entry);
void pl_signal_update_queued (Playlist::ID * id, Playlist::UpdateLevel level, int flags);
void pl_signal_playlist_deleted (Playlist::ID * id);

#endif // PLAYLIST_DATA_H
