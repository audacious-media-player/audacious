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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"
#include "drct.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "internal.h"
#include "mainloop.h"
#include "multihash.h"
#include "objects.h"
#include "plugins.h"
#include "scanner.h"
#include "tuple.h"
#include "tuple-compiler.h"

enum {RESUME_STOP, RESUME_PLAY, RESUME_PAUSE};

#define STATE_FILE "playlist-state"

#define ENTER pthread_mutex_lock (& mutex)
#define LEAVE pthread_mutex_unlock (& mutex)

#define RETURN(...) do { \
    pthread_mutex_unlock (& mutex); \
    return __VA_ARGS__; \
} while (0)

#define ENTER_GET_PLAYLIST(...) ENTER; \
    Playlist * playlist = lookup_playlist (playlist_num); \
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

struct Update {
    int level, before, after;
};

struct Entry {
    Entry (PlaylistAddItem && item);
    ~Entry ();

    int number;
    String filename;
    PluginHandle * decoder;
    Tuple tuple;
    String formatted, title, artist, album;
    int length;
    bool_t failed;
    bool_t selected;
    int shuffle_num;
    bool_t queued;
};

struct Playlist {
    Playlist (int id);
    ~Playlist ();

    int number, unique_id;
    String filename, title;
    bool_t modified;
    Index<SmartPtr<Entry>> entries;
    Entry * position, * focus;
    int selected_count;
    int last_shuffle_num;
    GList * queued;
    int64_t total_length, selected_length;
    bool_t scanning, scan_ending;
    Update next_update, last_update;
    bool_t resume_paused;
    int resume_time;
};

static const char * const default_title = N_("New Playlist");
static const char * const temp_title = N_("Now Playing");

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* The unique ID table contains pointers to Playlist for ID's in use and NULL
 * for "dead" (previously used and therefore unavailable) ID's. */
static SimpleHash<UniqueID, Playlist *> unique_id_table;
static int next_unique_id = 1000;

static Index<SmartPtr<Playlist>> playlists;
static Playlist * active_playlist = NULL;
static Playlist * playing_playlist = NULL;
static int resume_playlist = -1;

static QueuedFunc queued_update;
static int update_level;

struct ScanItem {
    Playlist * playlist;
    Entry * entry;
    ScanRequest * request;
};

static int scan_playlist, scan_row;
static GList * scan_list = NULL;

static void scan_finish (ScanRequest * request);
static void scan_cancel (Entry * entry);
static void scan_restart (void);

static bool_t next_song_locked (Playlist * playlist, bool_t repeat, int hint);

static void playlist_reformat_titles (void);
static void playlist_trigger_scan (void);

static SmartPtr<TupleCompiler> title_formatter;

static void entry_set_tuple_real (Entry * entry, Tuple && tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (entry->tuple.get_value_type (FIELD_SEGMENT_START) == TUPLE_INT)
        return;

    entry->tuple = std::move (tuple);
    entry->failed = FALSE;

    describe_song (entry->filename, entry->tuple, entry->title, entry->artist, entry->album);

    if (! entry->tuple)
    {
        entry->formatted = String ();
        entry->length = 0;
    }
    else
    {
        entry->formatted = title_formatter->evaluate (entry->tuple);
        entry->length = entry->tuple.get_int (FIELD_LENGTH);
        if (entry->length < 0)
            entry->length = 0;
    }
}

static void entry_set_tuple (Playlist * playlist, Entry * entry, Tuple && tuple)
{
    scan_cancel (entry);

    playlist->total_length -= entry->length;
    if (entry->selected)
        playlist->selected_length -= entry->length;

    entry_set_tuple_real (entry, std::move (tuple));

    playlist->total_length += entry->length;
    if (entry->selected)
        playlist->selected_length += entry->length;
}

static void entry_set_failed (Playlist * playlist, Entry * entry)
{
    Tuple tuple;
    tuple.set_filename (entry->filename);
    entry_set_tuple (playlist, entry, std::move (tuple));
    entry->failed = TRUE;
}

