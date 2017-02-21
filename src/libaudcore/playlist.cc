/*
 * playlist.cc
 * Copyright 2009-2014 John Lindgren
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
#include "runtime.h"

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
#include "plugins.h"
#include "scanner.h"
#include "tuple.h"
#include "tuple-compiler.h"

using namespace Playlist;

enum {
    ResumeStop,
    ResumePlay,
    ResumePause
};

enum PlaybackChange {
    NoChange,
    NextSong,
    PlaybackStopped
};

enum {
    QueueChanged = (1 << 0),
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
    PlaylistData * playlist = lookup_playlist (playlist_num); \
    if (! playlist) \
        RETURN (__VA_ARGS__);

#define ENTER_GET_ENTRY(...) ENTER_GET_PLAYLIST (__VA_ARGS__); \
    Entry * entry = lookup_entry (playlist, entry_num); \
    if (! entry) \
        RETURN (__VA_ARGS__);

struct UniqueID
{
    constexpr UniqueID (int val) :
        val (val) {}

    operator int () const
        { return val; }

    unsigned hash () const
        { return int32_hash (val); }

private:
    int val;
};

struct Entry {
    Entry (PlaylistAddItem && item);
    ~Entry ();

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

struct PlaylistData {
    PlaylistData (int id);
    ~PlaylistData ();

    void set_entry_tuple (Entry * entry, Tuple && tuple);

    int number, unique_id;
    String filename, title;
    bool modified;
    Index<SmartPtr<Entry>> entries;
    Entry * position, * focus;
    int selected_count;
    int last_shuffle_num;
    Index<Entry *> queued;
    int64_t total_length, selected_length;
    bool scanning, scan_ending;
    Update next_update, last_update;
    int resume_time;
};

static const char * const default_title = N_("New Playlist");
static const char * const temp_title = N_("Now Playing");

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* The unique ID table contains pointers to PlaylistData for ID's in use and nullptr
 * for "dead" (previously used and therefore unavailable) ID's. */
static SimpleHash<UniqueID, PlaylistData *> unique_id_table;
static int next_unique_id = 1000;

static Index<SmartPtr<PlaylistData>> playlists;
static PlaylistData * active_playlist = nullptr;
static PlaylistData * playing_playlist = nullptr;
static int resume_playlist = -1;
static bool resume_paused = false;

static bool metadata_fallbacks = false;
static TupleCompiler title_formatter;

static QueuedFunc queued_update;
static UpdateLevel update_level;
static bool update_delayed;

struct ScanItem : public ListNode
{
    ScanItem (PlaylistData * playlist, Entry * entry, ScanRequest * request, bool for_playback) :
        playlist (playlist),
        entry (entry),
        request (request),
        for_playback (for_playback),
        handled_by_playback (false) {}

    PlaylistData * playlist;
    Entry * entry;
    ScanRequest * request;
    bool for_playback;
    bool handled_by_playback;
};

static bool scan_enabled_nominal, scan_enabled;
static int scan_playlist, scan_row;
static List<ScanItem> scan_list;

static void scan_finish (ScanRequest * request);
static void scan_cancel (Entry * entry);
static void scan_queue_playlist (PlaylistData * playlist);
static void scan_restart ();

static bool next_song_locked (PlaylistData * playlist, bool repeat, int hint);

static void playlist_reformat_titles (void * = nullptr, void * = nullptr);
static void playlist_trigger_scan (void * = nullptr, void * = nullptr);

void Entry::format ()
{
    tuple.delete_fallbacks ();

    if (metadata_fallbacks)
        tuple.generate_fallbacks ();
    else
        tuple.generate_title ();

    title_formatter.format (tuple);
}

void Entry::set_tuple (Tuple && new_tuple)
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

void PlaylistData::set_entry_tuple (Entry * entry, Tuple && tuple)
{
    total_length -= entry->length;
    if (entry->selected)
        selected_length -= entry->length;

    entry->set_tuple (std::move (tuple));

    total_length += entry->length;
    if (entry->selected)
        selected_length += entry->length;
}

