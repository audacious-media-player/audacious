/*
 * playlist.cc
 * Copyright 2009-2017 John Lindgren
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

#include "playlist-internal.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "drct.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "internal.h"
#include "list.h"
#include "mainloop.h"
#include "multihash.h"
#include "objects.h"
#include "parse.h"
#include "playlist-data.h"
#include "plugins.h"
#include "runtime.h"
#include "scanner.h"
#include "tuple.h"
#include "tuple-compiler.h"

enum {
    ResumeStop,
    ResumePlay,
    ResumePause
};

/* playback hooks */
enum {
    SetPlaylist   = (1 << 0),
    SetPosition   = (1 << 1),
    PlaybackBegin = (1 << 2),
    PlaybackStop  = (1 << 3)
};

/* update flags */
enum {
    QueueChanged  = (1 << 0),
    DelayedUpdate = (1 << 1)
};

#define STATE_FILE "playlist-state"

#define ENTER pthread_mutex_lock (& mutex)
#define LEAVE pthread_mutex_unlock (& mutex)

#define RETURN(...) do { \
    pthread_mutex_unlock (& mutex); \
    return __VA_ARGS__; \
} while (0)

#define ENTER_GET_PLAYLIST(...) ENTER; \
    PlaylistData * playlist = lookup_playlist (m_id); \
    if (! playlist) \
        RETURN (__VA_ARGS__);

#define ENTER_GET_ENTRY(...) ENTER_GET_PLAYLIST (__VA_ARGS__); \
    PlaylistEntry * entry = playlist->lookup_entry (entry_num); \
    if (! entry) \
        RETURN (__VA_ARGS__);

static const char * const default_title = N_("New Playlist");
static const char * const temp_title = N_("Now Playing");

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/*
 * Each playlist is associated with its own ID struct, which contains a unique
 * integer "stamp" (this is the source of the internal filenames 1000.audpl,
 * 1001.audpl, etc.)  The ID struct also serves as a "weak" pointer to the
 * actual data, and persists even after the playlist itself is destroyed.
 * The IDs are stored in a hash table, allowing lookup by stamp.
 *
 * In brief: Playlist (public handle)
 *             points to ->
 *           Playlist::ID (unique ID / weak pointer)
 *             points to ->
 *           PlaylistData (actual playlist data)
 */
struct Playlist::ID
{
    int stamp;            // integer stamp, determines filename
    int index;            // display order
    PlaylistData * data;  // pointer to actual playlist data
};

static SimpleHash<IntHashKey, Playlist::ID> id_table;
static int next_stamp = 1000;

static Index<SmartPtr<PlaylistData>> playlists;
static Playlist::ID * active_id = nullptr;
static Playlist::ID * playing_id = nullptr;
static int resume_playlist = -1;
static bool resume_paused = false;

static QueuedFunc queued_update;
static Playlist::UpdateLevel update_level;
static bool update_delayed;

struct ScanItem : public ListNode
{
    ScanItem (PlaylistData * playlist, PlaylistEntry * entry, ScanRequest * request, bool for_playback) :
        playlist (playlist),
        entry (entry),
        request (request),
        for_playback (for_playback),
        handled_by_playback (false) {}

    PlaylistData * playlist;
    PlaylistEntry * entry;
    ScanRequest * request;
    bool for_playback;
    bool handled_by_playback;
};

static bool scan_enabled_nominal, scan_enabled;
static int scan_playlist, scan_row;
static List<ScanItem> scan_list;

static void scan_finish (ScanRequest * request);
static void scan_cancel (PlaylistEntry * entry);
static void scan_queue_playlist (PlaylistData * playlist);
static void scan_restart ();

static bool next_song_locked (PlaylistData * playlist, bool repeat, int hint);

static void playlist_reformat_titles (void * = nullptr, void * = nullptr);
static void playlist_trigger_scan (void * = nullptr, void * = nullptr);

/* creates a new playlist with the requested stamp (if not already in use) */
static Playlist::ID * create_playlist (int stamp)
{
    Playlist::ID * id;

    if (stamp >= 0 && ! id_table.lookup (stamp))
        id = id_table.add (stamp, {stamp, -1, nullptr});
    else
    {
        while (id_table.lookup (next_stamp))
            next_stamp ++;

        id = id_table.add (next_stamp, {next_stamp, -1, nullptr});
    }

    id->data = new PlaylistData (id, _(default_title));

    return id;
}

static void number_playlists (int at, int length)
{
    for (int i = at; i < at + length; i ++)
        playlists[i]->id ()->index = i;
}

static PlaylistData * lookup_playlist (Playlist::ID * id)
{
    return id ? id->data : nullptr;
}

static void update (void *)
{
    ENTER;

    for (auto & p : playlists)
    {
        p->last_update = p->next_update;
        p->next_update = Playlist::Update ();
    }

    auto level = update_level;
    update_level = Playlist::NoUpdate;
    update_delayed = false;

    LEAVE;

    hook_call ("playlist update", aud::to_ptr (level));
}

static void queue_update (Playlist::UpdateLevel level, PlaylistData * p, int at,
 int count, int flags = 0)
{
    if (p)
    {
        if (level == Playlist::Structure)
            scan_queue_playlist (p);

        if (level >= Playlist::Metadata)
        {
            if (p->id () == playing_id && p->position)
                playback_set_info (p->position->number, p->position->tuple.ref ());

            p->modified = true;
        }

        if (p->next_update.level)
        {
            p->next_update.level = aud::max (p->next_update.level, level);
            p->next_update.before = aud::min (p->next_update.before, at);
            p->next_update.after = aud::min (p->next_update.after, p->entries.len () - at - count);
        }
        else
        {
            p->next_update.level = level;
            p->next_update.before = at;
            p->next_update.after = p->entries.len () - at - count;
        }

        if ((flags & QueueChanged))
            p->next_update.queue_changed = true;
    }

    if (level == Playlist::Structure)
        scan_restart ();

    // only allow delayed update if a scan is still in progress
    if ((flags & DelayedUpdate) && scan_enabled && p && (p->scanning || p->scan_ending))
    {
        if (! update_level)
        {
            queued_update.queue (250, update, nullptr);
            update_delayed = true;
        }
    }
    else
    {
        if (! update_level || update_delayed)
        {
            queued_update.queue (update, nullptr);
            update_delayed = false;
        }
    }

    update_level = aud::max (update_level, level);
}