Entry::Entry (PlaylistAddItem && item) :
    number (-1),
    filename (item.filename),
    decoder (item.decoder),
    formatted (NULL),
    title (NULL),
    artist (NULL),
    album (NULL),
    length (0),
    failed (FALSE),
    selected (FALSE),
    shuffle_num (0),
    queued (FALSE)
{
    entry_set_tuple_real (this, std::move (item.tuple));
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

Playlist::Playlist (int id) :
    number (-1),
    unique_id (new_unique_id (id)),
    filename (NULL),
    title (_(default_title)),
    modified (TRUE),
    position (NULL),
    focus (NULL),
    selected_count (0),
    last_shuffle_num (0),
    queued (NULL),
    total_length (0),
    selected_length (0),
    scanning (FALSE),
    scan_ending (FALSE),
    next_update (),
    last_update (),
    resume_paused (FALSE),
    resume_time (0)
{
    unique_id_table.add (unique_id, (Playlist *) this);
}

Playlist::~Playlist ()
{
    unique_id_table.add (unique_id, nullptr);
    g_list_free (queued);
}

static void number_playlists (int at, int length)
{
    for (int i = at; i < at + length; i ++)
        playlists[i]->number = i;
}

static Playlist * lookup_playlist (int i)
{
    return (i >= 0 && i < playlists.len ()) ? playlists[i].get () : NULL;
}

static void number_entries (Playlist * p, int at, int length)
{
    for (int i = at; i < at + length; i ++)
        p->entries[i]->number = i;
}

static Entry * lookup_entry (Playlist * p, int i)
{
    return (i >= 0 && i < p->entries.len ()) ? p->entries[i].get () : NULL;
}

static void update (void * unused)
{
    ENTER;

    for (auto & p : playlists)
    {
        p->last_update = p->next_update;
        p->next_update = Update ();
    }

    int level = update_level;
    update_level = 0;

    LEAVE;

    hook_call ("playlist update", GINT_TO_POINTER (level));
}

static void queue_update (int level, Playlist * p, int at, int count)
{
    if (p)
    {
        if (level >= PLAYLIST_UPDATE_METADATA)
        {
            p->modified = TRUE;

            if (! aud_get_bool (NULL, "metadata_on_play"))
            {
                p->scanning = TRUE;
                p->scan_ending = FALSE;
                scan_restart ();
            }
        }

        if (p->next_update.level)
        {
            p->next_update.level = MAX (p->next_update.level, level);
            p->next_update.before = MIN (p->next_update.before, at);
            p->next_update.after = MIN (p->next_update.after, p->entries.len () - at - count);
        }
        else
        {
            p->next_update.level = level;
            p->next_update.before = at;
            p->next_update.after = p->entries.len () - at - count;
        }
    }

    if (! update_level)
        queued_update.queue (update, NULL);

    update_level = MAX (update_level, level);
}

EXPORT bool_t aud_playlist_update_pending (void)
{
    ENTER;
    bool_t pending = update_level ? TRUE : FALSE;
    RETURN (pending);
}

EXPORT int aud_playlist_updated_range (int playlist_num, int * at, int * count)
{
    ENTER_GET_PLAYLIST (0);

    Update * u = & playlist->last_update;

    int level = u->level;
    * at = u->before;
    * count = playlist->entries.len () - u->before - u->after;

    RETURN (level);
}

EXPORT bool_t aud_playlist_scan_in_progress (int playlist_num)
{
    if (playlist_num >= 0)
    {
        ENTER_GET_PLAYLIST (FALSE);
        bool_t scanning = playlist->scanning || playlist->scan_ending;
        RETURN (scanning);
    }
    else
    {
        ENTER;

        bool_t scanning = FALSE;
        for (auto & p : playlists)
        {
            if (p->scanning || p->scan_ending)
                scanning = TRUE;
        }

        RETURN (scanning);
    }
}

static GList * scan_list_find_playlist (Playlist * playlist)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = (ScanItem *) node->data;
        if (item->playlist == playlist)
            return node;
    }

    return NULL;
}

static GList * scan_list_find_entry (Entry * entry)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = (ScanItem *) node->data;
        if (item->entry == entry)
            return node;
    }

    return NULL;
}

static GList * scan_list_find_request (ScanRequest * request)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = (ScanItem *) node->data;
        if (item->request == request)
            return node;
    }

    return NULL;
}

static void scan_queue_entry (Playlist * playlist, Entry * entry)
{
    int flags = 0;
    if (! entry->tuple)
        flags |= SCAN_TUPLE;

    ScanItem * item = g_slice_new (ScanItem);
    item->playlist = playlist;
    item->entry = entry;
    item->request = scan_request (entry->filename, flags, entry->decoder, scan_finish);
    scan_list = g_list_prepend (scan_list, item);
}

static void scan_check_complete (Playlist * playlist)
{
    if (! playlist->scan_ending || scan_list_find_playlist (playlist))
        return;

    playlist->scan_ending = FALSE;
    event_queue_cancel ("playlist scan complete", NULL);
    event_queue ("playlist scan complete", NULL);
}