Entry::Entry (PlaylistAddItem && item) :
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

Entry::~Entry ()
{
    scan_cancel (this);
}

static int new_unique_id (int preferred)
{
    if (preferred >= 0 && ! unique_id_table.lookup (preferred))
        return preferred;

    while (unique_id_table.lookup (next_unique_id))
        next_unique_id ++;

    return next_unique_id ++;
}

PlaylistData::PlaylistData (int id) :
    number (-1),
    unique_id (new_unique_id (id)),
    title (_(default_title)),
    modified (true),
    position (nullptr),
    focus (nullptr),
    selected_count (0),
    last_shuffle_num (0),
    total_length (0),
    selected_length (0),
    scanning (false),
    scan_ending (false),
    next_update (),
    last_update (),
    resume_time (0)
{
    unique_id_table.add (unique_id, (PlaylistData *) this);
}

PlaylistData::~PlaylistData ()
{
    unique_id_table.add (unique_id, nullptr);
}

static void number_playlists (int at, int length)
{
    for (int i = at; i < at + length; i ++)
        playlists[i]->number = i;
}

static PlaylistData * lookup_playlist (int i)
{
    return (i >= 0 && i < playlists.len ()) ? playlists[i].get () : nullptr;
}

static void number_entries (PlaylistData * p, int at, int length)
{
    for (int i = at; i < at + length; i ++)
        p->entries[i]->number = i;
}

static Entry * lookup_entry (PlaylistData * p, int i)
{
    return (i >= 0 && i < p->entries.len ()) ? p->entries[i].get () : nullptr;
}

static void update (void *)
{
    ENTER;

    for (auto & p : playlists)
    {
        p->last_update = p->next_update;
        p->next_update = Update ();
    }

    UpdateLevel level = update_level;
    update_level = NoUpdate;
    update_delayed = false;

    LEAVE;

    hook_call ("playlist update", aud::to_ptr (level));
}

