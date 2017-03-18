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

const PlaylistEntry * PlaylistData::lookup_entry (int i) const
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

void PlaylistData::insert_items (int at, Index<PlaylistAddItem> && items)
{
    int n_entries = entries.len ();
    int n_items = items.len ();

    if (at < 0 || at > n_entries)
        at = n_entries;

    entries.insert (at, n_items);

    int i = at;
    for (auto & item : items)
    {
        auto entry = new PlaylistEntry (std::move (item));
        entries[i ++].capture (entry);
        total_length += entry->length;
    }

    items.clear ();

    number_entries (at, n_entries + n_items - at);
    queue_update (Playlist::Structure, at, n_items);
}

void PlaylistData::remove_entries (int at, int number, bool & position_changed)
{
    int n_entries = entries.len ();
    int update_flags = 0;

    if (at < 0 || at > n_entries)
        at = n_entries;
    if (number < 0 || number > n_entries - at)
        number = n_entries - at;

    if (position && position->number >= at && position->number < at + number)
    {
        set_position (nullptr, false);
        position_changed = true;
    }

    if (focus && focus->number >= at && focus->number < at + number)
    {
        if (at + number < n_entries)
            focus = entries[at + number].get ();
        else if (at > 0)
            focus = entries[at - 1].get ();
        else
            focus = nullptr;
    }

    for (int i = 0; i < number; i ++)
    {
        PlaylistEntry * entry = entries [at + i].get ();

        if (entry->queued)
        {
            queued.remove (queued.find (entry), 1);
            update_flags |= PlaylistData::QueueChanged;
        }

        if (entry->selected)
        {
            selected_count --;
            selected_length -= entry->length;
        }

        total_length -= entry->length;
    }

    entries.remove (at, number);

    number_entries (at, n_entries - at - number);
    queue_update (Playlist::Structure, at, 0, update_flags);
}

bool PlaylistData::entry_selected (int entry_num) const
{
    auto entry = lookup_entry (entry_num);
    return entry ? entry->selected : false;
}

int PlaylistData::n_selected (int at, int number) const
{
    int n_entries = entries.len ();

    if (at < 0 || at > n_entries)
        at = n_entries;
    if (number < 0 || number > n_entries - at)
        number = n_entries - at;

    int n_selected = 0;

    if (at == 0 && number == n_entries)
        n_selected = selected_count;
    else
    {
        for (int i = 0; i < number; i ++)
        {
            if (entries[at + i]->selected)
                n_selected ++;
        }
    }

    return n_selected;
}

void PlaylistData::select_entry (int entry_num, bool selected)
{
    auto entry = lookup_entry (entry_num);
    if (! entry || entry->selected == selected)
        return;

    entry->selected = selected;

    if (selected)
    {
        selected_count ++;
        selected_length += entry->length;
    }
    else
    {
        selected_count --;
        selected_length -= entry->length;
    }

    queue_update (Playlist::Selection, entry_num, 1);
}

void PlaylistData::select_all (bool selected)
{
    int n_entries = entries.len ();
    int first = n_entries, last = 0;

    for (auto & entry : entries)
    {
        if (entry->selected != selected)
        {
            entry->selected = selected;
            first = aud::min (first, entry->number);
            last = entry->number;
        }
    }

    if (selected)
    {
        selected_count = n_entries;
        selected_length = total_length;
    }
    else
    {
        selected_count = 0;
        selected_length = 0;
    }

    if (first < n_entries)
        queue_update (Playlist::Selection, first, last + 1 - first);
}

int PlaylistData::shift_entries (int entry_num, int distance)
{
    PlaylistEntry * entry = lookup_entry (entry_num);
    if (! entry || ! entry->selected || ! distance)
        return 0;

    int n_entries = entries.len ();
    int shift = 0, center, top, bottom;

    if (distance < 0)
    {
        for (center = entry_num; center > 0 && shift > distance; )
        {
            if (! entries[-- center]->selected)
                shift --;
        }
    }
    else
    {
        for (center = entry_num + 1; center < n_entries && shift < distance; )
        {
            if (! entries[center ++]->selected)
                shift ++;
        }
    }

    top = bottom = center;

    for (int i = 0; i < top; i ++)
    {
        if (entries[i]->selected)
            top = i;
    }

    for (int i = n_entries; i > bottom; i --)
    {
        if (entries[i - 1]->selected)
            bottom = i;
    }

    Index<SmartPtr<PlaylistEntry>> temp;

    for (int i = top; i < center; i ++)
    {
        if (! entries[i]->selected)
            temp.append (std::move (entries[i]));
    }

    for (int i = top; i < bottom; i ++)
    {
        if (entries[i] && entries[i]->selected)
            temp.append (std::move (entries[i]));
    }

    for (int i = center; i < bottom; i ++)
    {
        if (entries[i] && ! entries[i]->selected)
            temp.append (std::move (entries[i]));
    }

    entries.move_from (temp, 0, top, bottom - top, false, true);

    number_entries (top, bottom - top);
    queue_update (Playlist::Structure, top, bottom - top);

    return shift;
}

void PlaylistData::remove_selected (bool & position_changed, int & next_song_hint)
{
    if (! selected_count)
        return;

    int n_entries = entries.len ();
    int update_flags = 0;

    if (position && position->selected)
    {
        set_position (nullptr, false);
        position_changed = true;
    }

    focus = find_unselected_focus ();

    int before = 0;  // number of entries before first selected
    int after = 0;   // number of entries after last selected

    while (before < n_entries && ! entries[before]->selected)
        before ++;

    int to = before;

    for (int from = before; from < n_entries; from ++)
    {
        PlaylistEntry * entry = entries[from].get ();

        if (entry->selected)
        {
            if (entry->queued)
            {
                queued.remove (queued.find (entry), 1);
                update_flags |= PlaylistData::QueueChanged;
            }

            total_length -= entry->length;
            after = 0;
        }
        else
        {
            entries[to ++] = std::move (entries[from]);
            after ++;
        }
    }

    n_entries = to;
    entries.remove (n_entries, -1);

    selected_count = 0;
    selected_length = 0;

    number_entries (before, n_entries - before);
    queue_update (Playlist::Structure, before, n_entries - after - before, update_flags);

    next_song_hint = n_entries - after;
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