static bool_t scan_queue_next_entry (void)
{
    while (scan_playlist < playlists.len ())
    {
        Playlist * playlist = playlists[scan_playlist].get ();

        if (playlist->scanning)
        {
            while (scan_row < playlist->entries.len ())
            {
                Entry * entry = playlist->entries[scan_row ++].get ();

                if (! entry->tuple && ! scan_list_find_entry (entry))
                {
                    scan_queue_entry (playlist, entry);
                    return TRUE;
                }
            }

            playlist->scanning = FALSE;
            playlist->scan_ending = TRUE;
            scan_check_complete (playlist);
        }

        scan_playlist ++;
        scan_row = 0;
    }

    return FALSE;
}

static void scan_schedule (void)
{
    while (g_list_length (scan_list) < SCAN_THREADS)
    {
        if (! scan_queue_next_entry ())
            break;
    }
}

static void scan_finish (ScanRequest * request)
{
    ENTER;

    GList * node = scan_list_find_request (request);
    if (! node)
        RETURN ();

    ScanItem * item = (ScanItem *) node->data;
    Playlist * playlist = item->playlist;
    Entry * entry = item->entry;

    scan_list = g_list_delete_link (scan_list, node);
    g_slice_free (ScanItem, item);

    if (! entry->decoder)
        entry->decoder = scan_request_get_decoder (request);

    if (! entry->tuple)
    {
        Tuple tuple = scan_request_get_tuple (request);
        if (tuple)
        {
            entry_set_tuple (playlist, entry, std::move (tuple));
            queue_update (PLAYLIST_UPDATE_METADATA, playlist, entry->number, 1);
        }
    }

    if (! entry->decoder || ! entry->tuple)
        entry_set_failed (playlist, entry);

    scan_check_complete (playlist);
    scan_schedule ();

    pthread_cond_broadcast (& cond);

    LEAVE;
}

static void scan_cancel (Entry * entry)
{
    GList * node = scan_list_find_entry (entry);
    if (! node)
        return;

    ScanItem * item = (ScanItem *) node->data;
    scan_list = g_list_delete_link (scan_list, node);
    g_slice_free (ScanItem, item);
}

static void scan_restart (void)
{
    scan_playlist = 0;
    scan_row = 0;
    scan_schedule ();
}

/* mutex may be unlocked during the call */
static Entry * get_entry (int playlist_num, int entry_num,
 bool_t need_decoder, bool_t need_tuple)
{
    while (1)
    {
        Playlist * playlist = lookup_playlist (playlist_num);
        Entry * entry = playlist ? lookup_entry (playlist, entry_num) : NULL;

        if (! entry || entry->failed)
            return entry;

        if ((need_decoder && ! entry->decoder) || (need_tuple && ! entry->tuple))
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
static Entry * get_playback_entry (bool_t need_decoder, bool_t need_tuple)
{
    while (1)
    {
        Entry * entry = playing_playlist ? playing_playlist->position : NULL;

        if (! entry || entry->failed)
            return entry;

        if ((need_decoder && ! entry->decoder) || (need_tuple && ! entry->tuple))
        {
            if (! scan_list_find_entry (entry))
                scan_queue_entry (playing_playlist, entry);

            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        return entry;
    }
}

void playlist_init (void)
{
    srand (time (NULL));

    ENTER;

    update_level = 0;
    scan_playlist = scan_row = 0;

    title_formatter = new TupleCompiler;

    LEAVE;

    /* initialize title formatter */
    playlist_reformat_titles ();

    hook_associate ("set metadata_on_play", (HookFunction) playlist_trigger_scan, NULL);
    hook_associate ("set generic_title_format", (HookFunction) playlist_reformat_titles, NULL);
    hook_associate ("set show_numbers_in_pl", (HookFunction) playlist_reformat_titles, NULL);
    hook_associate ("set leading_zero", (HookFunction) playlist_reformat_titles, NULL);
}

void playlist_end (void)
{
    hook_dissociate ("set metadata_on_play", (HookFunction) playlist_trigger_scan);
    hook_dissociate ("set generic_title_format", (HookFunction) playlist_reformat_titles);
    hook_dissociate ("set show_numbers_in_pl", (HookFunction) playlist_reformat_titles);
    hook_dissociate ("set leading_zero", (HookFunction) playlist_reformat_titles);

    ENTER;

    queued_update.stop ();

    active_playlist = playing_playlist = NULL;
    resume_playlist = -1;

    playlists.clear ();
    unique_id_table.clear ();

    title_formatter = nullptr;

    LEAVE;
}

EXPORT int aud_playlist_count (void)
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

    playlists.insert (at, 1);
    playlists[at] = SmartPtr<Playlist> (new Playlist (id));

    number_playlists (at, playlists.len () - at);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, NULL, 0, 0);
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

    Index<SmartPtr<Playlist>> displaced;

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

    queue_update (PLAYLIST_UPDATE_STRUCTURE, NULL, 0, 0);
    LEAVE;
}

EXPORT void aud_playlist_delete (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    bool_t was_playing = (playlist == playing_playlist);

    playlists.remove (playlist_num, 1);

    if (! playlists.len ())
        playlists.append (SmartPtr<Playlist> (new Playlist (-1)));

    number_playlists (playlist_num, playlists.len () - playlist_num);

    if (playlist == active_playlist)
    {
        int active_num = MIN (playlist_num, playlists.len () - 1);
        active_playlist = playlists[active_num].get ();
    }

    if (playlist == playing_playlist)
        playing_playlist = NULL;

    queue_update (PLAYLIST_UPDATE_STRUCTURE, NULL, 0, 0);
    LEAVE;

    if (was_playing)
        playback_stop ();
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

    Playlist * * ptr = unique_id_table.lookup (id);
    int num = (ptr && * ptr) ? (* ptr)->number : -1;

    RETURN (num);
}

EXPORT void aud_playlist_set_filename (int playlist_num, const char * filename)
{
    ENTER_GET_PLAYLIST ();

    playlist->filename = String (filename);
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, NULL, 0, 0);
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
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, NULL, 0, 0);
    LEAVE;
}