static void queue_update (UpdateLevel level, PlaylistData * p, int at, int count, int flags = 0)
{
    if (p)
    {
        if (level == Structure)
            scan_queue_playlist (p);

        if (level >= Metadata)
        {
            if (p == playing_playlist && p->position)
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

    if (level == Structure)
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

EXPORT bool aud_playlist_update_pending (int playlist_num)
{
    if (playlist_num >= 0)
    {
        ENTER_GET_PLAYLIST (false);
        bool pending = playlist->next_update.level ? true : false;
        RETURN (pending);
    }
    else
    {
        ENTER;
        bool pending = update_level ? true : false;
        RETURN (pending);
    }
}

EXPORT Update aud_playlist_update_detail (int playlist_num)
{
    ENTER_GET_PLAYLIST (Update ());
    Update update = playlist->last_update;
    RETURN (update);
}

EXPORT bool aud_playlist_scan_in_progress (int playlist_num)
{
    if (playlist_num >= 0)
    {
        ENTER_GET_PLAYLIST (false);
        bool scanning = playlist->scanning || playlist->scan_ending;
        RETURN (scanning);
    }
    else
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

static ScanItem * scan_list_find_entry (Entry * entry)
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

static void scan_queue_entry (PlaylistData * playlist, Entry * entry, bool for_playback = false)
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
                Entry * entry = playlist->entries[scan_row ++].get ();

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
    Entry * entry = item->entry;

    scan_list.remove (item);

    if (! entry->decoder)
        entry->decoder = request->decoder;

    if (! entry->tuple.valid () && request->tuple.valid ())
    {
        playlist->set_entry_tuple (entry, std::move (request->tuple));
        queue_update (Metadata, playlist, entry->number, 1, DelayedUpdate);
    }

    if (! entry->decoder || ! entry->tuple.valid ())
        entry->error = request->error;

    if (entry->tuple.state () == Tuple::Initial)
    {
        entry->tuple.set_state (Tuple::Failed);
        queue_update (Metadata, playlist, entry->number, 1, DelayedUpdate);
    }

    delete item;

    scan_check_complete (playlist);
    scan_schedule ();

    pthread_cond_broadcast (& cond);

    LEAVE;
}

static void scan_cancel (Entry * entry)
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
static Entry * get_entry (int playlist_num, int entry_num,
 bool need_decoder, bool need_tuple)
{
    bool scan_started = false;

    while (1)
    {
        PlaylistData * playlist = lookup_playlist (playlist_num);
        Entry * entry = playlist ? lookup_entry (playlist, entry_num) : nullptr;

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

static void start_playback (int seek_time, bool pause)
{
    art_clear_current ();
    scan_reset_playback ();

    playback_play (seek_time, pause);

    // playback always begins with a rescan of the current entry in order to
    // open the file, ensure a valid tuple, and read album art
    scan_cancel (playing_playlist->position);
    scan_queue_entry (playing_playlist, playing_playlist->position, true);
}

static void stop_playback ()
{
    art_clear_current ();
    scan_reset_playback ();

    playback_stop ();
}

void playlist_init ()
{
    srand (time (nullptr));

    ENTER;

    update_level = NoUpdate;
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
    assert (! playing_playlist);

    queued_update.stop ();

    active_playlist = nullptr;
    resume_playlist = -1;
    resume_paused = false;

    playlists.clear ();
    unique_id_table.clear ();

    title_formatter.reset ();

    LEAVE;
}

EXPORT int aud_playlist_count ()
{
    ENTER;
    int count = playlists.len ();
    RETURN (count);
}

void playlist_insert_with_id (int at, int id)
{
    ENTER;

    if (at < 0 || at > playlists.len ())
        at = playlists.len ();

    auto playlist = new PlaylistData (id);
    playlists.insert (at, 1);
    playlists[at].capture (playlist);

    number_playlists (at, playlists.len () - at);

    queue_update (Structure, playlist, 0, 0);
    LEAVE;
}

EXPORT void aud_playlist_insert (int at)
{
    playlist_insert_with_id (at, -1);
}

EXPORT void aud_playlist_reorder (int from, int to, int count)
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

EXPORT void aud_playlist_delete (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    bool was_active = false;
    bool was_playing = false;

    playlists.remove (playlist_num, 1);

    if (! playlists.len ())
        playlists.append (SmartNew<PlaylistData> (-1));

    number_playlists (playlist_num, playlists.len () - playlist_num);

    if (playlist == active_playlist)
    {
        int active_num = aud::min (playlist_num, playlists.len () - 1);
        active_playlist = playlists[active_num].get ();
        was_active = true;
    }

    if (playlist == playing_playlist)
    {
        playing_playlist = nullptr;
        stop_playback ();
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

EXPORT int aud_playlist_get_unique_id (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int unique_id = playlist->unique_id;
    RETURN (unique_id);
}

EXPORT int aud_playlist_by_unique_id (int id)
{
    ENTER;

    PlaylistData * * ptr = unique_id_table.lookup (id);
    int num = (ptr && * ptr) ? (* ptr)->number : -1;

    RETURN (num);
}

EXPORT void aud_playlist_set_filename (int playlist_num, const char * filename)
{
    ENTER_GET_PLAYLIST ();

    playlist->filename = String (filename);
    playlist->modified = true;

    queue_update (Metadata, nullptr, 0, 0);
    LEAVE;
}

EXPORT String aud_playlist_get_filename (int playlist_num)
{
    ENTER_GET_PLAYLIST (String ());
    String filename = playlist->filename;
    RETURN (filename);
}

EXPORT void aud_playlist_set_title (int playlist_num, const char * title)
{
    ENTER_GET_PLAYLIST ();

    playlist->title = String (title);
    playlist->modified = true;

    queue_update (Metadata, nullptr, 0, 0);
    LEAVE;
}

EXPORT String aud_playlist_get_title (int playlist_num)
{
    ENTER_GET_PLAYLIST (String ());
    String title = playlist->title;
    RETURN (title);
}

void playlist_set_modified (int playlist_num, bool modified)
{
    ENTER_GET_PLAYLIST ();
    playlist->modified = modified;
    LEAVE;
}

bool playlist_get_modified (int playlist_num)
{
    ENTER_GET_PLAYLIST (false);
    bool modified = playlist->modified;
    RETURN (modified);
}

EXPORT void aud_playlist_set_active (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    bool changed = false;

    if (playlist != active_playlist)
    {
        changed = true;
        active_playlist = playlist;
    }

    LEAVE;

    if (changed)
        hook_call ("playlist activate", nullptr);
}

EXPORT int aud_playlist_get_active ()
{
    ENTER;
    int list = active_playlist ? active_playlist->number : -1;
    RETURN (list);
}

EXPORT int aud_playlist_new ()
{
    int playlist = aud_playlist_get_active () + 1;
    aud_playlist_insert (playlist);
    aud_playlist_set_active (playlist);
    return playlist;
}

EXPORT void aud_playlist_play (int playlist_num, bool paused)
{
    ENTER;

    PlaylistData * playlist = lookup_playlist (playlist_num);
    bool position_changed = false;

    if (playlist == playing_playlist)
    {
        /* already playing, just need to pause/unpause */
        if (aud_drct_get_paused () != paused)
            aud_drct_pause ();

        RETURN ();
    }

    if (playing_playlist)
        playing_playlist->resume_time = aud_drct_get_time ();

    /* is there anything to play? */
    if (playlist && ! playlist->position)
    {
        if (next_song_locked (playlist, true, 0))
            position_changed = true;
        else
            playlist = nullptr;
    }

    playing_playlist = playlist;

    if (playlist)
        start_playback (playlist->resume_time, paused);
    else
        stop_playback ();

    LEAVE;

    if (position_changed)
        hook_call ("playlist position", aud::to_ptr (playlist_num));

    hook_call ("playlist set playing", nullptr);

    if (playlist)
        hook_call ("playback begin", nullptr);
    else
        hook_call ("playback stop", nullptr);
}

EXPORT int aud_playlist_get_playing ()
{
    ENTER;
    int list = playing_playlist ? playing_playlist->number: -1;
    RETURN (list);
}

EXPORT int aud_playlist_get_blank ()
{
    int list = aud_playlist_get_active ();
    String title = aud_playlist_get_title (list);

    if (strcmp (title, _(default_title)) || aud_playlist_entry_count (list) > 0)
        aud_playlist_insert (++ list);

    return list;
}

EXPORT int aud_playlist_get_temporary ()
{
    int count = aud_playlist_count ();

    for (int list = 0; list < count; list ++)
    {
        String title = aud_playlist_get_title (list);
        if (! strcmp (title, _(temp_title)))
            return list;
    }

    int list = aud_playlist_get_blank ();
    aud_playlist_set_title (list, _(temp_title));
    return list;
}

static void set_position (PlaylistData * playlist, Entry * entry, bool update_shuffle)
{
    playlist->position = entry;
    playlist->resume_time = 0;

    /* move entry to top of shuffle list */
    if (entry && update_shuffle)
        entry->shuffle_num = ++ playlist->last_shuffle_num;
}

// updates playback state (while locked) if playlist position was changed
static PlaybackChange change_playback (PlaylistData * playlist)
{
    if (playlist != playing_playlist)
        return NoChange;

    if (playlist->position)
    {
        start_playback (0, aud_drct_get_paused ());
        return NextSong;
    }
    else
    {
        playing_playlist = nullptr;
        stop_playback ();
        return PlaybackStopped;
    }
}

// call hooks (while unlocked) if playback state was changed
static void call_playback_change_hooks (PlaybackChange change)
{
    if (change == NextSong)
        hook_call ("playback begin", nullptr);

    if (change == PlaybackStopped)
    {
        hook_call ("playlist set playing", nullptr);
        hook_call ("playback stop", nullptr);
    }
}

EXPORT int aud_playlist_entry_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int count = playlist->entries.len ();
    RETURN (count);
}

void playlist_entry_insert_batch_raw (int playlist_num, int at, Index<PlaylistAddItem> && items)
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
        Entry * entry = new Entry (std::move (item));
        playlist->entries[i ++].capture (entry);
        playlist->total_length += entry->length;
    }

    items.clear ();

    number_entries (playlist, at, entries + number - at);

    queue_update (Structure, playlist, at, number);
    LEAVE;
}

EXPORT void aud_playlist_entry_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    bool position_changed = false;
    int update_flags = 0;
    PlaybackChange change = NoChange;

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    if (playlist->position && playlist->position->number >= at &&
     playlist->position->number < at + number)
    {
        set_position (playlist, nullptr, false);
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
        Entry * entry = playlist->entries [at + count].get ();

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
    number_entries (playlist, at, entries - at - number);

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_locked (playlist, aud_get_bool (nullptr, "repeat"), at);

        change = change_playback (playlist);
    }

    queue_update (Structure, playlist, at, 0, update_flags);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", aud::to_ptr (playlist_num));

    call_playback_change_hooks (change);
}

EXPORT String aud_playlist_entry_get_filename (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (String ());
    String filename = entry->filename;
    RETURN (filename);
}

EXPORT PluginHandle * aud_playlist_entry_get_decoder (int playlist_num,
 int entry_num, GetMode mode, String * error)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, (mode == Wait), false);
    PluginHandle * decoder = entry ? entry->decoder : nullptr;

    if (error)
        * error = entry ? entry->error : String ();

    RETURN (decoder);
}

EXPORT Tuple aud_playlist_entry_get_tuple (int playlist_num, int entry_num,
 GetMode mode, String * error)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, false, (mode == Wait));
    Tuple tuple = entry ? entry->tuple.ref () : Tuple ();

    if (error)
        * error = entry ? entry->error : String ();

    RETURN (tuple);
}

