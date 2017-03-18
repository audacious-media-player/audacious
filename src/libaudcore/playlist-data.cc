/*
 * playlist-data.cc
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

#include "playlist-data.h"

#include "runtime.h"
#include "tuple-compiler.h"

TupleCompiler PlaylistEntry::s_compiler;
bool PlaylistEntry::s_use_fallbacks = false;

void PlaylistEntry::update_formatting () // static
{
    s_compiler.compile (aud_get_str (nullptr, "generic_title_format"));
    s_use_fallbacks = aud_get_bool (nullptr, "metadata_fallbacks");
}

void PlaylistEntry::cleanup () // static
{
    s_compiler.reset ();
}

void PlaylistEntry::format ()
{
    tuple.delete_fallbacks ();

    if (s_use_fallbacks)
        tuple.generate_fallbacks ();
    else
        tuple.generate_title ();

    s_compiler.format (tuple);
}

void PlaylistEntry::set_tuple (Tuple && new_tuple)
{
    /* Since 3.8, cuesheet entries are handled differently.  The entry filename
     * points to the .cue file, and the path to the actual audio file is stored
     * in the Tuple::AudioFile.  If Tuple::AudioFile is not set, then assume
     * that the playlist was created by an older version of Audacious, and
     * revert to the former behavior (don't refresh this entry). */
    if (tuple.is_set (Tuple::StartTime) && ! tuple.is_set (Tuple::AudioFile))
        return;

    error = String ();

    if (! new_tuple.valid ())
        new_tuple.set_filename (filename);

    length = aud::max (0, new_tuple.get_int (Tuple::Length));
    tuple = std::move (new_tuple);

    format ();
}

PlaylistEntry::PlaylistEntry (PlaylistAddItem && item) :
    filename (item.filename),
    decoder (item.decoder),
    number (-1),
    length (0),
    shuffle_num (0),
    selected (false),
    queued (false)
{
    set_tuple (std::move (item.tuple));
}

PlaylistEntry::~PlaylistEntry ()
{
    pl_signal_entry_deleted (this);
}

PlaylistData::PlaylistData (Playlist::ID * id, const char * title) :
    m_id (id),
    modified (true),
    scan_status (NotScanning),
    title (title),
    position (nullptr),
    focus (nullptr),
    selected_count (0),
    last_shuffle_num (0),
    total_length (0),
    selected_length (0),
    m_last_update (),
    m_next_update (),
    resume_time (0) {}

PlaylistData::~PlaylistData ()
{
    pl_signal_playlist_deleted (m_id);
}

void PlaylistData::set_entry_tuple (PlaylistEntry * entry, Tuple && tuple)
{
    total_length -= entry->length;
    if (entry->selected)
        selected_length -= entry->length;

    entry->set_tuple (std::move (tuple));

    total_length += entry->length;
    if (entry->selected)
        selected_length += entry->length;
}

void PlaylistData::number_entries (int at, int length)
{
    for (int i = at; i < at + length; i ++)
        entries[i]->number = i;
}

PlaylistEntry * PlaylistData::lookup_entry (int i)
{
    return (i >= 0 && i < entries.len ()) ? entries[i].get () : nullptr;
}

void PlaylistData::queue_update (Playlist::UpdateLevel level, int at, int count, int flags)
{
    if (m_next_update.level)
    {
        m_next_update.level = aud::max (m_next_update.level, level);
        m_next_update.before = aud::min (m_next_update.before, at);
        m_next_update.after = aud::min (m_next_update.after, entries.len () - at - count);
    }
    else
    {
        m_next_update.level = level;
        m_next_update.before = at;
        m_next_update.after = entries.len () - at - count;
    }

    if ((flags & QueueChanged))
        m_next_update.queue_changed = true;

    pl_signal_update_queued (m_id, level, flags);
}

void PlaylistData::cancel_updates ()
{
    m_last_update = Playlist::Update ();
    m_next_update = Playlist::Update ();
}

void PlaylistData::swap_updates ()
{
    m_last_update = m_next_update;
    m_next_update = Playlist::Update ();
}

void PlaylistData::set_position (PlaylistEntry * entry, bool update_shuffle)
{
    position = entry;
    resume_time = 0;

    /* move entry to top of shuffle list */
    if (entry && update_shuffle)
        entry->shuffle_num = ++ last_shuffle_num;
}

PlaylistEntry * PlaylistData::find_unselected_focus ()
{
    if (! focus || ! focus->selected)
        return focus;

    int n_entries = entries.len ();

    for (int search = focus->number + 1; search < n_entries; search ++)
    {
        if (! entries[search]->selected)
            return entries[search].get ();
    }

    for (int search = focus->number; search --;)
    {
        if (! entries[search]->selected)
            return entries[search].get ();
    }

    return nullptr;
}
