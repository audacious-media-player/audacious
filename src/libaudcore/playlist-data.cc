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

#include <stdlib.h>
#include <string.h>

#include "runtime.h"
#include "scanner.h"
#include "tuple-compiler.h"

static TupleCompiler s_tuple_formatter;
static bool s_use_tuple_fallbacks = false;

struct PlaylistEntry
{
    PlaylistEntry (PlaylistAddItem && item);
    ~PlaylistEntry ();

    void format ();
    void set_tuple (Tuple && new_tuple);

    String filename;
    PluginHandle * decoder;
    Tuple tuple;
    String error;
    int number;
    int length;
    int shuffle_num;
    bool selected, queued;
};

void PlaylistEntry::format ()
{
    tuple.delete_fallbacks ();

    if (s_use_tuple_fallbacks)
        tuple.generate_fallbacks ();
    else
        tuple.generate_title ();

    s_tuple_formatter.format (tuple);
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

void PlaylistData::update_formatter () // static
{
    s_tuple_formatter.compile (aud_get_str (nullptr, "generic_title_format"));
    s_use_tuple_fallbacks = aud_get_bool (nullptr, "metadata_fallbacks");
}

void PlaylistData::cleanup_formatter () // static
    { s_tuple_formatter.reset (); }

void PlaylistData::delete_entry (PlaylistEntry * entry) // static
    { delete entry; }

PlaylistData::PlaylistData (Playlist::ID * id, const char * title) :
    modified (true),
    scan_status (NotScanning),
    title (title),
    resume_time (0),
    m_id (id),
    m_position (nullptr),
    m_focus (nullptr),
    m_selected_count (0),
    m_last_shuffle_num (0),
    m_total_length (0),
    m_selected_length (0),
    m_last_update (),
    m_next_update (),
    m_position_changed (false) {}

PlaylistData::~PlaylistData ()
{
    pl_signal_playlist_deleted (m_id);
}

void PlaylistData::number_entries (int at, int length)
{
    for (int i = at; i < at + length; i ++)
        m_entries[i]->number = i;
}

PlaylistEntry * PlaylistData::entry_at (int i)
{
    return (i >= 0 && i < m_entries.len ()) ? m_entries[i].get () : nullptr;
}

const PlaylistEntry * PlaylistData::entry_at (int i) const
{
    return (i >= 0 && i < m_entries.len ()) ? m_entries[i].get () : nullptr;
}

String PlaylistData::entry_filename (int i) const
{
    auto entry = entry_at (i);
    return entry ? entry->filename : String ();
}

PluginHandle * PlaylistData::entry_decoder (int i, String * error) const
{
    auto entry = entry_at (i);
    if (error) * error = entry ? entry->error : String ();
    return entry ? entry->decoder : nullptr;
}

Tuple PlaylistData::entry_tuple (int i, String * error) const
{
    auto entry = entry_at (i);
    if (error) * error = entry ? entry->error : String ();
    return entry ? entry->tuple.ref () : Tuple ();
}

void PlaylistData::set_entry_tuple (PlaylistEntry * entry, Tuple && tuple)
{
    m_total_length -= entry->length;
    if (entry->selected)
        m_selected_length -= entry->length;

    entry->set_tuple (std::move (tuple));

    m_total_length += entry->length;
    if (entry->selected)
        m_selected_length += entry->length;
}

void PlaylistData::queue_update (Playlist::UpdateLevel level, int at, int count, int flags)
{
    if (m_next_update.level)
    {
        m_next_update.level = aud::max (m_next_update.level, level);
        m_next_update.before = aud::min (m_next_update.before, at);
        m_next_update.after = aud::min (m_next_update.after, m_entries.len () - at - count);
    }
    else
    {
        m_next_update.level = level;
        m_next_update.before = at;
        m_next_update.after = m_entries.len () - at - count;
    }

    if ((flags & QueueChanged))
        m_next_update.queue_changed = true;

    pl_signal_update_queued (m_id, level, flags);
}

void PlaylistData::queue_position_change ()
{
    m_position_changed = true;
    pl_signal_position_changed (m_id);
}

void PlaylistData::cancel_updates ()
{
    m_last_update = Playlist::Update ();
    m_next_update = Playlist::Update ();
    m_position_changed = false;
}

void PlaylistData::swap_updates (bool & position_changed)
{
    m_last_update = m_next_update;
    m_next_update = Playlist::Update ();
    position_changed = m_position_changed;
    m_position_changed = false;
}

void PlaylistData::insert_items (int at, Index<PlaylistAddItem> && items)
{
    int n_entries = m_entries.len ();
    int n_items = items.len ();

    if (at < 0 || at > n_entries)
        at = n_entries;

    m_entries.insert (at, n_items);

    int i = at;
    for (auto & item : items)
    {
        auto entry = new PlaylistEntry (std::move (item));
        m_entries[i ++].capture (entry);
        m_total_length += entry->length;
    }

    items.clear ();

    number_entries (at, n_entries + n_items - at);
    queue_update (Playlist::Structure, at, n_items);
}

void PlaylistData::remove_entries (int at, int number)
{
    int n_entries = m_entries.len ();
    bool position_changed = false;
    int update_flags = 0;

    if (at < 0 || at > n_entries)
        at = n_entries;
    if (number < 0 || number > n_entries - at)
        number = n_entries - at;

    if (m_position && m_position->number >= at && m_position->number < at + number)
    {
        set_position (nullptr, false);
        position_changed = true;
    }

    if (m_focus && m_focus->number >= at && m_focus->number < at + number)
    {
        if (at + number < n_entries)
            m_focus = m_entries[at + number].get ();
        else if (at > 0)
            m_focus = m_entries[at - 1].get ();
        else
            m_focus = nullptr;
    }

    for (int i = 0; i < number; i ++)
    {
        PlaylistEntry * entry = m_entries [at + i].get ();

        if (entry->queued)
        {
            m_queued.remove (m_queued.find (entry), 1);
            update_flags |= QueueChanged;
        }

        if (entry->selected)
        {
            m_selected_count --;
            m_selected_length -= entry->length;
        }

        m_total_length -= entry->length;
    }

    m_entries.remove (at, number);

    number_entries (at, n_entries - at - number);
    queue_update (Playlist::Structure, at, 0, update_flags);

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_with_hint (aud_get_bool (nullptr, "repeat"), at);

        queue_position_change ();
    }
}