EXPORT bool Playlist::update_pending () const
{
    ENTER_GET_PLAYLIST (false);
    bool pending = playlist->next_update.level ? true : false;
    RETURN (pending);
}

EXPORT bool Playlist::update_pending_any ()
{
    ENTER;
    bool pending = update_level ? true : false;
    RETURN (pending);
}

EXPORT Playlist::Update Playlist::update_detail () const
{
    ENTER_GET_PLAYLIST (Update ());
    Update update = playlist->last_update;
    RETURN (update);
}

EXPORT bool Playlist::scan_in_progress () const
{
    ENTER_GET_PLAYLIST (false);
    bool scanning = playlist->scanning || playlist->scan_ending;
    RETURN (scanning);
}

EXPORT bool Playlist::scan_in_progress_any ()
{
    ENTER;

    bool scanning = false;
    for (auto & p : playlists)
    {
        if (p->scanning || p->scan_ending)
            scanning = true;
    }

    RETURN (scanning);
}

static ScanItem * scan_list_find_playlist (PlaylistData * playlist)
{
    for (ScanItem * item = scan_list.head (); item; item = scan_list.next (item))
    {
        if (item->playlist == playlist)
            return item;
    }

    return nullptr;
}

static ScanItem * scan_list_find_entry (PlaylistEntry * entry)
{
    for (ScanItem * item = scan_list.head (); item; item = scan_list.next (item))
    {
        if (item->entry == entry)
            return item;
    }

    return nullptr;
}

static ScanItem * scan_list_find_request (ScanRequest * request)
{
    for (ScanItem * item = scan_list.head (); item; item = scan_list.next (item))
    {
        if (item->request == request)
            return item;
    }

    return nullptr;
}

static void scan_queue_entry (PlaylistData * playlist, PlaylistEntry * entry, bool for_playback = false)
{
    int flags = 0;
    if (! entry->tuple.valid ())
        flags |= SCAN_TUPLE;
    if (for_playback)
        flags |= (SCAN_IMAGE | SCAN_FILE);

    /* scanner uses Tuple::AudioFile from existing tuple, if valid */
    auto request = new ScanRequest (entry->filename, flags, scan_finish,
     entry->decoder, (flags & SCAN_TUPLE) ? Tuple () : entry->tuple.ref ());

    scan_list.append (new ScanItem (playlist, entry, request, for_playback));

    /* playback entry will be scanned by the playback thread */
    if (! for_playback)
        scanner_request (request);
}

static void scan_reset_playback ()
{
    for (ScanItem * item = scan_list.head (); item; item = scan_list.next (item))
    {
        if (item->for_playback)
        {
            item->for_playback = false;

            /* if playback was canceled before the entry was scanned, requeue it */
            if (! item->handled_by_playback)
                scanner_request (item->request);
        }
    }
}

static void scan_check_complete (PlaylistData * playlist)
{
    if (! playlist->scan_ending || scan_list_find_playlist (playlist))
        return;

    playlist->scan_ending = false;

    if (update_delayed)
    {
        queued_update.queue (update, nullptr);
        update_delayed = false;
    }

    event_queue_cancel ("playlist scan complete");
    event_queue ("playlist scan complete", nullptr);
}

static bool scan_queue_next_entry ()
{
    if (! scan_enabled)
        return false;

    while (scan_playlist < playlists.len ())
    {
        PlaylistData * playlist = playlists[scan_playlist].get ();

        if (playlist->scanning)
        {
            while (scan_row < playlist->entries.len ())
            {
                PlaylistEntry * entry = playlist->entries[scan_row ++].get ();

                // blacklist stdin
                if (entry->tuple.state () == Tuple::Initial &&
                 ! scan_list_find_entry (entry) &&
                 strncmp (entry->filename, "stdin://", 8))
                {
                    scan_queue_entry (playlist, entry);
                    return true;
                }
            }

            playlist->scanning = false;
            playlist->scan_ending = true;
            scan_check_complete (playlist);
        }

        scan_playlist ++;
        scan_row = 0;
    }

    return false;
}

static void scan_schedule ()
{
    int scheduled = 0;

    for (ScanItem * item = scan_list.head (); item; item = scan_list.next (item))
    {
        if (++ scheduled >= SCAN_THREADS)
            return;
    }

    while (scan_queue_next_entry ())
    {
        if (++ scheduled >= SCAN_THREADS)
            return;
    }
}

static void scan_finish (ScanRequest * request)
{
    ENTER;

    ScanItem * item = scan_list_find_request (request);
    if (! item)
        RETURN ();

    PlaylistData * playlist = item->playlist;
    PlaylistEntry * entry = item->entry;

    scan_list.remove (item);

    if (! entry->decoder)
        entry->decoder = request->decoder;

    if (! entry->tuple.valid () && request->tuple.valid ())
    {
        playlist->set_entry_tuple (entry, std::move (request->tuple));
        queue_update (Playlist::Metadata, playlist, entry->number, 1, DelayedUpdate);
    }

    if (! entry->decoder || ! entry->tuple.valid ())
        entry->error = request->error;

    if (entry->tuple.state () == Tuple::Initial)
    {
        entry->tuple.set_state (Tuple::Failed);
        queue_update (Playlist::Metadata, playlist, entry->number, 1, DelayedUpdate);
    }

    delete item;

    scan_check_complete (playlist);
    scan_schedule ();

    pthread_cond_broadcast (& cond);

    LEAVE;
}

static void scan_cancel (PlaylistEntry * entry)
{
    ScanItem * item = scan_list_find_entry (entry);
    if (! item)
        return;

    scan_list.remove (item);
    delete (item);
}

static void scan_queue_playlist (PlaylistData * playlist)
{
    playlist->scanning = true;
    playlist->scan_ending = false;
}

static void scan_restart ()
{
    scan_playlist = 0;
    scan_row = 0;
    scan_schedule ();
}

/* mutex may be unlocked during the call */
static PlaylistEntry * get_entry (Playlist::ID * playlist_id, int entry_num,
 bool need_decoder, bool need_tuple)
{
    bool scan_started = false;

    while (1)
    {
        PlaylistData * playlist = lookup_playlist (playlist_id);
        PlaylistEntry * entry = playlist ? playlist->lookup_entry (entry_num) : nullptr;

        // check whether entry was deleted; also blacklist stdin
        if (! entry || ! strncmp (entry->filename, "stdin://", 8))
            return entry;

        // check whether requested data (decoder and/or tuple) has been read
        if ((! need_decoder || entry->decoder) && (! need_tuple || entry->tuple.valid ()))
            return entry;

        // start scan if not already running ...
        if (! scan_list_find_entry (entry))
        {
            // ... but only once
            if (scan_started)
                return entry;

            scan_queue_entry (playlist, entry);
        }

        // wait for scan to finish
        scan_started = true;
        pthread_cond_wait (& cond, & mutex);
    }
}