EXPORT void aud_playlist_set_position (int playlist_num, int entry_num)
{
    ENTER_GET_PLAYLIST ();

    Entry * entry = lookup_entry (playlist, entry_num);
    set_position (playlist, entry, true);

    PlaybackChange change = change_playback (playlist);

    LEAVE;

    hook_call ("playlist position", aud::to_ptr (playlist_num));
    call_playback_change_hooks (change);
}

EXPORT int aud_playlist_get_position (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int position = playlist->position ? playlist->position->number : -1;
    RETURN (position);
}

EXPORT void aud_playlist_set_focus (int playlist_num, int entry_num)
{
    ENTER_GET_PLAYLIST ();

    Entry * new_focus = lookup_entry (playlist, entry_num);
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

EXPORT int aud_playlist_get_focus (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int focus = playlist->focus ? playlist->focus->number : -1;
    RETURN (focus);
}

EXPORT void aud_playlist_entry_set_selected (int playlist_num, int entry_num,
 bool selected)
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

EXPORT bool aud_playlist_entry_get_selected (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (false);
    bool selected = entry->selected;
    RETURN (selected);
}

EXPORT int aud_playlist_selected_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int selected_count = playlist->selected_count;
    RETURN (selected_count);
}

EXPORT int aud_playlist_selected_count (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST (0);

    int entries = playlist->entries.len ();

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    int selected_count = 0;
    for (int i = 0; i < number; i ++)
    {
        if (playlist->entries[at + i]->selected)
            selected_count ++;
    }

    RETURN (selected_count);
}