EXPORT String aud_playlist_get_title (int playlist_num)
{
    ENTER_GET_PLAYLIST (String ());
    String title = playlist->title;
    RETURN (title);
}

void playlist_set_modified (int playlist_num, bool_t modified)
{
    ENTER_GET_PLAYLIST ();
    playlist->modified = modified;
    LEAVE;
}

bool_t playlist_get_modified (int playlist_num)
{
    ENTER_GET_PLAYLIST (FALSE);
    bool_t modified = playlist->modified;
    RETURN (modified);
}

EXPORT void aud_playlist_set_active (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    bool_t changed = FALSE;

    if (playlist != active_playlist)
    {
        changed = TRUE;
        active_playlist = playlist;
    }

    LEAVE;

    if (changed)
        hook_call ("playlist activate", NULL);
}

EXPORT int aud_playlist_get_active (void)
{
    ENTER;
    int list = active_playlist ? active_playlist->number : -1;
    RETURN (list);
}

EXPORT void aud_playlist_set_playing (int playlist_num)
{
    /* get playback state before locking playlists */
    bool_t paused = aud_drct_get_paused ();
    int time = aud_drct_get_time ();

    ENTER;

    Playlist * playlist = lookup_playlist (playlist_num);
    bool_t can_play = FALSE;
    bool_t position_changed = FALSE;

    if (playlist == playing_playlist)
        RETURN ();

    if (playing_playlist)
    {
        playing_playlist->resume_paused = paused;
        playing_playlist->resume_time = time;
    }

    /* is there anything to play? */
    if (playlist && ! playlist->position)
    {
        if (next_song_locked (playlist, TRUE, 0))
            position_changed = TRUE;
        else
            playlist = NULL;
    }

    if (playlist)
    {
        can_play = TRUE;
        paused = playlist->resume_paused;
        time = playlist->resume_time;
    }

    playing_playlist = playlist;

    LEAVE;

    if (position_changed)
        hook_call ("playlist position", GINT_TO_POINTER (playlist_num));

    hook_call ("playlist set playing", NULL);

    /* start playback after unlocking playlists */
    if (can_play)
        playback_play (time, paused);
    else
        playback_stop ();
}

EXPORT int aud_playlist_get_playing (void)
{
    ENTER;
    int list = playing_playlist ? playing_playlist->number: -1;
    RETURN (list);
}

EXPORT int aud_playlist_get_blank (void)
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

EXPORT int aud_playlist_get_temporary (void)
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

static void set_position (Playlist * playlist, Entry * entry, bool_t update_shuffle)
{
    playlist->position = entry;
    playlist->resume_time = 0;

    /* move entry to top of shuffle list */
    if (entry && update_shuffle)
        entry->shuffle_num = ++ playlist->last_shuffle_num;
}

/* unlocked */
static void change_playback (bool_t can_play)
{
    if (can_play && aud_drct_get_playing ())
        playback_play (0, aud_drct_get_paused ());
    else
        aud_playlist_set_playing (-1);
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
        playlist->entries[i ++] = SmartPtr<Entry> (entry);
        playlist->total_length += entry->length;
    }

    items.clear ();

    number_entries (playlist, at, entries + number - at);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, at, number);
    LEAVE;
}