static void start_playback_locked (int seek_time, bool pause)
{
    art_clear_current ();
    scan_reset_playback ();

    playback_play (seek_time, pause);

    // playback always begins with a rescan of the current entry in order to
    // open the file, ensure a valid tuple, and read album art
    scan_cancel (playing_id->data->position);
    scan_queue_entry (playing_id->data, playing_id->data->position, true);
}

static void stop_playback_locked ()
{
    art_clear_current ();
    scan_reset_playback ();

    playback_stop ();
}

void pl_signal_entry_deleted (PlaylistEntry * entry)
{
    scan_cancel (entry);
}

void pl_signal_playlist_deleted (Playlist::ID * id)
{
    /* break weak pointer link */
    id->data = nullptr;
    id->index = -1;
}

void playlist_init ()
{
    srand (time (nullptr));

    ENTER;

    update_level = Playlist::NoUpdate;
    update_delayed = false;
    scan_enabled = false;
    scan_playlist = scan_row = 0;

    LEAVE;

    /* initialize title formatter */
    playlist_reformat_titles ();

    hook_associate ("set metadata_on_play", playlist_trigger_scan, nullptr);
    hook_associate ("set generic_title_format", playlist_reformat_titles, nullptr);
    hook_associate ("set leading_zero", playlist_reformat_titles, nullptr);
    hook_associate ("set show_hours", playlist_reformat_titles, nullptr);
    hook_associate ("set metadata_fallbacks", playlist_reformat_titles, nullptr);
    hook_associate ("set show_numbers_in_pl", playlist_reformat_titles, nullptr);
}

void playlist_enable_scan (bool enable)
{
    ENTER;

    scan_enabled_nominal = enable;
    scan_enabled = scan_enabled_nominal && ! aud_get_bool (nullptr, "metadata_on_play");
    scan_restart ();

    LEAVE;
}

void playlist_end ()
{
    hook_dissociate ("set metadata_on_play", playlist_trigger_scan);
    hook_dissociate ("set generic_title_format", playlist_reformat_titles);
    hook_dissociate ("set leading_zero", playlist_reformat_titles);
    hook_dissociate ("set show_hours", playlist_reformat_titles);
    hook_dissociate ("set metadata_fallbacks", playlist_reformat_titles);
    hook_dissociate ("set show_numbers_in_pl", playlist_reformat_titles);

    playlist_cache_clear ();

    ENTER;

    /* playback should already be stopped */
    assert (! playing_id);

    queued_update.stop ();

    active_id = nullptr;
    resume_playlist = -1;
    resume_paused = false;

    playlists.clear ();
    id_table.clear ();

    PlaylistEntry::cleanup ();

    LEAVE;
}

EXPORT int Playlist::index () const
{
    ENTER_GET_PLAYLIST (-1);
    int at = m_id->index;
    RETURN (at);
}

EXPORT int PlaylistEx::stamp () const
{
    ENTER_GET_PLAYLIST (-1);
    int stamp = m_id->stamp;
    RETURN (stamp);
}

EXPORT int Playlist::n_playlists ()
{
    ENTER;
    int count = playlists.len ();
    RETURN (count);
}

EXPORT Playlist Playlist::by_index (int at)
{
    ENTER;
    Playlist::ID * id = (at >= 0 && at < playlists.len ()) ? playlists[at]->id () : nullptr;
    RETURN (Playlist (id));
}

static Playlist::ID * insert_playlist_locked (int at, int stamp = -1)
{
    if (at < 0 || at > playlists.len ())
        at = playlists.len ();

    auto id = create_playlist (stamp);

    playlists.insert (at, 1);
    playlists[at].capture (id->data);

    number_playlists (at, playlists.len () - at);

    /* this will only happen at startup */
    if (! active_id)
        active_id = id;

    queue_update (Playlist::Structure, id->data, 0, 0);

    return id;
}

static Playlist::ID * get_blank_locked ()
{
    if (! strcmp (active_id->data->title, _(default_title)) && ! active_id->data->entries.len ())
        return active_id;

    return insert_playlist_locked (active_id->index + 1);
}

Playlist PlaylistEx::insert_with_stamp (int at, int stamp)
{
    ENTER;
    auto id = insert_playlist_locked (at, stamp);
    RETURN (Playlist (id));
}

EXPORT Playlist Playlist::insert_playlist (int at)
{
    ENTER;
    auto id = insert_playlist_locked (at);
    RETURN (Playlist (id));
}

EXPORT void Playlist::reorder_playlists (int from, int to, int count)
{
    ENTER;

    if (from < 0 || from + count > playlists.len () || to < 0 || to +
     count > playlists.len () || count < 0)
        RETURN ();

    Index<SmartPtr<PlaylistData>> displaced;

    if (to < from)
        displaced.move_from (playlists, to, -1, from - to, true, false);
    else
        displaced.move_from (playlists, from + count, -1, to - from, true, false);

    playlists.shift (from, to, count);

    if (to < from)
    {
        playlists.move_from (displaced, 0, to + count, from - to, false, true);
        number_playlists (to, from + count - to);
    }
    else
    {
        playlists.move_from (displaced, 0, from, to - from, false, true);
        number_playlists (from, to + count - from);
    }

    queue_update (Structure, nullptr, 0, 0);
    LEAVE;
}

EXPORT void Playlist::remove_playlist () const
{
    ENTER_GET_PLAYLIST ();

    bool was_active = false;
    bool was_playing = false;

    int at = m_id->index;
    playlists.remove (at, 1);

    if (! playlists.len ())
        playlists.append (create_playlist (-1)->data);

    number_playlists (at, playlists.len () - at);

    if (m_id == active_id)
    {
        int active_num = aud::min (at, playlists.len () - 1);
        active_id = playlists[active_num]->id ();
        was_active = true;
    }

    if (m_id == playing_id)
    {
        playing_id = nullptr;
        stop_playback_locked ();
        was_playing = true;
    }

    queue_update (Structure, nullptr, 0, 0);
    LEAVE;

    if (was_active)
        hook_call ("playlist activate", nullptr);

    if (was_playing)
    {
        hook_call ("playlist set playing", nullptr);
        hook_call ("playback stop", nullptr);
    }
}

