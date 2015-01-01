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

// uncomment to print a backtrace when scanning blocks the main thread
// #define WARN_BLOCKED

#include "playlist-internal.h"
#include "runtime.h"

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
#include "plugins.h"
#include "scanner.h"
#include "tuple.h"
#include "tuple-compiler.h"

#ifdef WARN_BLOCKED
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#endif

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

    void set_tuple (Tuple && new_tuple);
    void set_failed (String && new_error);

    String filename;
    PluginHandle * decoder;
    Tuple tuple;
    String error;
    int number;
    int length;
    int shuffle_num;
    bool scanned, failed;
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

#ifdef WARN_BLOCKED
static pthread_t main_thread;
#endif

/* The unique ID table contains pointers to PlaylistData for ID's in use and nullptr
 * for "dead" (previously used and therefore unavailable) ID's. */
static SimpleHash<UniqueID, PlaylistData *> unique_id_table;
static int next_unique_id = 1000;

static Index<SmartPtr<PlaylistData>> playlists;
static PlaylistData * active_playlist = nullptr;
static PlaylistData * playing_playlist = nullptr;
static int resume_playlist = -1;
static bool resume_paused = false;

static QueuedFunc queued_update;
static UpdateLevel update_level;

struct ScanItem : public ListNode
{
    ScanItem (PlaylistData * playlist, Entry * entry, ScanRequest * request) :
        playlist (playlist),
        entry (entry),
        request (request) {}

    PlaylistData * playlist;
    Entry * entry;
    ScanRequest * request;
};

static bool scan_enabled;
static int scan_playlist, scan_row;
static List<ScanItem> scan_list;

static void scan_finish (ScanRequest * request);
static void scan_cancel (Entry * entry);
static void scan_queue_playlist (PlaylistData * playlist);
static void scan_restart ();

static bool next_song_locked (PlaylistData * playlist, bool repeat, int hint);

static void playlist_reformat_titles ();
static void playlist_trigger_scan ();

static SmartPtr<TupleCompiler> title_formatter;

void Entry::set_tuple (Tuple && new_tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (tuple.get_value_type (Tuple::StartTime) == Tuple::Int)
        return;

    scanned = (bool) new_tuple;
    failed = false;
    error = String ();

    if (! new_tuple)
        new_tuple.set_filename (filename);

    new_tuple.generate_fallbacks ();
    title_formatter->format (new_tuple);

    length = aud::max (0, new_tuple.get_int (Tuple::Length));
    tuple = std::move (new_tuple);
}

void PlaylistData::set_entry_tuple (Entry * entry, Tuple && tuple)
{
    scan_cancel (entry);

    total_length -= entry->length;
    if (entry->selected)
        selected_length -= entry->length;

    entry->set_tuple (std::move (tuple));

    total_length += entry->length;
    if (entry->selected)
        selected_length += entry->length;
}

void Entry::set_failed (String && new_error)
{
    scanned = true;
    failed = true;
    error = std::move (new_error);
}

Entry::Entry (PlaylistAddItem && item) :
    filename (item.filename),
    decoder (item.decoder),
    number (-1),
    length (0),
    shuffle_num (0),
    scanned (false),
    failed (false),
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

    LEAVE;

    hook_call ("playlist update", aud::to_ptr (level));
}

static bool send_playback_info (Entry * entry)
{
    // if the entry was not scanned or failed to scan, we must still call
    // playback_set_info() in order to update the entry number
    Tuple tuple = (entry->scanned && ! entry->failed) ? entry->tuple.ref () : Tuple ();
    return playback_set_info (entry->number, entry->filename, entry->decoder, std::move (tuple));
}

