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
#include "scanner.h"

class TupleCompiler;
struct PlaylistEntry;

class PlaylistData
{
public:
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

    PlaylistEntry * entry_at (int i);
    const PlaylistEntry * entry_at (int i) const;

    String entry_filename (int i) const;
    PluginHandle * entry_decoder (int i, String * error = nullptr) const;
    Tuple entry_tuple (int i, String * error = nullptr) const;

    void cancel_updates ();
    void swap_updates (bool & position_changed);

    void insert_items (int at, Index<PlaylistAddItem> && items);
    void remove_entries (int at, int number);

    int position () const;
    int focus () const;

    bool entry_selected (int entry_num) const;
    int n_selected (int at, int number) const;

    void set_focus (int entry_num);

    void select_entry (int entry_num, bool selected);
    void select_all (bool selected);
    int shift_entries (int entry_num, int distance);
    void remove_selected ();

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

    void set_position (int entry_num);

    bool prev_song ();
    bool next_song (bool repeat);

    int next_unscanned_entry (int entry_num) const;
    bool entry_needs_rescan (PlaylistEntry * entry, bool need_decoder, bool need_tuple);
    ScanRequest * create_scan_request (PlaylistEntry * entry,
     ScanRequest::Callback callback, int extra_flags);
    void update_entry_from_scan (PlaylistEntry * entry, ScanRequest * request, int update_flags);
    void update_playback_entry (Tuple && tuple);

    void reformat_titles ();
    void reset_tuples (bool selected_only);
    void reset_tuple_of_file (const char * filename);

    Playlist::ID * id () const { return m_id; }

    int n_entries () const { return m_entries.len (); }
    int n_queued () const { return m_queued.len (); }

    int64_t total_length () const { return m_total_length; }
    int64_t selected_length () const { return m_selected_length; }

    const Playlist::Update & last_update () const { return m_last_update; }
    bool update_pending () const { return m_next_update.level != Playlist::NoUpdate; }

    static void update_formatter ();
    static void cleanup_formatter ();

private:
    static void delete_entry (PlaylistEntry * entry);
    typedef SmartPtr<PlaylistEntry, delete_entry> EntryPtr;

    void number_entries (int at, int length);
    void set_entry_tuple (PlaylistEntry * entry, Tuple && tuple);
    void queue_update (Playlist::UpdateLevel level, int at, int count, int flags = 0);
    void queue_position_change ();

    static void sort_entries (Index<EntryPtr> & entries, const CompareData & data);

    void set_position (PlaylistEntry * entry, bool update_shuffle);

    bool shuffle_prev ();
    bool shuffle_next ();
    void shuffle_reset ();

    bool next_song_with_hint (bool repeat, int hint);

    PlaylistEntry * find_unselected_focus ();
    PlaylistEntry * queue_pop ();

public:
    bool modified;
    ScanStatus scan_status;
    String filename, title;
    int resume_time;

private:
    Playlist::ID * m_id;
    Index<EntryPtr> m_entries;
    PlaylistEntry * m_position, * m_focus;
    int m_selected_count;
    int m_last_shuffle_num;
    Index<PlaylistEntry *> m_queued;
    int64_t m_total_length, m_selected_length;
    Playlist::Update m_last_update, m_next_update;
    bool m_position_changed;
};

/* callbacks or "signals" (in the QObject sense) */
void pl_signal_entry_deleted (PlaylistEntry * entry);
void pl_signal_position_changed (Playlist::ID * id);
void pl_signal_update_queued (Playlist::ID * id, Playlist::UpdateLevel level, int flags);
void pl_signal_rescan_needed (Playlist::ID * id);
void pl_signal_playlist_deleted (Playlist::ID * id);

#endif // PLAYLIST_DATA_H