EXPORT void Playlist::set_filename (const char * filename) const
{
    ENTER_GET_PLAYLIST ();

    playlist->filename = String (filename);
    playlist->modified = true;

    queue_update (Metadata, nullptr, 0, 0);
    LEAVE;
}

EXPORT String Playlist::get_filename () const
{
    ENTER_GET_PLAYLIST (String ());
    String filename = playlist->filename;
    RETURN (filename);
}

EXPORT void Playlist::set_title (const char * title) const
{
    ENTER_GET_PLAYLIST ();

    playlist->title = String (title);
    playlist->modified = true;

    queue_update (Metadata, nullptr, 0, 0);
    LEAVE;
}

EXPORT String Playlist::get_title () const
{
    ENTER_GET_PLAYLIST (String ());
    String title = playlist->title;
    RETURN (title);
}

void PlaylistEx::set_modified (bool modified) const
{
    ENTER_GET_PLAYLIST ();
    playlist->modified = modified;
    LEAVE;
}

bool PlaylistEx::get_modified () const
{
    ENTER_GET_PLAYLIST (false);
    bool modified = playlist->modified;
    RETURN (modified);
}

EXPORT void Playlist::activate () const
{
    ENTER_GET_PLAYLIST ();

    bool changed = false;

    if (m_id != active_id)
    {
        active_id = m_id;
        changed = true;
    }

    LEAVE;

    if (changed)
        hook_call ("playlist activate", nullptr);
}

EXPORT Playlist Playlist::active_playlist ()
{
    ENTER;
    auto id = active_id;
    RETURN (Playlist (id));
}

EXPORT Playlist Playlist::new_playlist ()
{
    ENTER;
    int at = active_id->index + 1;
    auto id = insert_playlist_locked (at);
    active_id = id;
    LEAVE;

    hook_call ("playlist activate", nullptr);
    return Playlist (id);
}

static int set_playing_locked (Playlist::ID * id, bool paused)
{
    if (id == playing_id)
    {
        /* already playing, just need to pause/unpause */
        if (aud_drct_get_paused () != paused)
            aud_drct_pause ();

        return 0;
    }

    int playback_hooks = SetPlaylist;

    if (playing_id)
        playing_id->data->resume_time = aud_drct_get_time ();

    /* is there anything to play? */
    if (id && ! id->data->position)
    {
        if (next_song_locked (id->data, true, 0))
            playback_hooks |= SetPosition;
        else
            id = nullptr;
    }

    playing_id = id;

    if (id)
    {
        start_playback_locked (id->data->resume_time, paused);
        playback_hooks |= PlaybackBegin;
    }
    else
    {
        stop_playback_locked ();
        playback_hooks |= PlaybackStop;
    }

    return playback_hooks;
}

static void call_playback_hooks (Playlist playlist, int hooks)
{
    if ((hooks & SetPlaylist))
        hook_call ("playlist set playing", nullptr);
    if ((hooks & SetPosition))
        hook_call ("playlist position", aud::to_ptr (playlist));
    if ((hooks & PlaybackBegin))
        hook_call ("playback begin", nullptr);
    if ((hooks & PlaybackStop))
        hook_call ("playback stop", nullptr);
}

EXPORT void Playlist::start_playback (bool paused) const
{
    ENTER_GET_PLAYLIST ();
    int hooks = set_playing_locked (m_id, paused);
    LEAVE;

    call_playback_hooks (* this, hooks);
}

EXPORT void aud_drct_stop ()
{
    ENTER;
    int hooks = set_playing_locked (nullptr, false);
    LEAVE;

    call_playback_hooks (Playlist (), hooks);
}

EXPORT Playlist Playlist::playing_playlist ()
{
    ENTER;
    auto id = playing_id;
    RETURN (Playlist (id));
}

EXPORT Playlist Playlist::blank_playlist ()
{
    ENTER;
    auto id = get_blank_locked ();
    RETURN (Playlist (id));
}

EXPORT Playlist Playlist::temporary_playlist ()
{
    ENTER;

    const char * title = _(temp_title);
    ID * id = nullptr;

    for (auto & playlist : playlists)
    {
        if (! strcmp (playlist->title, title))
        {
            id = playlist->id ();
            break;
        }
    }

    if (! id)
    {
        id = get_blank_locked ();
        id->data->title = String (title);
    }

    RETURN (Playlist (id));
}

// updates playback state (while locked) if playlist position was changed
static int change_playback (Playlist::ID * id)
{
    int hooks = SetPosition;

    if (id == playing_id)
    {
        if (id->data->position)
        {
            start_playback_locked (0, aud_drct_get_paused ());
            hooks |= PlaybackBegin;
        }
        else
        {
            playing_id = nullptr;
            stop_playback_locked ();
            hooks |= SetPlaylist | PlaybackStop;
        }
    }

    return hooks;
}

EXPORT int Playlist::n_entries () const
{
    ENTER_GET_PLAYLIST (0);
    int count = playlist->entries.len ();
    RETURN (count);
}

void PlaylistEx::insert_flat_items (int at, Index<PlaylistAddItem> && items) const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    if (at < 0 || at > entries)
        at = entries;

    int number = items.len ();

    playlist->entries.insert (at, number);

    int i = at;
    for (auto & item : items)
    {
        PlaylistEntry * entry = new PlaylistEntry (std::move (item));
        playlist->entries[i ++].capture (entry);
        playlist->total_length += entry->length;
    }

    items.clear ();

    playlist->number_entries (at, entries + number - at);

    queue_update (Structure, playlist, at, number);
    LEAVE;
}

EXPORT void Playlist::remove_entries (int at, int number) const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    bool position_changed = false;
    int update_flags = 0, playback_hooks = 0;

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    if (playlist->position && playlist->position->number >= at &&
     playlist->position->number < at + number)
    {
        playlist->set_position (nullptr, false);
        position_changed = true;
    }

    if (playlist->focus && playlist->focus->number >= at &&
     playlist->focus->number < at + number)
    {
        if (at + number < entries)
            playlist->focus = playlist->entries[at + number].get ();
        else if (at > 0)
            playlist->focus = playlist->entries[at - 1].get ();
        else
            playlist->focus = nullptr;
    }

    for (int count = 0; count < number; count ++)
    {
        PlaylistEntry * entry = playlist->entries [at + count].get ();

        if (entry->queued)
        {
            playlist->queued.remove (playlist->queued.find (entry), 1);
            update_flags |= QueueChanged;
        }

        if (entry->selected)
        {
            playlist->selected_count --;
            playlist->selected_length -= entry->length;
        }

        playlist->total_length -= entry->length;
    }

    playlist->entries.remove (at, number);
    playlist->number_entries (at, entries - at - number);

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_locked (playlist, aud_get_bool (nullptr, "repeat"), at);

        playback_hooks = change_playback (m_id);
    }

    queue_update (Structure, playlist, at, 0, update_flags);
    LEAVE;

    call_playback_hooks (* this, playback_hooks);
}