EXPORT void aud_playlist_entry_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    bool_t position_changed = FALSE;
    bool_t was_playing = FALSE;
    bool_t can_play = FALSE;

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    if (playlist->position && playlist->position->number >= at &&
     playlist->position->number < at + number)
    {
        position_changed = TRUE;
        was_playing = (playlist == playing_playlist);

        set_position (playlist, NULL, FALSE);
    }

    if (playlist->focus && playlist->focus->number >= at &&
     playlist->focus->number < at + number)
    {
        if (at + number < entries)
            playlist->focus = playlist->entries[at + number].get ();
        else if (at > 0)
            playlist->focus = playlist->entries[at - 1].get ();
        else
            playlist->focus = NULL;
    }

    for (int count = 0; count < number; count ++)
    {
        Entry * entry = playlist->entries [at + count].get ();

        if (entry->queued)
            playlist->queued = g_list_remove (playlist->queued, entry);

        if (entry->selected)
        {
            playlist->selected_count --;
            playlist->selected_length -= entry->length;
        }

        playlist->total_length -= entry->length;
    }

    playlist->entries.remove (at, number);
    number_entries (playlist, at, entries - at - number);

    if (position_changed && aud_get_bool (NULL, "advance_on_delete"))
        can_play = next_song_locked (playlist, aud_get_bool (NULL, "repeat"), at);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, at, 0);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (can_play);
}

EXPORT String aud_playlist_entry_get_filename (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (String ());
    String filename = entry->filename;
    RETURN (filename);
}

EXPORT PluginHandle * aud_playlist_entry_get_decoder (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, ! fast, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    RETURN (decoder);
}

EXPORT Tuple aud_playlist_entry_get_tuple (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    Tuple tuple = entry->tuple.ref ();

    RETURN (tuple);
}

EXPORT String aud_playlist_entry_get_title (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    String title = entry ? (entry->formatted ? entry->formatted : entry->title) : String ();

    RETURN (title);
}

EXPORT void aud_playlist_entry_describe (int playlist_num, int entry_num,
 String & title, String & artist, String & album, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);

    title = entry ? entry->title : String ();
    artist = entry ? entry->artist : String ();
    album = entry ? entry->album : String ();

    LEAVE;
}

EXPORT int aud_playlist_entry_get_length (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    int length = entry ? entry->length : 0;

    RETURN (length);
}

EXPORT void aud_playlist_set_position (int playlist_num, int entry_num)
{
    ENTER_GET_PLAYLIST ();

    Entry * entry = lookup_entry (playlist, entry_num);
    bool_t was_playing = (playlist == playing_playlist);
    bool_t can_play = !! entry;

    set_position (playlist, entry, TRUE);

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (can_play);
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
        first = MIN (first, playlist->focus->number);
        last = MAX (last, playlist->focus->number);
    }

    playlist->focus = lookup_entry (playlist, entry_num);

    if (playlist->focus)
    {
        first = MIN (first, playlist->focus->number);
        last = MAX (last, playlist->focus->number);
    }

    if (first <= last)
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist, first, last + 1 - first);

    LEAVE;
}

EXPORT int aud_playlist_get_focus (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int focus = playlist->focus ? playlist->focus->number : -1;
    RETURN (focus);
}

EXPORT void aud_playlist_entry_set_selected (int playlist_num, int entry_num,
 bool_t selected)
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

    queue_update (PLAYLIST_UPDATE_SELECTION, playlist, entry_num, 1);
    LEAVE;
}

EXPORT bool_t aud_playlist_entry_get_selected (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (FALSE);
    bool_t selected = entry->selected;
    RETURN (selected);
}

EXPORT int aud_playlist_selected_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int selected_count = playlist->selected_count;
    RETURN (selected_count);
}

EXPORT void aud_playlist_select_all (int playlist_num, bool_t selected)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (auto & entry : playlist->entries)
    {
        if ((selected && ! entry->selected) || (entry->selected && ! selected))
        {
            entry->selected = selected;
            first = MIN (first, entry->number);
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
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist, first, last + 1 - first);

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
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, top, bottom - top);

    RETURN (shift);
}

static Entry * find_unselected_focus (Playlist * playlist)
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

    return NULL;
}

EXPORT void aud_playlist_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    if (! playlist->selected_count)
        RETURN ();

    int entries = playlist->entries.len ();
    bool_t position_changed = FALSE;
    bool_t was_playing = FALSE;
    bool_t can_play = FALSE;

    if (playlist->position && playlist->position->selected)
    {
        position_changed = TRUE;
        was_playing = (playlist == playing_playlist);
        set_position (playlist, NULL, FALSE);
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
                playlist->queued = g_list_remove (playlist->queued, entry);

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

    if (position_changed && aud_get_bool (NULL, "advance_on_delete"))
        can_play = next_song_locked (playlist, aud_get_bool (NULL, "repeat"), entries - after);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, before, entries - after - before);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (can_play);
}