EXPORT void aud_playlist_select_all (int playlist_num, bool selected)
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

EXPORT int aud_playlist_shift (int playlist_num, int entry_num, int distance)
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

    Index<SmartPtr<Entry>> temp;

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

    number_entries (playlist, top, bottom - top);
    queue_update (Structure, playlist, top, bottom - top);

    RETURN (shift);
}

static Entry * find_unselected_focus (PlaylistData * playlist)
{
    if (! playlist->focus || ! playlist->focus->selected)
        return playlist->focus;

    int entries = playlist->entries.len ();

    for (int search = playlist->focus->number + 1; search < entries; search ++)
    {
        Entry * entry = playlist->entries[search].get ();
        if (! entry->selected)
            return entry;
    }

    for (int search = playlist->focus->number; search --;)
    {
        Entry * entry = playlist->entries[search].get ();
        if (! entry->selected)
            return entry;
    }

    return nullptr;
}

EXPORT void aud_playlist_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    if (! playlist->selected_count)
        RETURN ();

    int entries = playlist->entries.len ();
    bool position_changed = false;
    int update_flags = 0;
    PlaybackChange change = NoChange;

    if (playlist->position && playlist->position->selected)
    {
        set_position (playlist, nullptr, false);
        position_changed = true;
    }

    playlist->focus = find_unselected_focus (playlist);

    int before = 0;  // number of entries before first selected
    int after = 0;   // number of entries after last selected

    while (before < entries && ! playlist->entries[before]->selected)
        before ++;

    int to = before;

    for (int from = before; from < entries; from ++)
    {
        Entry * entry = playlist->entries[from].get ();

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
    number_entries (playlist, before, entries - before);

    playlist->selected_count = 0;
    playlist->selected_length = 0;

    if (position_changed)
    {
        if (aud_get_bool (nullptr, "advance_on_delete"))
            next_song_locked (playlist, aud_get_bool (nullptr, "repeat"), entries - after);

        change = change_playback (playlist);
    }

    queue_update (Structure, playlist, before, entries - after - before, update_flags);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", aud::to_ptr (playlist_num));

    call_playback_change_hooks (change);
}