EXPORT String Playlist::entry_filename (int entry_num) const
{
    ENTER_GET_ENTRY (String ());
    String filename = entry->filename;
    RETURN (filename);
}

EXPORT PluginHandle * Playlist::entry_decoder (int entry_num, GetMode mode, String * error) const
{
    ENTER;

    PlaylistEntry * entry = get_entry (m_id, entry_num, (mode == Wait), false);
    PluginHandle * decoder = entry ? entry->decoder : nullptr;

    if (error)
        * error = entry ? entry->error : String ();

    RETURN (decoder);
}

EXPORT Tuple Playlist::entry_tuple (int entry_num, GetMode mode, String * error) const
{
    ENTER;

    PlaylistEntry * entry = get_entry (m_id, entry_num, false, (mode == Wait));
    Tuple tuple = entry ? entry->tuple.ref () : Tuple ();

    if (error)
        * error = entry ? entry->error : String ();

    RETURN (tuple);
}

EXPORT void Playlist::set_position (int entry_num) const
{
    ENTER_GET_PLAYLIST ();

    PlaylistEntry * entry = playlist->lookup_entry (entry_num);
    playlist->set_position (entry, true);

    int hooks = change_playback (m_id);

    LEAVE;

    call_playback_hooks (* this, hooks);
}

EXPORT int Playlist::get_position () const
{
    ENTER_GET_PLAYLIST (-1);
    int position = playlist->position ? playlist->position->number : -1;
    RETURN (position);
}

EXPORT void Playlist::set_focus (int entry_num) const
{
    ENTER_GET_PLAYLIST ();

    PlaylistEntry * new_focus = playlist->lookup_entry (entry_num);
    if (new_focus == playlist->focus)
        RETURN ();

    int first = INT_MAX;
    int last = -1;

    if (playlist->focus)
    {
        first = aud::min (first, playlist->focus->number);
        last = aud::max (last, playlist->focus->number);
    }

    playlist->focus = new_focus;

    if (playlist->focus)
    {
        first = aud::min (first, playlist->focus->number);
        last = aud::max (last, playlist->focus->number);
    }

    if (first <= last)
        queue_update (Selection, playlist, first, last + 1 - first);

    LEAVE;
}

EXPORT int Playlist::get_focus () const
{
    ENTER_GET_PLAYLIST (-1);
    int focus = playlist->focus ? playlist->focus->number : -1;
    RETURN (focus);
}

EXPORT void Playlist::select_entry (int entry_num, bool selected) const
{
    ENTER_GET_ENTRY ();

    if (entry->selected == selected)
        RETURN ();

    entry->selected = selected;

    if (selected)
    {
        playlist->selected_count ++;
        playlist->selected_length += entry->length;
    }
    else
    {
        playlist->selected_count --;
        playlist->selected_length -= entry->length;
    }

    queue_update (Selection, playlist, entry_num, 1);
    LEAVE;
}

EXPORT bool Playlist::entry_selected (int entry_num) const
{
    ENTER_GET_ENTRY (false);
    bool selected = entry->selected;
    RETURN (selected);
}

EXPORT int Playlist::n_selected (int at, int number) const
{
    ENTER_GET_PLAYLIST (0);

    int entries = playlist->entries.len ();

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    int selected_count = 0;

    if (at == 0 && number == entries)
        selected_count = playlist->selected_count;
    else
    {
        for (int i = 0; i < number; i ++)
        {
            if (playlist->entries[at + i]->selected)
                selected_count ++;
        }
    }

    RETURN (selected_count);
}

EXPORT void Playlist::select_all (bool selected) const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (auto & entry : playlist->entries)
    {
        if ((selected && ! entry->selected) || (entry->selected && ! selected))
        {
            entry->selected = selected;
            first = aud::min (first, entry->number);
            last = entry->number;
        }
    }

    if (selected)
    {
        playlist->selected_count = entries;
        playlist->selected_length = playlist->total_length;
    }
    else
    {
        playlist->selected_count = 0;
        playlist->selected_length = 0;
    }

    if (first < entries)
        queue_update (Selection, playlist, first, last + 1 - first);

    LEAVE;
}

EXPORT int Playlist::shift_entries (int entry_num, int distance) const
{
    ENTER_GET_ENTRY (0);

    if (! entry->selected || ! distance)
        RETURN (0);

    int entries = playlist->entries.len ();
    int shift = 0, center, top, bottom;

    if (distance < 0)
    {
        for (center = entry_num; center > 0 && shift > distance; )
        {
            if (! playlist->entries[-- center]->selected)
                shift --;
        }
    }
    else
    {
        for (center = entry_num + 1; center < entries && shift < distance; )
        {
            if (! playlist->entries[center ++]->selected)
                shift ++;
        }
    }

    top = bottom = center;

    for (int i = 0; i < top; i ++)
    {
        if (playlist->entries[i]->selected)
            top = i;
    }

    for (int i = entries; i > bottom; i --)
    {
        if (playlist->entries[i - 1]->selected)
            bottom = i;
    }

    Index<SmartPtr<PlaylistEntry>> temp;

    for (int i = top; i < center; i ++)
    {
        if (! playlist->entries[i]->selected)
            temp.append (std::move (playlist->entries[i]));
    }

    for (int i = top; i < bottom; i ++)
    {
        if (playlist->entries[i] && playlist->entries[i]->selected)
            temp.append (std::move (playlist->entries[i]));
    }

    for (int i = center; i < bottom; i ++)
    {
        if (playlist->entries[i] && ! playlist->entries[i]->selected)
            temp.append (std::move (playlist->entries[i]));
    }

    playlist->entries.move_from (temp, 0, top, bottom - top, false, true);
    playlist->number_entries (top, bottom - top);

    queue_update (Structure, playlist, top, bottom - top);

    RETURN (shift);
}