int PlaylistData::position () const
{
    return m_position ? m_position->number : -1;
}

int PlaylistData::focus () const
{
    return m_focus ? m_focus->number : -1;
}

bool PlaylistData::entry_selected (int entry_num) const
{
    auto entry = entry_at (entry_num);
    return entry ? entry->selected : false;
}

int PlaylistData::n_selected (int at, int number) const
{
    int n_entries = m_entries.len ();

    if (at < 0 || at > n_entries)
        at = n_entries;
    if (number < 0 || number > n_entries - at)
        number = n_entries - at;

    int n_selected = 0;

    if (at == 0 && number == n_entries)
        n_selected = m_selected_count;
    else
    {
        for (int i = 0; i < number; i ++)
        {
            if (m_entries[at + i]->selected)
                n_selected ++;
        }
    }

    return n_selected;
}

void PlaylistData::set_focus (int entry_num)
{
    auto new_focus = entry_at (entry_num);
    if (new_focus == m_focus)
        return;

    int first = m_entries.len ();
    int last = -1;

    if (m_focus)
    {
        first = aud::min (first, m_focus->number);
        last = aud::max (last, m_focus->number);
    }

    m_focus = new_focus;

    if (m_focus)
    {
        first = aud::min (first, m_focus->number);
        last = aud::max (last, m_focus->number);
    }

    if (first <= last)
        queue_update (Playlist::Selection, first, last + 1 - first);
}

void PlaylistData::select_entry (int entry_num, bool selected)
{
    auto entry = entry_at (entry_num);
    if (! entry || entry->selected == selected)
        return;

    entry->selected = selected;

    if (selected)
    {
        m_selected_count ++;
        m_selected_length += entry->length;
    }
    else
    {
        m_selected_count --;
        m_selected_length -= entry->length;
    }

    queue_update (Playlist::Selection, entry_num, 1);
}

void PlaylistData::select_all (bool selected)
{
    int n_entries = m_entries.len ();
    int first = n_entries, last = 0;

    for (auto & entry : m_entries)
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
        m_selected_count = n_entries;
        m_selected_length = m_total_length;
    }
    else
    {
        m_selected_count = 0;
        m_selected_length = 0;
    }

    if (first < n_entries)
        queue_update (Playlist::Selection, first, last + 1 - first);
}