EXPORT void aud_playlist_reverse (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries / 2; i ++)
        playlist->entries[i].swap (playlist->entries[entries - 1 - i]);

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, entries);
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

        playlist->entries[top ++].swap (playlist->entries[bottom --]);
    }

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, entries);
    LEAVE;
}

EXPORT void aud_playlist_randomize (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();

    for (int i = 0; i < entries; i ++)
        playlist->entries[i].swap (playlist->entries[rand () % entries]);

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, entries);
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
        playlist->entries[a].swap (playlist->entries[b]);
    }

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, entries);
    LEAVE;
}

enum {COMPARE_TYPE_FILENAME, COMPARE_TYPE_TUPLE, COMPARE_TYPE_TITLE};

struct CompareData {
    PlaylistStringCompareFunc filename_compare;
    PlaylistTupleCompareFunc tuple_compare;
    PlaylistStringCompareFunc title_compare;
};

static int compare_cb (const SmartPtr<Entry> & a, const SmartPtr<Entry> & b, void * _data)
{
    CompareData * data = (CompareData *) _data;

    int diff = 0;

    if (data->filename_compare)
        diff = data->filename_compare (a->filename, b->filename);
    else if (data->tuple_compare)
        diff = data->tuple_compare (a->tuple, b->tuple);
    else if (data->title_compare)
        diff = data->title_compare (a->formatted ? a->formatted : a->filename,
         b->formatted ? b->formatted : b->filename);

    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static void sort (Playlist * playlist, CompareData * data)
{
    playlist->entries.sort (compare_cb, data);
    number_entries (playlist, 0, playlist->entries.len ());

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, playlist->entries.len ());
}

static void sort_selected (Playlist * playlist, CompareData * data)
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
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist, 0, entries);
}

static bool_t entries_are_scanned (Playlist * playlist, bool_t selected)
{
    for (auto & entry : playlist->entries)
    {
        if (selected && ! entry->selected)
            continue;

        if (! entry->tuple)
        {
            aud_ui_show_error (_("The playlist cannot be sorted because "
             "metadata scanning is still in progress (or has been disabled)."));
            return FALSE;
        }
    }

    return TRUE;
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
    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, & data);

    LEAVE;
}

EXPORT void aud_playlist_sort_by_title (int playlist_num, PlaylistStringCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, nullptr, compare};
    if (entries_are_scanned (playlist, FALSE))
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
    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, & data);

    LEAVE;
}

EXPORT void aud_playlist_sort_selected_by_title (int playlist_num,
 PlaylistStringCompareFunc compare)
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {nullptr, nullptr, compare};
    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, & data);

    LEAVE;
}

static void playlist_reformat_titles (void)
{
    ENTER;

    String format = aud_get_str (NULL, "generic_title_format");
    title_formatter->compile (format);

    for (auto & playlist : playlists)
    {
        for (auto & entry : playlist->entries)
        {
            if (entry->tuple)
                entry->formatted = title_formatter->evaluate (entry->tuple);
            else
                entry->formatted = String ();
        }

        queue_update (PLAYLIST_UPDATE_METADATA, playlist.get (), 0, playlist->entries.len ());
    }

    LEAVE;
}

static void playlist_trigger_scan (void)
{
    ENTER;

    for (auto & playlist : playlists)
        playlist->scanning = TRUE;

    scan_restart ();

    LEAVE;
}

static void playlist_rescan_real (int playlist_num, bool_t selected)
{
    ENTER_GET_PLAYLIST ();

    for (auto & entry : playlist->entries)
    {
        if (! selected || entry->selected)
            entry_set_tuple (playlist, entry.get (), Tuple ());
    }

    queue_update (PLAYLIST_UPDATE_METADATA, playlist, 0, playlist->entries.len ());
    LEAVE;
}

EXPORT void aud_playlist_rescan (int playlist_num)
{
    playlist_rescan_real (playlist_num, FALSE);
}

EXPORT void aud_playlist_rescan_selected (int playlist_num)
{
    playlist_rescan_real (playlist_num, TRUE);
}

EXPORT void aud_playlist_rescan_file (const char * filename)
{
    ENTER;

    for (auto & playlist : playlists)
    {
        for (auto & entry : playlist->entries)
        {
            if (! strcmp (entry->filename, filename))
            {
                entry_set_tuple (playlist.get (), entry.get (), Tuple ());
                queue_update (PLAYLIST_UPDATE_METADATA, playlist.get (), entry->number, 1);
            }
        }
    }

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
    int count = g_list_length (playlist->queued);
    RETURN (count);
}