EXPORT void Playlist::remove_selected () const
{
    ENTER_GET_PLAYLIST ();

    if (! playlist->selected_count)
        RETURN ();

    int entries = playlist->entries.len ();
    bool position_changed = false;
    int update_flags = 0, playback_hooks = 0;

    if (playlist->position && playlist->position->selected)
    {
        playlist->set_position (nullptr, false);
        position_changed = true;
    }

    playlist->focus = playlist->find_unselected_focus ();

    int before = 0;  // number of entries before first selected
    int after = 0;   // number of entries after last selected

    while (before < entries && ! playlist->entries[before]->selected)
        before ++;

    int to = before;

    for (int from = before; from < entries; from ++)
    {
        PlaylistEntry * entry = playlist->entries[from].get ();

        if (entry->selected)
        {
            if (entry->queued)
            {
                playlist->queued.remove (playlist->queued.find (entry), 1);
                update_flags |= QueueChanged;
            }

            playlist->total_length -= entry->length;
            after = 0;
        }
        else
        {
            playlist->entries[to ++] = std::move (playlist->entries[from]);
            after ++;
        }
    }

    entries = to;
    playlist->entries.remove (entries, -1);
    playlist->number_entries (before, entries - before);

    playlist->selected_count = 0;
    playlist->selected_length = 0;

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_locked (playlist, aud_get_bool (nullptr, "repeat"), entries - after);

        playback_hooks = change_playback (m_id);
    }

    queue_update (Structure, playlist, before, entries - after - before, update_flags);
    LEAVE;

    call_playback_hooks (* this, playback_hooks);
}

EXPORT void Playlist::reverse_order () const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries / 2; i ++)
        std::swap (playlist->entries[i], playlist->entries[entries - 1 - i]);

    playlist->number_entries (0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void Playlist::reverse_selected () const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    int top = 0;
    int bottom = entries - 1;

    while (1)
    {
        while (top < bottom && ! playlist->entries[top]->selected)
            top ++;
        while (top < bottom && ! playlist->entries[bottom]->selected)
            bottom --;

        if (top >= bottom)
            break;

        std::swap (playlist->entries[top ++], playlist->entries[bottom --]);
    }

    playlist->number_entries (0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void Playlist::randomize_order () const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries; i ++)
        std::swap (playlist->entries[i], playlist->entries[rand () % entries]);

    playlist->number_entries (0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void Playlist::randomize_selected () const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    Index<PlaylistEntry *> selected;

    for (auto & entry : playlist->entries)
    {
        if (entry->selected)
            selected.append (entry.get ());
    }

    int n_selected = selected.len ();

    for (int i = 0; i < n_selected; i ++)
    {
        int a = selected[i]->number;
        int b = selected[rand () % n_selected]->number;
        std::swap (playlist->entries[a], playlist->entries[b]);
    }

    playlist->number_entries (0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

enum {COMPARE_TYPE_FILENAME, COMPARE_TYPE_TUPLE, COMPARE_TYPE_TITLE};

struct CompareData {
    Playlist::StringCompareFunc filename_compare;
    Playlist::TupleCompareFunc tuple_compare;
};

static void sort_entries (Index<SmartPtr<PlaylistEntry>> & entries, CompareData * data)
{
    entries.sort ([data] (const SmartPtr<PlaylistEntry> & a, const SmartPtr<PlaylistEntry> & b) {
        if (data->filename_compare)
            return data->filename_compare (a->filename, b->filename);
        else
            return data->tuple_compare (a->tuple, b->tuple);
    });
}

static void sort (PlaylistData * playlist, CompareData * data)
{
    sort_entries (playlist->entries, data);
    playlist->number_entries (0, playlist->entries.len ());

    queue_update (Playlist::Structure, playlist, 0, playlist->entries.len ());
}

static void sort_selected (PlaylistData * playlist, CompareData * data)
{
    int entries = playlist->entries.len ();

    Index<SmartPtr<PlaylistEntry>> selected;

    for (auto & entry : playlist->entries)
    {
        if (entry->selected)
            selected.append (std::move (entry));
    }

    sort_entries (selected, data);

    int i = 0;
    for (auto & entry : playlist->entries)
    {
        if (! entry)
            entry = std::move (selected[i ++]);
    }

    playlist->number_entries (0, entries);
    queue_update (Playlist::Structure, playlist, 0, entries);
}

static bool entries_are_scanned (PlaylistData * playlist, bool selected)
{
    for (auto & entry : playlist->entries)
    {
        if (selected && ! entry->selected)
            continue;

        if (entry->tuple.state () == Tuple::Initial)
        {
            aud_ui_show_error (_("The playlist cannot be sorted because "
             "metadata scanning is still in progress (or has been disabled)."));
            return false;
        }
    }

    return true;
}

EXPORT void Playlist::sort_by_filename (StringCompareFunc compare) const
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {compare};
    sort (playlist, & data);

    LEAVE;
}

EXPORT void Playlist::sort_by_tuple (TupleCompareFunc compare) const
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, compare};
    if (entries_are_scanned (playlist, false))
        sort (playlist, & data);

    LEAVE;
}

EXPORT void Playlist::sort_selected_by_filename (StringCompareFunc compare) const
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {compare};
    ::sort_selected (playlist, & data);

    LEAVE;
}

EXPORT void Playlist::sort_selected_by_tuple (TupleCompareFunc compare) const
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, compare};
    if (entries_are_scanned (playlist, true))
        ::sort_selected (playlist, & data);

    LEAVE;
}

static void playlist_reformat_titles (void *, void *)
{
    ENTER;

    PlaylistEntry::update_formatting ();

    for (auto & playlist : playlists)
    {
        for (auto & entry : playlist->entries)
            entry->format ();

        queue_update (Playlist::Metadata, playlist.get (), 0, playlist->entries.len ());
    }

    LEAVE;
}

static void playlist_trigger_scan (void *, void *)
{
    ENTER;

    scan_enabled = scan_enabled_nominal && ! aud_get_bool (nullptr, "metadata_on_play");
    scan_restart ();

    LEAVE;
}

static void playlist_rescan_real (PlaylistData * playlist, bool selected)
{
    for (auto & entry : playlist->entries)
    {
        if (! selected || entry->selected)
            playlist->set_entry_tuple (entry.get (), Tuple ());
    }

    queue_update (Playlist::Metadata, playlist, 0, playlist->entries.len ());
    scan_queue_playlist (playlist);
    scan_restart ();
}

EXPORT void Playlist::rescan_all () const
{
    ENTER_GET_PLAYLIST ();
    playlist_rescan_real (playlist, false);
    LEAVE;
}