int PlaylistData::shift_entries (int entry_num, int distance)
{
    PlaylistEntry * entry = entry_at (entry_num);
    if (! entry || ! entry->selected || ! distance)
        return 0;

    int n_entries = m_entries.len ();
    int shift = 0, center, top, bottom;

    if (distance < 0)
    {
        for (center = entry_num; center > 0 && shift > distance; )
        {
            if (! m_entries[-- center]->selected)
                shift --;
        }
    }
    else
    {
        for (center = entry_num + 1; center < n_entries && shift < distance; )
        {
            if (! m_entries[center ++]->selected)
                shift ++;
        }
    }

    top = bottom = center;

    for (int i = 0; i < top; i ++)
    {
        if (m_entries[i]->selected)
            top = i;
    }

    for (int i = n_entries; i > bottom; i --)
    {
        if (m_entries[i - 1]->selected)
            bottom = i;
    }

    Index<EntryPtr> temp;

    for (int i = top; i < center; i ++)
    {
        if (! m_entries[i]->selected)
            temp.append (std::move (m_entries[i]));
    }

    for (int i = top; i < bottom; i ++)
    {
        if (m_entries[i] && m_entries[i]->selected)
            temp.append (std::move (m_entries[i]));
    }

    for (int i = center; i < bottom; i ++)
    {
        if (m_entries[i] && ! m_entries[i]->selected)
            temp.append (std::move (m_entries[i]));
    }

    m_entries.move_from (temp, 0, top, bottom - top, false, true);

    number_entries (top, bottom - top);
    queue_update (Playlist::Structure, top, bottom - top);

    return shift;
}

void PlaylistData::remove_selected ()
{
    if (! m_selected_count)
        return;

    int n_entries = m_entries.len ();
    bool position_changed = false;
    int update_flags = 0;

    if (m_position && m_position->selected)
    {
        set_position (nullptr, false);
        position_changed = true;
    }

    m_focus = find_unselected_focus ();

    int before = 0;  // number of entries before first selected
    int after = 0;   // number of entries after last selected

    while (before < n_entries && ! m_entries[before]->selected)
        before ++;

    int to = before;

    for (int from = before; from < n_entries; from ++)
    {
        PlaylistEntry * entry = m_entries[from].get ();

        if (entry->selected)
        {
            if (entry->queued)
            {
                m_queued.remove (m_queued.find (entry), 1);
                update_flags |= QueueChanged;
            }

            m_total_length -= entry->length;
            after = 0;
        }
        else
        {
            m_entries[to ++] = std::move (m_entries[from]);
            after ++;
        }
    }

    n_entries = to;
    m_entries.remove (n_entries, -1);

    m_selected_count = 0;
    m_selected_length = 0;

    number_entries (before, n_entries - before);
    queue_update (Playlist::Structure, before, n_entries - after - before, update_flags);

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_with_hint (aud_get_bool (nullptr, "repeat"), n_entries - after);

        queue_position_change ();
    }
}

void PlaylistData::sort_entries (Index<EntryPtr> & entries, const CompareData & data) // static
{
    entries.sort ([data] (const EntryPtr & a, const EntryPtr & b) {
        if (data.filename_compare)
            return data.filename_compare (a->filename, b->filename);
        else
            return data.tuple_compare (a->tuple, b->tuple);
    });
}

void PlaylistData::sort (const CompareData & data)
{
    sort_entries (m_entries, data);

    number_entries (0, m_entries.len ());
    queue_update (Playlist::Structure, 0, m_entries.len ());
}

void PlaylistData::sort_selected (const CompareData & data)
{
    int n_entries = m_entries.len ();

    Index<EntryPtr> selected;

    for (auto & entry : m_entries)
    {
        if (entry->selected)
            selected.append (std::move (entry));
    }

    sort_entries (selected, data);

    int i = 0;
    for (auto & entry : m_entries)
    {
        if (! entry)
            entry = std::move (selected[i ++]);
    }

    number_entries (0, n_entries);
    queue_update (Playlist::Structure, 0, n_entries);
}