EXPORT void aud_playlist_reverse (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries / 2; i ++)
        std::swap (playlist->entries[i], playlist->entries[entries - 1 - i]);

    number_entries (playlist, 0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void aud_playlist_reverse_selected (int playlist_num)
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

    number_entries (playlist, 0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void aud_playlist_randomize (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries; i ++)
        std::swap (playlist->entries[i], playlist->entries[rand () % entries]);

    number_entries (playlist, 0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

EXPORT void aud_playlist_randomize_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    Index<Entry *> selected;

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

    number_entries (playlist, 0, entries);
    queue_update (Structure, playlist, 0, entries);
    LEAVE;
}

enum {COMPARE_TYPE_FILENAME, COMPARE_TYPE_TUPLE, COMPARE_TYPE_TITLE};

struct CompareData {
    PlaylistStringCompareFunc filename_compare;
    PlaylistTupleCompareFunc tuple_compare;
};

static void sort_entries (Index<SmartPtr<Entry>> & entries, CompareData * data)
{
    entries.sort ([data] (const SmartPtr<Entry> & a, const SmartPtr<Entry> & b) {
        if (data->filename_compare)
            return data->filename_compare (a->filename, b->filename);
        else
            return data->tuple_compare (a->tuple, b->tuple);
    });
}

static void sort (PlaylistData * playlist, CompareData * data)
{
    sort_entries (playlist->entries, data);
    number_entries (playlist, 0, playlist->entries.len ());

    queue_update (Structure, playlist, 0, playlist->entries.len ());
}

static void sort_selected (PlaylistData * playlist, CompareData * data)
{
    int entries = playlist->entries.len ();

    Index<SmartPtr<Entry>> selected;

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

    number_entries (playlist, 0, entries);
    queue_update (Structure, playlist, 0, entries);
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

EXPORT void aud_playlist_sort_by_filename (int playlist_num, PlaylistStringCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {compare};
    sort (playlist, & data);

    LEAVE;
}

EXPORT void aud_playlist_sort_by_tuple (int playlist_num, PlaylistTupleCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, compare};
    if (entries_are_scanned (playlist, false))
        sort (playlist, & data);

    LEAVE;
}

EXPORT void aud_playlist_sort_selected_by_filename (int playlist_num,
 PlaylistStringCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {compare};
    sort_selected (playlist, & data);

    LEAVE;
}

EXPORT void aud_playlist_sort_selected_by_tuple (int playlist_num,
 PlaylistTupleCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, compare};
    if (entries_are_scanned (playlist, true))
        sort_selected (playlist, & data);

    LEAVE;
}

static void playlist_reformat_titles (void *, void *)
{
    ENTER;

    metadata_fallbacks = aud_get_bool (nullptr, "metadata_fallbacks");
    title_formatter.compile (aud_get_str (nullptr, "generic_title_format"));

    for (auto & playlist : playlists)
    {
        for (auto & entry : playlist->entries)
            entry->format ();

        queue_update (Metadata, playlist.get (), 0, playlist->entries.len ());
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

static void playlist_rescan_real (int playlist_num, bool selected)
{
    ENTER_GET_PLAYLIST ();

    for (auto & entry : playlist->entries)
    {
        if (! selected || entry->selected)
            playlist->set_entry_tuple (entry.get (), Tuple ());
    }

    queue_update (Metadata, playlist, 0, playlist->entries.len ());
    scan_queue_playlist (playlist);
    scan_restart ();

    LEAVE;
}

EXPORT void aud_playlist_rescan (int playlist_num)
{
    playlist_rescan_real (playlist_num, false);
}

EXPORT void aud_playlist_rescan_selected (int playlist_num)
{
    playlist_rescan_real (playlist_num, true);
}

EXPORT void aud_playlist_rescan_file (const char * filename)
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

EXPORT int64_t aud_playlist_get_total_length (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->total_length;
    RETURN (length);
}

EXPORT int64_t aud_playlist_get_selected_length (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->selected_length;
    RETURN (length);
}

EXPORT int aud_playlist_queue_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int count = playlist->queued.len ();
    RETURN (count);
}

EXPORT void aud_playlist_queue_insert (int playlist_num, int at, int entry_num)
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

EXPORT void aud_playlist_queue_insert_selected (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST ();

    if (at > playlist->queued.len ())
        RETURN ();

    Index<Entry *> add;
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

EXPORT int aud_playlist_queue_get_entry (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST (-1);

    int entry_num = -1;
    if (at >= 0 && at < playlist->queued.len ())
        entry_num = playlist->queued[at]->number;

    RETURN (entry_num);
}

EXPORT int aud_playlist_queue_find_entry (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (-1);
    int pos = entry->queued ? playlist->queued.find (entry) : -1;
    RETURN (pos);
}

EXPORT void aud_playlist_queue_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    if (at < 0 || number < 0 || at + number > playlist->queued.len ())
        RETURN ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (int i = at; i < at + number; i ++)
    {
        Entry * entry = playlist->queued[i];
        entry->queued = false;
        first = aud::min (first, entry->number);
        last = entry->number;
    }

    playlist->queued.remove (at, number);

    if (first < entries)
        queue_update (Selection, playlist, first, last + 1 - first, QueueChanged);

    LEAVE;
}

EXPORT void aud_playlist_queue_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (int i = 0; i < playlist->queued.len ();)
    {
        Entry * entry = playlist->queued[i];

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
    Entry * found = nullptr;

    for (auto & entry : playlist->entries)
    {
        if (entry->shuffle_num && (! playlist->position ||
         entry->shuffle_num < playlist->position->shuffle_num) && (! found
         || entry->shuffle_num > found->shuffle_num))
            found = entry.get ();
    }

    if (! found)
        return false;

    set_position (playlist, found, false);
    return true;
}

bool playlist_prev_song (int playlist_num)
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

        set_position (playlist, playlist->entries[playlist->position->number - 1].get (), true);
    }

    PlaybackChange change = change_playback (playlist);

    LEAVE;

    hook_call ("playlist position", aud::to_ptr (playlist_num));
    call_playback_change_hooks (change);
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
    auto is_choice = [&] (Entry * prev, Entry * entry)
    {
        return (! entry->shuffle_num) && (! by_album || ! prev ||
         prev->shuffle_num || ! same_album (prev->tuple, entry->tuple));
    };

    if (playlist->position)
    {
        // step #1: check to see if the shuffle order is already established
        Entry * next = nullptr;

        for (auto & entry : playlist->entries)
        {
            if (entry->shuffle_num > playlist->position->shuffle_num &&
             (! next || entry->shuffle_num < next->shuffle_num))
                next = entry.get ();
        }

        if (next)
        {
            set_position (playlist, next, false);
            return true;
        }

        // step #2: check to see if we should advance to the next entry
        if (by_album && playlist->position->number + 1 < playlist->entries.len ())
        {
            next = playlist->entries[playlist->position->number + 1].get ();

            if (! next->shuffle_num && same_album (playlist->position->tuple, next->tuple))
            {
                set_position (playlist, next, true);
                return true;
            }
        }
    }

    // step #3: count the number of possible shuffle choices
    int choices = 0;
    Entry * prev = nullptr;

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
                set_position (playlist, entry.get (), true);
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
        set_position (playlist, playlist->queued[0], true);
        playlist->queued.remove (0, 1);
        playlist->position->queued = false;

        queue_update (Selection, playlist, playlist->position->number, 1, QueueChanged);
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

        set_position (playlist, playlist->entries[hint].get (), true);
    }

    return true;
}