EXPORT void aud_playlist_queue_insert (int playlist_num, int at, int entry_num)
{
    ENTER_GET_ENTRY ();

    if (entry->queued)
        RETURN ();

    if (at < 0)
        playlist->queued = g_list_append (playlist->queued, entry);
    else
        playlist->queued = g_list_insert (playlist->queued, entry, at);

    entry->queued = TRUE;

    queue_update (PLAYLIST_UPDATE_SELECTION, playlist, entry_num, 1);
    LEAVE;
}

EXPORT void aud_playlist_queue_insert_selected (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST ();

    int first = playlist->entries.len ();
    int last = 0;

    for (auto & entry : playlist->entries)
    {
        if (! entry->selected || entry->queued)
            continue;

        if (at < 0)
            playlist->queued = g_list_append (playlist->queued, entry.get ());
        else
            playlist->queued = g_list_insert (playlist->queued, entry.get (), at ++);

        entry->queued = TRUE;
        first = MIN (first, entry->number);
        last = entry->number;
    }

    if (first < playlist->entries.len ())
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist, first, last + 1 - first);

    LEAVE;
}

EXPORT int aud_playlist_queue_get_entry (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST (-1);

    GList * node = g_list_nth (playlist->queued, at);
    int entry_num = node ? ((Entry *) node->data)->number : -1;

    RETURN (entry_num);
}

EXPORT int aud_playlist_queue_find_entry (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (-1);
    int pos = entry->queued ? g_list_index (playlist->queued, entry) : -1;
    RETURN (pos);
}

EXPORT void aud_playlist_queue_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    if (at == 0)
    {
        while (playlist->queued && number --)
        {
            Entry * entry = (Entry *) playlist->queued->data;
            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;

            playlist->queued = g_list_delete_link (playlist->queued, playlist->queued);
        }
    }
    else
    {
        GList * anchor = g_list_nth (playlist->queued, at - 1);
        if (! anchor)
            goto DONE;

        while (anchor->next && number --)
        {
            Entry * entry = (Entry *) anchor->next->data;
            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;

            playlist->queued = g_list_delete_link (playlist->queued, anchor->next);
        }
    }

DONE:
    if (first < entries)
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist, first, last + 1 - first);

    LEAVE;
}

EXPORT void aud_playlist_queue_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = playlist->entries.len ();
    int first = entries, last = 0;

    for (GList * node = playlist->queued; node; )
    {
        GList * next = node->next;
        Entry * entry = (Entry *) node->data;

        if (entry->selected)
        {
            entry->queued = FALSE;
            playlist->queued = g_list_delete_link (playlist->queued, node);
            first = MIN (first, entry->number);
            last = entry->number;
        }

        node = next;
    }

    if (first < entries)
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist, first, last + 1 - first);

    LEAVE;
}

static bool_t shuffle_prev (Playlist * playlist)
{
    Entry * found = NULL;

    for (auto & entry : playlist->entries)
    {
        if (entry->shuffle_num && (! playlist->position ||
         entry->shuffle_num < playlist->position->shuffle_num) && (! found
         || entry->shuffle_num > found->shuffle_num))
            found = entry.get ();
    }

    if (! found)
        return FALSE;

    set_position (playlist, found, FALSE);
    return TRUE;
}

bool_t playlist_prev_song (int playlist_num)
{
    ENTER_GET_PLAYLIST (FALSE);

    bool_t was_playing = (playlist == playing_playlist);

    if (aud_get_bool (NULL, "shuffle"))
    {
        if (! shuffle_prev (playlist))
            RETURN (FALSE);
    }
    else
    {
        if (! playlist->position || playlist->position->number == 0)
            RETURN (FALSE);

        set_position (playlist, playlist->entries[playlist->position->number - 1].get (), TRUE);
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (TRUE);

    return TRUE;
}

static bool_t shuffle_next (Playlist * playlist)
{
    int choice = 0;
    Entry * found = NULL;

    for (auto & entry : playlist->entries)
    {
        if (! entry->shuffle_num)
            choice ++;
        else if (playlist->position && entry->shuffle_num >
         playlist->position->shuffle_num && (! found || entry->shuffle_num
         < found->shuffle_num))
            found = entry.get ();
    }

    if (found)
    {
        set_position (playlist, found, FALSE);
        return TRUE;
    }

    if (! choice)
        return FALSE;

    choice = rand () % choice;

    for (auto & entry : playlist->entries)
    {
        if (! entry->shuffle_num)
        {
            if (! choice)
            {
                set_position (playlist, entry.get (), TRUE);
                break;
            }

            choice --;
        }
    }

    return TRUE;
}

static void shuffle_reset (Playlist * playlist)
{
    playlist->last_shuffle_num = 0;

    for (auto & entry : playlist->entries)
        entry->shuffle_num = 0;
}

static bool_t next_song_locked (Playlist * playlist, bool_t repeat, int hint)
{
    int entries = playlist->entries.len ();
    if (! entries)
        return FALSE;

    if (playlist->queued)
    {
        set_position (playlist, (Entry *) playlist->queued->data, TRUE);
        playlist->queued = g_list_remove (playlist->queued, playlist->position);
        playlist->position->queued = FALSE;
    }
    else if (aud_get_bool (NULL, "shuffle"))
    {
        if (! shuffle_next (playlist))
        {
            if (! repeat)
                return FALSE;

            shuffle_reset (playlist);

            if (! shuffle_next (playlist))
                return FALSE;
        }
    }
    else
    {
        if (hint >= entries)
        {
            if (! repeat)
                return FALSE;

            hint = 0;
        }

        set_position (playlist, playlist->entries[hint].get (), TRUE);
    }

    return TRUE;
}

bool_t playlist_next_song (int playlist_num, bool_t repeat)
{
    ENTER_GET_PLAYLIST (FALSE);

    int hint = playlist->position ? playlist->position->number + 1 : 0;
    bool_t was_playing = (playlist == playing_playlist);

    if (! next_song_locked (playlist, repeat, hint))
        RETURN (FALSE);

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (TRUE);

    return TRUE;
}

int playback_entry_get_position (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, FALSE);
    int entry_num = entry ? entry->number : -1;

    RETURN (entry_num);
}