void PlaylistData::reverse_order ()
{
    int n_entries = m_entries.len ();

    for (int i = 0; i < n_entries / 2; i ++)
        std::swap (m_entries[i], m_entries[n_entries - 1 - i]);

    number_entries (0, n_entries);
    queue_update (Playlist::Structure, 0, n_entries);
}

void PlaylistData::reverse_selected ()
{
    int n_entries = m_entries.len ();

    int top = 0;
    int bottom = n_entries - 1;

    while (1)
    {
        while (top < bottom && ! m_entries[top]->selected)
            top ++;
        while (top < bottom && ! m_entries[bottom]->selected)
            bottom --;

        if (top >= bottom)
            break;

        std::swap (m_entries[top ++], m_entries[bottom --]);
    }

    number_entries (0, n_entries);
    queue_update (Playlist::Structure, 0, n_entries);
}

void PlaylistData::randomize_order ()
{
    int n_entries = m_entries.len ();

    for (int i = 0; i < n_entries; i ++)
        std::swap (m_entries[i], m_entries[rand () % n_entries]);

    number_entries (0, n_entries);
    queue_update (Playlist::Structure, 0, n_entries);
}

void PlaylistData::randomize_selected ()
{
    int n_entries = m_entries.len ();

    Index<PlaylistEntry *> selected;

    for (auto & entry : m_entries)
    {
        if (entry->selected)
            selected.append (entry.get ());
    }

    int n_selected = selected.len ();

    for (int i = 0; i < n_selected; i ++)
    {
        int a = selected[i]->number;
        int b = selected[rand () % n_selected]->number;
        std::swap (m_entries[a], m_entries[b]);
    }

    number_entries (0, n_entries);
    queue_update (Playlist::Structure, 0, n_entries);
}

int PlaylistData::queue_get_entry (int at) const
{
    return (at >= 0 && at < m_queued.len ()) ? m_queued[at]->number : -1;
}

int PlaylistData::queue_find_entry (int entry_num) const
{
    auto entry = entry_at (entry_num);
    return entry->queued ? m_queued.find ((PlaylistEntry *) entry) : -1;
}

void PlaylistData::queue_insert (int at, int entry_num)
{
    auto entry = entry_at (entry_num);
    if (entry->queued)
        return;

    if (at < 0 || at > m_queued.len ())
        m_queued.append (entry);
    else
    {
        m_queued.insert (at, 1);
        m_queued[at] = entry;
    }

    entry->queued = true;

    queue_update (Playlist::Selection, entry_num, 1, QueueChanged);
}

void PlaylistData::queue_insert_selected (int at)
{
    if (at < 0 || at > m_queued.len ())
        at = m_queued.len ();

    Index<PlaylistEntry *> add;
    int first = m_entries.len ();
    int last = 0;

    for (auto & entry : m_entries)
    {
        if (! entry->selected || entry->queued)
            continue;

        add.append (entry.get ());
        entry->queued = true;
        first = aud::min (first, entry->number);
        last = entry->number;
    }

    m_queued.move_from (add, 0, at, -1, true, true);

    if (first < m_entries.len ())
        queue_update (Playlist::Selection, first, last + 1 - first, QueueChanged);
}

void PlaylistData::queue_remove (int at, int number)
{
    int queue_len = m_queued.len ();

    if (at < 0 || at > queue_len)
        at = queue_len;
    if (number < 0 || number > queue_len - at)
        number = queue_len - at;

    int n_entries = m_entries.len ();
    int first = n_entries, last = 0;

    for (int i = at; i < at + number; i ++)
    {
        PlaylistEntry * entry = m_queued[i];
        entry->queued = false;
        first = aud::min (first, entry->number);
        last = entry->number;
    }

    m_queued.remove (at, number);

    if (first < n_entries)
        queue_update (Playlist::Selection, first, last + 1 - first, QueueChanged);
}

void PlaylistData::queue_remove_selected ()
{
    int n_entries = m_entries.len ();
    int first = n_entries, last = 0;

    for (int i = 0; i < m_queued.len ();)
    {
        PlaylistEntry * entry = m_queued[i];

        if (entry->selected)
        {
            m_queued.remove (i, 1);
            entry->queued = false;
            first = aud::min (first, entry->number);
            last = entry->number;
        }
        else
            i ++;
    }

    if (first < n_entries)
        queue_update (Playlist::Selection, first, last + 1 - first, QueueChanged);
}