static void queue_update (UpdateLevel level, PlaylistData * p, int at,
 int count, bool queue_changed = false)
{
    if (p)
    {
        if (level == Structure)
            scan_queue_playlist (p);

        if (level >= Metadata)
        {
            if (p == playing_playlist && p->position)
                send_playback_info (p->position);

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

        if (queue_changed)
            p->next_update.queue_changed = true;
    }

    if (level == Structure)
        scan_restart ();

    if (! update_level)
        queued_update.queue (update, nullptr);

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

static void scan_queue_entry (PlaylistData * playlist, Entry * entry)
{
    int flags = entry->scanned ? 0 : SCAN_TUPLE;
    auto request = new ScanRequest (entry->filename, flags, scan_finish, entry->decoder);
    scanner_request (request);

    scan_list.append (new ScanItem (playlist, entry, request));
}

static void scan_check_complete (PlaylistData * playlist)
{
    if (! playlist->scan_ending || scan_list_find_playlist (playlist))
        return;

    playlist->scan_ending = false;
    event_queue_cancel ("playlist scan complete", nullptr);
    event_queue ("playlist scan complete", nullptr);
}

static bool scan_queue_next_entry ()
{
    if (! scan_enabled || aud_get_bool (nullptr, "metadata_on_play"))
        return false;

    while (scan_playlist < playlists.len ())
    {
        PlaylistData * playlist = playlists[scan_playlist].get ();

        if (playlist->scanning)
        {
            while (scan_row < playlist->entries.len ())
            {
                Entry * entry = playlist->entries[scan_row ++].get ();

                if (! entry->scanned && ! scan_list_find_entry (entry))
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
    delete item;

    if (! entry->decoder)
        entry->decoder = request->decoder;

    if (! entry->scanned && request->tuple)
    {
        playlist->set_entry_tuple (entry, std::move (request->tuple));
        queue_update (Metadata, playlist, entry->number, 1);
    }

    if (! entry->decoder || ! entry->scanned)
        entry->set_failed (std::move (request->error));

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

#ifdef WARN_BLOCKED
static void warn_main_thread_blocked ()
{
    printf ("\nMain thread blocked, backtrace:\n");

    void * syms[100];
    int n_syms = backtrace (syms, aud::n_elems (syms));
    char * * names = backtrace_symbols (syms, n_syms);

    for (int i = 0; i < n_syms; i ++)
        printf ("%d. %s\n", i, names[i]);

    free (names);
}
#endif

/* mutex may be unlocked during the call */
static Entry * get_entry (int playlist_num, int entry_num,
 bool need_decoder, bool need_tuple)
{
#ifdef WARN_BLOCKED
    if ((need_decoder || need_tuple) && pthread_self () == main_thread)
        warn_main_thread_blocked ();
#endif

    while (1)
    {
        PlaylistData * playlist = lookup_playlist (playlist_num);
        Entry * entry = playlist ? lookup_entry (playlist, entry_num) : nullptr;

        if (! entry || entry->failed)
            return entry;

        if ((need_decoder && ! entry->decoder) || (need_tuple && ! entry->scanned))
        {
            if (! scan_list_find_entry (entry))
                scan_queue_entry (playlist, entry);

            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        return entry;
    }
}

/* mutex may be unlocked during the call */
static Entry * get_playback_entry (int serial)
{
    while (1)
    {
        if (! playback_check_serial (serial))
            return nullptr;

        Entry * entry = playing_playlist ? playing_playlist->position : nullptr;

        if (! entry || entry->failed)
            return entry;

        if (! entry->decoder || ! entry->scanned)
        {
            if (! scan_list_find_entry (entry))
                scan_queue_entry (playing_playlist, entry);

            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        return entry;
    }
}

void playlist_init ()
{
    srand (time (nullptr));

#ifdef WARN_BLOCKED
    main_thread = pthread_self ();
#endif

    ENTER;

    update_level = NoUpdate;
    scan_enabled = false;
    scan_playlist = scan_row = 0;

    title_formatter.capture (new TupleCompiler);

    LEAVE;

    /* initialize title formatter */
    playlist_reformat_titles ();

    hook_associate ("set metadata_on_play", (HookFunction) playlist_trigger_scan, nullptr);
    hook_associate ("set generic_title_format", (HookFunction) playlist_reformat_titles, nullptr);
    hook_associate ("set show_numbers_in_pl", (HookFunction) playlist_reformat_titles, nullptr);
    hook_associate ("set leading_zero", (HookFunction) playlist_reformat_titles, nullptr);
}

void playlist_enable_scan (bool enable)
{
    ENTER;

    if (! enable)
        scan_list.clear ();

    scan_enabled = enable;

    LEAVE;

    if (enable)
        playlist_trigger_scan ();
}

void playlist_end ()
{
    hook_dissociate ("set metadata_on_play", (HookFunction) playlist_trigger_scan);
    hook_dissociate ("set generic_title_format", (HookFunction) playlist_reformat_titles);
    hook_dissociate ("set show_numbers_in_pl", (HookFunction) playlist_reformat_titles);
    hook_dissociate ("set leading_zero", (HookFunction) playlist_reformat_titles);

    ENTER;

    queued_update.stop ();

    active_playlist = playing_playlist = nullptr;
    resume_playlist = -1;

    playlists.clear ();
    unique_id_table.clear ();

    title_formatter.clear ();

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
        playback_stop ();
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
        playback_play (playlist->resume_time, paused);
    else
        playback_stop ();

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
    {
        list = aud_playlist_count ();
        aud_playlist_insert (list);
    }

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
        playback_play (0, aud_drct_get_paused ());
        return NextSong;
    }
    else
    {
        playing_playlist = nullptr;
        playback_stop ();
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
    bool position_changed = false, queue_changed = false;
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
            queue_changed = true;
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

    queue_update (Structure, playlist, at, 0, queue_changed);
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

    const bool wait = (mode == Wait || mode == WaitGuess);

    Entry * entry = get_entry (playlist_num, entry_num, wait, false);
    PluginHandle * decoder = entry ? entry->decoder : nullptr;

    if (error)
        * error = entry ? entry->error : String ();

    RETURN (decoder);
}

EXPORT Tuple aud_playlist_entry_get_tuple (int playlist_num, int entry_num,
 GetMode mode, String * error)
{
    ENTER;

    const bool wait = (mode == Wait || mode == WaitGuess);
    const bool guess = (mode == Guess || mode == WaitGuess);

    Entry * entry = get_entry (playlist_num, entry_num, false, wait);

    Tuple tuple;
    if (entry && ((entry->scanned && ! entry->failed) || guess))
        tuple = entry->tuple.ref ();

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

    int first = INT_MAX;
    int last = -1;

    if (playlist->focus)
    {
        first = aud::min (first, playlist->focus->number);
        last = aud::max (last, playlist->focus->number);
    }

    playlist->focus = lookup_entry (playlist, entry_num);

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
        playlist->selected_count++;
        playlist->selected_length += entry->length;
    }
    else
    {
        playlist->selected_count--;
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
    bool position_changed = false, queue_changed = false;
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
                queue_changed = true;
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

    queue_update (Structure, playlist, before, entries - after - before, queue_changed);
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

static int compare_cb (const SmartPtr<Entry> & a, const SmartPtr<Entry> & b, void * _data)
{
    CompareData * data = (CompareData *) _data;

    int diff = 0;

    if (data->filename_compare)
        diff = data->filename_compare (a->filename, b->filename);
    else if (data->tuple_compare)
        diff = data->tuple_compare (a->tuple, b->tuple);

    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static void sort (PlaylistData * playlist, CompareData * data)
{
    playlist->entries.sort (compare_cb, data);
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

    selected.sort (compare_cb, data);

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

        if (! entry->scanned)
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

static void playlist_reformat_titles ()
{
    ENTER;

    String format = aud_get_str (nullptr, "generic_title_format");
    title_formatter->compile (format);

    for (auto & playlist : playlists)
    {
        for (auto & entry : playlist->entries)
            title_formatter->format (entry->tuple);

        queue_update (Metadata, playlist.get (), 0, playlist->entries.len ());
    }

    LEAVE;
}

static void playlist_trigger_scan ()
{
    ENTER;

    for (auto & playlist : playlists)
        scan_queue_playlist (playlist.get ());

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

    queue_update (Selection, playlist, entry_num, 1, true);
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
        queue_update (Selection, playlist, first, last + 1 - first, true);

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
        queue_update (Selection, playlist, first, last + 1 - first, true);

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
        queue_update (Selection, playlist, first, last + 1 - first, true);

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
    int choice = 0;
    Entry * found = nullptr;

    for (auto & entry : playlist->entries)
    {
        if (! entry->shuffle_num)
            choice ++;
        else if (playlist->position &&
         entry->shuffle_num > playlist->position->shuffle_num &&
         (! found || entry->shuffle_num < found->shuffle_num))
            found = entry.get ();
    }

    if (found)
    {
        set_position (playlist, found, false);
        return true;
    }

    if (! choice)
        return false;

    choice = rand () % choice;

    for (auto & entry : playlist->entries)
    {
        if (! entry->shuffle_num)
        {
            if (! choice)
            {
                set_position (playlist, entry.get (), true);
                break;
            }

            choice --;
        }
    }

    return true;
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

        queue_update (Selection, playlist, playlist->position->number, 1, true);
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

bool playback_entry_read (int serial, String & error)
{
    ENTER;
    bool success = false;
    Entry * entry = get_playback_entry (serial);

    if (entry && send_playback_info (entry))
    {
        success = ! entry->failed;
        error = entry->error;
    }

    RETURN (success);
}

void playback_entry_set_tuple (int serial, Tuple && tuple)
{
    ENTER;
    PlaylistData * playlist = playing_playlist;
    if (! playlist || ! playlist->position || ! playback_check_serial (serial))
        RETURN ();

    Entry * entry = playlist->position;
    playlist->set_entry_tuple (entry, std::move (tuple));

    queue_update (Metadata, playlist, entry->number, 1);
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

static char parse_key[512];
static char * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = nullptr;

    if (! fgets (parse_key, sizeof parse_key, handle))
        return;

    char * space = strchr (parse_key, ' ');
    if (! space)
        return;

    * space = 0;
    parse_value = space + 1;

    char * newline = strchr (parse_value, '\n');
    if (newline)
        * newline = 0;
}

static bool parse_integer (const char * key, int * value)
{
    return (parse_value && ! strcmp (parse_key, key) && sscanf (parse_value, "%d", value) == 1);
}

static String parse_string (const char * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? String (parse_value) : String ();
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

    parse_next (handle);

    if (parse_integer ("active", & playlist_num))
    {
        if (! (active_playlist = lookup_playlist (playlist_num)))
            active_playlist = playlists[0].get ();
        parse_next (handle);
    }

    if (parse_integer ("playing", & resume_playlist))
        parse_next (handle);

    while (parse_integer ("playlist", & playlist_num) && playlist_num >= 0 &&
     playlist_num < playlists.len ())
    {
        PlaylistData * playlist = playlists[playlist_num].get ();
        int entries = playlist->entries.len ();

        parse_next (handle);

        playlist->filename = parse_string ("filename");
        if (playlist->filename)
            parse_next (handle);

        int position = -1;
        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            set_position (playlist, playlist->entries [position].get (), true);

        /* resume state is stored per-playlist for historical reasons */
        int resume_state = ResumePlay;
        if (parse_integer ("resume-state", & resume_state))
            parse_next (handle);

        if (playlist_num == resume_playlist)
        {
            if (resume_state == ResumeStop)
                resume_playlist = -1;
            if (resume_state == ResumePause)
                resume_paused = true;
        }

        if (parse_integer ("resume-time", & playlist->resume_time))
            parse_next (handle);
    }

    fclose (handle);

    /* clear updates queued during init sequence */

    for (auto & playlist : playlists)
    {
        playlist->next_update = Update ();
        playlist->last_update = Update ();
    }

    queued_update.stop ();
    update_level = NoUpdate;

    LEAVE;
}

EXPORT void aud_resume ()
{
    if (aud_get_bool (nullptr, "always_resume_paused"))
        resume_paused = true;

    aud_playlist_play (resume_playlist, resume_paused);
}