EXPORT void Playlist::rescan_selected () const
{
    ENTER_GET_PLAYLIST ();
    playlist_rescan_real (playlist, true);
    LEAVE;
}

EXPORT void Playlist::rescan_file (const char * filename)
{
    ENTER;

    bool restart = false;

    for (auto & playlist : playlists)
    {
        bool queue = false;

        for (auto & entry : playlist->entries)
        {
            if (! strcmp (entry->filename, filename))
            {
                playlist->set_entry_tuple (entry.get (), Tuple ());
                queue_update (Metadata, playlist.get (), entry->number, 1);
                queue = true;
            }
        }

        if (queue)
        {
            scan_queue_playlist (playlist.get ());
            restart = true;
        }
    }

    if (restart)
        scan_restart ();

    LEAVE;
}

EXPORT int64_t Playlist::total_length_ms () const
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->total_length;
    RETURN (length);
}

EXPORT int64_t Playlist::selected_length_ms () const
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->selected_length;
    RETURN (length);
}

EXPORT int Playlist::n_queued () const
{
    ENTER_GET_PLAYLIST (0);
    int count = playlist->queued.len ();
    RETURN (count);
}

EXPORT void Playlist::queue_insert (int at, int entry_num) const
{
    ENTER_GET_ENTRY ();

    if (entry->queued || at > playlist->queued.len ())
        RETURN ();

    if (at < 0)
        playlist->queued.append (entry);
    else
    {
        playlist->queued.insert (at, 1);
        playlist->queued[at] = entry;
    }

    entry->queued = true;

    queue_update (Selection, playlist, entry_num, 1, QueueChanged);
    LEAVE;
}

EXPORT void Playlist::queue_insert_selected (int at) const
{
    ENTER_GET_PLAYLIST ();

    if (at > playlist->queued.len ())
        RETURN ();

    Index<PlaylistEntry *> add;
    int first = playlist->entries.len ();
    int last = 0;

    for (auto & entry : playlist->entries)
    {
        if (! entry->selected || entry->queued)
            continue;

        add.append (entry.get ());
        entry->queued = true;
        first = aud::min (first, entry->number);
        last = entry->number;
    }

    playlist->queued.move_from (add, 0, at, -1, true, true);

    if (first < playlist->entries.len ())
        queue_update (Selection, playlist, first, last + 1 - first, QueueChanged);

    LEAVE;
}

EXPORT int Playlist::queue_get_entry (int at) const
{
    ENTER_GET_PLAYLIST (-1);

    int entry_num = -1;
    if (at >= 0 && at < playlist->queued.len ())
        entry_num = playlist->queued[at]->number;

    RETURN (entry_num);
}

EXPORT int Playlist::queue_find_entry (int entry_num) const
{
    ENTER_GET_ENTRY (-1);
    int pos = entry->queued ? playlist->queued.find (entry) : -1;
    RETURN (pos);
}

EXPORT void Playlist::queue_remove (int at, int number) const
{
    ENTER_GET_PLAYLIST ();

    int queue_len = playlist->queued.len ();

    if (at < 0 || at > queue_len)
        at = queue_len;
    if (number < 0 || number > queue_len - at)
        number = queue_len - at;

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (int i = at; i < at + number; i ++)
    {
        PlaylistEntry * entry = playlist->queued[i];
        entry->queued = false;
        first = aud::min (first, entry->number);
        last = entry->number;
    }

    playlist->queued.remove (at, number);

    if (first < entries)
        queue_update (Selection, playlist, first, last + 1 - first, QueueChanged);

    LEAVE;
}

EXPORT void Playlist::queue_remove_selected () const
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (int i = 0; i < playlist->queued.len ();)
    {
        PlaylistEntry * entry = playlist->queued[i];

        if (entry->selected)
        {
            playlist->queued.remove (i, 1);
            entry->queued = false;
            first = aud::min (first, entry->number);
            last = entry->number;
        }
        else
            i ++;
    }

    if (first < entries)
        queue_update (Selection, playlist, first, last + 1 - first, QueueChanged);

    LEAVE;
}

static bool shuffle_prev (PlaylistData * playlist)
{
    PlaylistEntry * found = nullptr;

    for (auto & entry : playlist->entries)
    {
        if (entry->shuffle_num && (! playlist->position ||
         entry->shuffle_num < playlist->position->shuffle_num) && (! found
         || entry->shuffle_num > found->shuffle_num))
            found = entry.get ();
    }

    if (! found)
        return false;

    playlist->set_position (found, false);
    return true;
}

bool PlaylistEx::prev_song () const
{
    ENTER_GET_PLAYLIST (false);

    if (aud_get_bool (nullptr, "shuffle"))
    {
        if (! shuffle_prev (playlist))
            RETURN (false);
    }
    else
    {
        if (! playlist->position || playlist->position->number == 0)
            RETURN (false);

        playlist->set_position (playlist->entries[playlist->position->number - 1].get (), true);
    }

    int hooks = change_playback (m_id);

    LEAVE;

    call_playback_hooks (* this, hooks);
    return true;
}

static bool shuffle_next (PlaylistData * playlist)
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

    if (playlist->position)
    {
        // step #1: check to see if the shuffle order is already established
        PlaylistEntry * next = nullptr;

        for (auto & entry : playlist->entries)
        {
            if (entry->shuffle_num > playlist->position->shuffle_num &&
             (! next || entry->shuffle_num < next->shuffle_num))
                next = entry.get ();
        }

        if (next)
        {
            playlist->set_position (next, false);
            return true;
        }

        // step #2: check to see if we should advance to the next entry
        if (by_album && playlist->position->number + 1 < playlist->entries.len ())
        {
            next = playlist->entries[playlist->position->number + 1].get ();

            if (! next->shuffle_num && same_album (playlist->position->tuple, next->tuple))
            {
                playlist->set_position (next, true);
                return true;
            }
        }
    }

    // step #3: count the number of possible shuffle choices
    int choices = 0;
    PlaylistEntry * prev = nullptr;

    for (auto & entry : playlist->entries)
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

    for (auto & entry : playlist->entries)
    {
        if (is_choice (prev, entry.get ()))
        {
            if (! choices)
            {
                playlist->set_position (entry.get (), true);
                return true;
            }

            choices --;
        }

        prev = entry.get ();
    }

    return false;  // never reached
}

static void shuffle_reset (PlaylistData * playlist)
{
    playlist->last_shuffle_num = 0;

    for (auto & entry : playlist->entries)
        entry->shuffle_num = 0;
}