void PlaylistData::set_position (PlaylistEntry * entry, bool update_shuffle)
{
    m_position = entry;
    resume_time = 0;

    /* move entry to top of shuffle list */
    if (entry && update_shuffle)
        entry->shuffle_num = ++ m_last_shuffle_num;
}

void PlaylistData::set_position (int entry_num)
{
    set_position (entry_at (entry_num), true);
    queue_position_change ();
}

bool PlaylistData::shuffle_prev ()
{
    PlaylistEntry * found = nullptr;

    for (auto & entry : m_entries)
    {
        if (entry->shuffle_num &&
            (! m_position || entry->shuffle_num < m_position->shuffle_num) &&
            (! found || entry->shuffle_num > found->shuffle_num))
        {
            found = entry.get ();
        }
    }

    if (! found)
        return false;

    set_position (found, false);
    return true;
}

bool PlaylistData::shuffle_next ()
{
    bool by_album = aud_get_bool (nullptr, "album_shuffle");

    // helper #1: determine whether two entries are in the same album
    auto same_album = [] (const Tuple & a, const Tuple & b)
    {
        String album = a.get_str (Tuple::Album);
        return (album && album == b.get_str (Tuple::Album));
    };

    // helper #2: determine whether an entry is among the shuffle choices
    auto is_choice = [&] (PlaylistEntry * prev, PlaylistEntry * entry)
    {
        return (! entry->shuffle_num) && (! by_album || ! prev ||
         prev->shuffle_num || ! same_album (prev->tuple, entry->tuple));
    };

    if (m_position)
    {
        // step #1: check to see if the shuffle order is already established
        PlaylistEntry * next = nullptr;

        for (auto & entry : m_entries)
        {
            if (entry->shuffle_num > m_position->shuffle_num &&
             (! next || entry->shuffle_num < next->shuffle_num))
                next = entry.get ();
        }

        if (next)
        {
            set_position (next, false);
            return true;
        }

        // step #2: check to see if we should advance to the next entry
        if (by_album && m_position->number + 1 < m_entries.len ())
        {
            next = m_entries[m_position->number + 1].get ();

            if (! next->shuffle_num && same_album (m_position->tuple, next->tuple))
            {
                set_position (next, true);
                return true;
            }
        }
    }

    // step #3: count the number of possible shuffle choices
    int choices = 0;
    PlaylistEntry * prev = nullptr;

    for (auto & entry : m_entries)
    {
        if (is_choice (prev, entry.get ()))
            choices ++;

        prev = entry.get ();
    }

    if (! choices)
        return false;

    // step #4: pick one of those choices by random and find it again
    choices = rand () % choices;
    prev = nullptr;

    for (auto & entry : m_entries)
    {
        if (is_choice (prev, entry.get ()))
        {
            if (! choices)
            {
                set_position (entry.get (), true);
                return true;
            }

            choices --;
        }

        prev = entry.get ();
    }

    return false;  // never reached
}

void PlaylistData::shuffle_reset ()
{
    m_last_shuffle_num = 0;

    for (auto & entry : m_entries)
        entry->shuffle_num = 0;
}

bool PlaylistData::prev_song ()
{
    if (aud_get_bool (nullptr, "shuffle"))
    {
        if (! shuffle_prev ())
            return false;
    }
    else
    {
        int pos = position ();
        if (pos < 1)
            return false;

        set_position (m_entries[pos - 1].get (), true);
    }

    queue_position_change ();
    return true;
}

bool PlaylistData::next_song_with_hint (bool repeat, int hint)
{
    int n_entries = m_entries.len ();
    if (! n_entries)
        return false;

    PlaylistEntry * entry;
    if ((entry = queue_pop ()))
    {
        set_position (entry, true);
        return true;
    }

    if (aud_get_bool (nullptr, "shuffle"))
    {
        if (shuffle_next ())
            return true;

        if (! repeat)
            return false;

        shuffle_reset ();

        return shuffle_next ();
    }

    if (hint < 0 || hint >= n_entries)
    {
        if (! repeat)
            return false;

        hint = 0;
    }

    set_position (m_entries[hint].get (), true);
    return true;
}