bool playlist_next_song (int playlist_num, bool repeat)
{
    ENTER_GET_PLAYLIST (false);

    int hint = playlist->position ? playlist->position->number + 1 : 0;

    if (! next_song_locked (playlist, repeat, hint))
        RETURN (false);

    PlaybackChange change = change_playback (playlist);

    LEAVE;

    hook_call ("playlist position", aud::to_ptr (playlist_num));
    call_playback_change_hooks (change);
    return true;
}

static Entry * get_playback_entry (int serial)
{
    if (! playback_check_serial (serial))
        return nullptr;

    assert (playing_playlist && playing_playlist->position);
    return playing_playlist->position;
}

// called from playback thread
DecodeInfo playback_entry_read (int serial)
{
    ENTER;
    DecodeInfo dec;
    Entry * entry;

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
    Entry * entry = get_playback_entry (serial);

    /* don't update cuesheet entries with stream metadata */
    if (entry && ! entry->tuple.is_set (Tuple::StartTime))
    {
        playing_playlist->set_entry_tuple (entry, std::move (tuple));
        queue_update (Metadata, playing_playlist, entry->number, 1);
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

    fprintf (handle, "active %d\n", active_playlist ? active_playlist->number : -1);
    fprintf (handle, "playing %d\n", playing_playlist ? playing_playlist->number : -1);

    for (auto & playlist : playlists)
    {
        fprintf (handle, "playlist %d\n", playlist->number);

        if (playlist->filename)
            fprintf (handle, "filename %s\n", (const char *) playlist->filename);

        fprintf (handle, "position %d\n", playlist->position ? playlist->position->number : -1);

        /* resume state is stored per-playlist for historical reasons */
        bool is_playing = (playlist.get () == playing_playlist);
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
        if (! (active_playlist = lookup_playlist (playlist_num)))
            active_playlist = playlists[0].get ();
        parser.next ();
    }

    if (parser.get_int ("playing", resume_playlist))
        parser.next ();

    while (parser.get_int ("playlist", playlist_num) && playlist_num >= 0 &&
     playlist_num < playlists.len ())
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
            set_position (playlist, playlist->entries [position].get (), true);

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
        Entry * focus = playlist->position;
        if (! focus && playlist->entries.len ())
            focus = playlist->entries[0].get ();

        if (focus)
        {
            focus->selected = true;
            playlist->focus = focus;
            playlist->selected_count = 1;
            playlist->selected_length = focus->length;
        }

        playlist->next_update = Update ();
        playlist->last_update = Update ();
    }

    queued_update.stop ();
    update_level = NoUpdate;
    update_delayed = false;

    LEAVE;
}

EXPORT void aud_resume ()
{
    if (aud_get_bool (nullptr, "always_resume_paused"))
        resume_paused = true;

    aud_playlist_play (resume_playlist, resume_paused);
}