String playback_entry_get_filename (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, FALSE);
    String filename = entry ? entry->filename : String ();

    RETURN (filename);
}

PluginHandle * playback_entry_get_decoder (void)
{
    ENTER;

    Entry * entry = get_playback_entry (TRUE, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    RETURN (decoder);
}

Tuple playback_entry_get_tuple (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    Tuple tuple = entry->tuple.ref ();

    RETURN (tuple);
}

String playback_entry_get_title (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    String title = entry ? (entry->formatted ? entry->formatted : entry->title) : String ();

    RETURN (title);
}

int playback_entry_get_length (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    int length = entry ? entry->length : 0;

    RETURN (length);
}

void playback_entry_set_tuple (Tuple && tuple)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        RETURN ();

    Entry * entry = playing_playlist->position;
    entry_set_tuple (playing_playlist, entry, std::move (tuple));

    queue_update (PLAYLIST_UPDATE_METADATA, playing_playlist, entry->number, 1);
    LEAVE;
}

void playlist_save_state (void)
{
    /* get playback state before locking playlists */
    bool_t paused = aud_drct_get_paused ();
    int time = aud_drct_get_time ();

    ENTER;

    const char * user_dir = aud_get_path (AUD_PATH_USER_DIR);
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

        if (playlist.get () == playing_playlist)
        {
            playlist->resume_paused = paused;
            playlist->resume_time = time;
        }

        fprintf (handle, "resume-state %d\n", paused ? RESUME_PAUSE : RESUME_PLAY);
        fprintf (handle, "resume-time %d\n", playlist->resume_time);
    }

    fclose (handle);
    LEAVE;
}

static char parse_key[512];
static char * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = NULL;

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

static bool_t parse_integer (const char * key, int * value)
{
    return (parse_value && ! strcmp (parse_key, key) && sscanf (parse_value, "%d", value) == 1);
}

static String parse_string (const char * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? String (parse_value) : String ();
}

void playlist_load_state (void)
{
    ENTER;
    int playlist_num;

    const char * user_dir = aud_get_path (AUD_PATH_USER_DIR);
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
        Playlist * playlist = playlists[playlist_num].get ();
        int entries = playlist->entries.len ();

        parse_next (handle);

        playlist->filename = parse_string ("filename");
        if (playlist->filename)
            parse_next (handle);

        int position = -1;
        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            set_position (playlist, playlist->entries [position].get (), TRUE);

        int resume_state = RESUME_PLAY;
        if (parse_integer ("resume-state", & resume_state))
            parse_next (handle);

        playlist->resume_paused = (resume_state == RESUME_PAUSE);

        if (parse_integer ("resume-time", & playlist->resume_time))
            parse_next (handle);

        /* compatibility with Audacious 3.3 */
        if (playlist_num == resume_playlist && resume_state == RESUME_STOP)
            resume_playlist = -1;
    }

    fclose (handle);

    /* clear updates queued during init sequence */

    for (auto & playlist : playlists)
    {
        playlist->next_update = Update ();
        playlist->last_update = Update ();
    }

    queued_update.stop ();
    update_level = 0;

    LEAVE;
}

EXPORT void aud_resume (void)
{
    aud_playlist_set_playing (resume_playlist);
}