bool PlaylistData::next_song (bool repeat)
{
    if (! next_song_with_hint (repeat, position () + 1)) // -1 becomes 0
        return false;

    queue_position_change ();
    return true;
}

int PlaylistData::next_unscanned_entry (int entry_num) const
{
    if (entry_num < 0)
        return -1;

    for (; entry_num < m_entries.len (); entry_num ++)
    {
        auto & entry = *m_entries[entry_num];

        if (entry.tuple.state () == Tuple::Initial &&
            strncmp (entry.filename, "stdin://", 8)) // blacklist stdin
        {
            return entry_num;
        }
    }

    return -1;
}

ScanRequest * PlaylistData::create_scan_request (PlaylistEntry * entry,
 ScanRequest::Callback callback, int extra_flags)
{
    int flags = extra_flags;
    if (! entry->tuple.valid ())
        flags |= SCAN_TUPLE;

    /* scanner uses Tuple::AudioFile from existing tuple, if valid */
    return new ScanRequest (entry->filename, flags, callback, entry->decoder,
     (flags & SCAN_TUPLE) ? Tuple () : entry->tuple.ref ());
}

void PlaylistData::update_entry_from_scan (PlaylistEntry * entry, ScanRequest * request, int update_flags)
{
    if (! entry->decoder)
        entry->decoder = request->decoder;

    if (! entry->tuple.valid () && request->tuple.valid ())
    {
        set_entry_tuple (entry, std::move (request->tuple));
        queue_update (Playlist::Metadata, entry->number, 1, update_flags);
    }

    if (! entry->decoder || ! entry->tuple.valid ())
        entry->error = request->error;

    if (entry->tuple.state () == Tuple::Initial)
    {
        entry->tuple.set_state (Tuple::Failed);
        queue_update (Playlist::Metadata, entry->number, 1, update_flags);
    }
}

void PlaylistData::update_playback_entry (Tuple && tuple)
{
    /* don't update cuesheet entries with stream metadata */
    if (m_position && ! m_position->tuple.is_set (Tuple::StartTime))
    {
        set_entry_tuple (m_position, std::move (tuple));
        queue_update (Playlist::Metadata, m_position->number, 1);
    }
}

bool PlaylistData::entry_needs_rescan (PlaylistEntry * entry, bool need_decoder, bool need_tuple)
{
    if (! strncmp (entry->filename, "stdin://", 8)) // blacklist stdin
        return false;

    // check whether requested data (decoder and/or tuple) has been read
    return (need_decoder && ! entry->decoder) || (need_tuple && ! entry->tuple.valid ());
}

void PlaylistData::reformat_titles ()
{
    for (auto & entry : m_entries)
        entry->format ();

    queue_update (Playlist::Metadata, 0, m_entries.len ());
}

void PlaylistData::reset_tuples (bool selected_only)
{
    for (auto & entry : m_entries)
    {
        if (! selected_only || entry->selected)
            set_entry_tuple (entry.get (), Tuple ());
    }

    queue_update (Playlist::Metadata, 0, m_entries.len ());
    pl_signal_rescan_needed (m_id);
}

void PlaylistData::reset_tuple_of_file (const char * filename)
{
    bool found = false;

    for (auto & entry : m_entries)
    {
        if (! strcmp (entry->filename, filename))
        {
            set_entry_tuple (entry.get (), Tuple ());
            queue_update (Playlist::Metadata, entry->number, 1);
            found = true;
        }
    }

    if (found)
        pl_signal_rescan_needed (m_id);
}

PlaylistEntry * PlaylistData::find_unselected_focus ()
{
    if (! m_focus || ! m_focus->selected)
        return m_focus;

    int n_entries = m_entries.len ();

    for (int search = m_focus->number + 1; search < n_entries; search ++)
    {
        if (! m_entries[search]->selected)
            return m_entries[search].get ();
    }

    for (int search = m_focus->number; search --;)
    {
        if (! m_entries[search]->selected)
            return m_entries[search].get ();
    }

    return nullptr;
}

PlaylistEntry * PlaylistData::queue_pop ()
{
    if (! m_queued.len ())
        return nullptr;

    auto entry = m_queued[0];
    m_queued.remove (0, 1);
    entry->queued = false;

    queue_update (Playlist::Selection, entry->number, 1, QueueChanged);

    return entry;
}