static bool next_song_locked (PlaylistData * playlist, bool repeat, int hint)
{
    int entries = playlist->entries.len ();
    if (! entries)
        return false;

    if (playlist->queued.len ())
    {
        playlist->set_position (playlist->queued[0], true);
        playlist->queued.remove (0, 1);
        playlist->position->queued = false;

        queue_update (Playlist::Selection, playlist, playlist->position->number, 1, QueueChanged);
    }
    else if (aud_get_bool (nullptr, "shuffle"))
    {
        if (! shuffle_next (playlist))
        {
            if (! repeat)
                return false;

            shuffle_reset (playlist);

            if (! shuffle_next (playlist))
                return false;
        }
    }
    else
    {
        if (hint >= entries)
        {
            if (! repeat)
                return false;

            hint = 0;
        }

        playlist->set_position (playlist->entries[hint].get (), true);
    }

    return true;
}

bool PlaylistEx::next_song (bool repeat) const
{
    ENTER_GET_PLAYLIST (false);

    int hint = playlist->position ? playlist->position->number + 1 : 0;

    if (! next_song_locked (playlist, repeat, hint))
        RETURN (false);

    int hooks = change_playback (m_id);

    LEAVE;

    call_playback_hooks (* this, hooks);
    return true;
}

static PlaylistEntry * get_playback_entry (int serial)
{
    if (! playback_check_serial (serial))
        return nullptr;

    return playing_id->data->position;
}

// called from playback thread
DecodeInfo playback_entry_read (int serial)
{
    ENTER;
    DecodeInfo dec;
    PlaylistEntry * entry;

    if ((entry = get_playback_entry (serial)))
    {
        ScanItem * item = scan_list_find_entry (entry);
        assert (item && item->for_playback);

        ScanRequest * request = item->request;
        item->handled_by_playback = true;

        LEAVE;
        request->run ();
        ENTER;

        if ((entry = get_playback_entry (serial)))
        {
            playback_set_info (entry->number, entry->tuple.ref ());
            art_cache_current (entry->filename, std::move (request->image_data),
             std::move (request->image_file));

            dec.filename = entry->filename;
            dec.ip = request->ip;
            dec.file = std::move (request->file);
            dec.error = std::move (request->error);
        }

        delete request;
    }

    RETURN (dec);
}

// called from playback thread
void playback_entry_set_tuple (int serial, Tuple && tuple)
{
    ENTER;
    PlaylistEntry * entry = get_playback_entry (serial);

    /* don't update cuesheet entries with stream metadata */
    if (entry && ! entry->tuple.is_set (Tuple::StartTime))
    {
        playing_id->data->set_entry_tuple (entry, std::move (tuple));
        queue_update (Playlist::Metadata, playing_id->data, entry->number, 1);
    }

    LEAVE;
}

void playlist_save_state ()
{
    /* get playback state before locking playlists */
    bool paused = aud_drct_get_paused ();
    int time = aud_drct_get_time ();

    ENTER;

    const char * user_dir = aud_get_path (AudPath::UserDir);
    StringBuf path = filename_build ({user_dir, STATE_FILE});

    FILE * handle = g_fopen (path, "w");
    if (! handle)
        RETURN ();

    fprintf (handle, "active %d\n", active_id ? active_id->index : -1);
    fprintf (handle, "playing %d\n", playing_id ? playing_id->index : -1);

    for (auto & playlist : playlists)
    {
        fprintf (handle, "playlist %d\n", playlist->id ()->index);

        if (playlist->filename)
            fprintf (handle, "filename %s\n", (const char *) playlist->filename);

        fprintf (handle, "position %d\n", playlist->position ? playlist->position->number : -1);

        /* resume state is stored per-playlist for historical reasons */
        bool is_playing = (playlist->id () == playing_id);
        fprintf (handle, "resume-state %d\n", (is_playing && paused) ? ResumePause : ResumePlay);
        fprintf (handle, "resume-time %d\n", is_playing ? time : playlist->resume_time);
    }

    fclose (handle);
    LEAVE;
}

void playlist_load_state ()
{
    ENTER;
    int playlist_num;

    const char * user_dir = aud_get_path (AudPath::UserDir);
    StringBuf path = filename_build ({user_dir, STATE_FILE});

    FILE * handle = g_fopen (path, "r");
    if (! handle)
        RETURN ();

    TextParser parser (handle);

    if (parser.get_int ("active", playlist_num))
    {
        if (playlist_num >= 0 && playlist_num < playlists.len ())
            active_id = playlists[playlist_num]->id ();

        parser.next ();
    }

    if (parser.get_int ("playing", resume_playlist))
        parser.next ();

    while (parser.get_int ("playlist", playlist_num) &&
           playlist_num >= 0 && playlist_num < playlists.len ())
    {
        PlaylistData * playlist = playlists[playlist_num].get ();
        int entries = playlist->entries.len ();

        parser.next ();

        playlist->filename = parser.get_str ("filename");
        if (playlist->filename)
            parser.next ();

        int position = -1;
        if (parser.get_int ("position", position))
            parser.next ();

        if (position >= 0 && position < entries)
            playlist->set_position (playlist->entries [position].get (), true);

        /* resume state is stored per-playlist for historical reasons */
        int resume_state = ResumePlay;
        if (parser.get_int ("resume-state", resume_state))
            parser.next ();

        if (playlist_num == resume_playlist)
        {
            if (resume_state == ResumeStop)
                resume_playlist = -1;
            if (resume_state == ResumePause)
                resume_paused = true;
        }

        if (parser.get_int ("resume-time", playlist->resume_time))
            parser.next ();
    }

    fclose (handle);

    /* set initial focus and selection */
    /* clear updates queued during init sequence */

    for (auto & playlist : playlists)
    {
        PlaylistEntry * focus = playlist->position;
        if (! focus && playlist->entries.len ())
            focus = playlist->entries[0].get ();

        if (focus)
        {
            focus->selected = true;
            playlist->focus = focus;
            playlist->selected_count = 1;
            playlist->selected_length = focus->length;
        }

        playlist->next_update = Playlist::Update ();
        playlist->last_update = Playlist::Update ();
    }

    queued_update.stop ();
    update_level = Playlist::NoUpdate;
    update_delayed = false;

    LEAVE;
}

EXPORT void aud_resume ()
{
    if (aud_get_bool (nullptr, "always_resume_paused"))
        resume_paused = true;

    Playlist::by_index (resume_playlist).start_playback (resume_paused);
}
