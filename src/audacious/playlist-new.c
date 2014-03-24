/*
 * playlist-new.c
 * Copyright 2009-2013 John Lindgren
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/tuple.h>

#include "drct.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "plugins.h"
#include "scanner.h"
#include "util.h"

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

typedef struct {
    int level, before, after;
} Update;

typedef struct {
    int number;
    char * filename;
    PluginHandle * decoder;
    Tuple * tuple;
    char * formatted, * title, * artist, * album;
    int length;
    bool_t failed;
    bool_t selected;
    int shuffle_num;
    bool_t queued;
} Entry;

typedef struct {
    int number, unique_id;
    char * filename, * title;
    bool_t modified;
    Index * entries;
    Entry * position, * focus;
    int selected_count;
    int last_shuffle_num;
    GList * queued;
    int64_t total_length, selected_length;
    bool_t scanning, scan_ending;
    Update next_update, last_update;
    bool_t resume_paused;
    int resume_time;
} Playlist;

static const char * const default_title = N_("New Playlist");
static const char * const temp_title = N_("Now Playing");

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* The unique ID table contains pointers to Playlist for ID's in use and NULL
 * for "dead" (previously used and therefore unavailable) ID's. */
static GHashTable * unique_id_table = NULL;
static int next_unique_id = 1000;

static Index * playlists = NULL;
static Playlist * active_playlist = NULL;
static Playlist * playing_playlist = NULL;
static int resume_playlist = -1;

static int update_source = 0, update_level;

typedef struct {
    Playlist * playlist;
    Entry * entry;
    ScanRequest * request;
} ScanItem;

static int scan_playlist, scan_row;
static GList * scan_list = NULL;

static void scan_finish (ScanRequest * request);
static void scan_cancel (Entry * entry);
static void scan_restart (void);

static bool_t next_song_locked (Playlist * playlist, bool_t repeat, int hint);

static TupleFormatter * title_formatter;

static void entry_set_tuple_real (Entry * entry, Tuple * tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (entry->tuple && tuple_get_value_type (entry->tuple, FIELD_SEGMENT_START) == TUPLE_INT)
    {
        if (tuple)
            tuple_unref (tuple);
        return;
    }

    if (entry->tuple)
        tuple_unref (entry->tuple);

    entry->tuple = tuple;
    entry->failed = FALSE;

    str_unref (entry->formatted);
    str_unref (entry->title);
    str_unref (entry->artist);
    str_unref (entry->album);

    describe_song (entry->filename, tuple, & entry->title, & entry->artist, & entry->album);

    if (! tuple)
    {
        entry->formatted = NULL;
        entry->length = 0;
    }
    else
    {
        entry->formatted = tuple_format_title (title_formatter, tuple);
        entry->length = tuple_get_int (tuple, FIELD_LENGTH);
        if (entry->length < 0)
            entry->length = 0;
    }
}

static void entry_set_tuple (Playlist * playlist, Entry * entry, Tuple * tuple)
{
    scan_cancel (entry);

    if (entry->tuple)
    {
        playlist->total_length -= entry->length;
        if (entry->selected)
            playlist->selected_length -= entry->length;
    }

    entry_set_tuple_real (entry, tuple);

    if (tuple)
    {
        playlist->total_length += entry->length;
        if (entry->selected)
            playlist->selected_length += entry->length;
    }
}

static void entry_set_failed (Playlist * playlist, Entry * entry)
{
    entry_set_tuple (playlist, entry, tuple_new_from_filename (entry->filename));
    entry->failed = TRUE;
}

static Entry * entry_new (char * filename, Tuple * tuple, PluginHandle * decoder)
{
    Entry * entry = g_slice_new (Entry);

    entry->filename = filename;
    entry->decoder = decoder;
    entry->tuple = NULL;
    entry->formatted = NULL;
    entry->title = NULL;
    entry->artist = NULL;
    entry->album = NULL;
    entry->failed = FALSE;
    entry->number = -1;
    entry->selected = FALSE;
    entry->shuffle_num = 0;
    entry->queued = FALSE;

    entry_set_tuple_real (entry, tuple);
    return entry;
}

static void entry_free (Entry * entry)
{
    scan_cancel (entry);

    str_unref (entry->filename);
    if (entry->tuple)
        tuple_unref (entry->tuple);

    str_unref (entry->formatted);
    str_unref (entry->title);
    str_unref (entry->artist);
    str_unref (entry->album);
    g_slice_free (Entry, entry);
}

static int new_unique_id (int preferred)
{
    if (preferred >= 0 && ! g_hash_table_lookup_extended (unique_id_table,
     GINT_TO_POINTER (preferred), NULL, NULL))
        return preferred;

    while (g_hash_table_lookup_extended (unique_id_table,
     GINT_TO_POINTER (next_unique_id), NULL, NULL))
        next_unique_id ++;

    return next_unique_id ++;
}

static Playlist * playlist_new (int id)
{
    Playlist * playlist = g_slice_new (Playlist);

    playlist->number = -1;
    playlist->unique_id = new_unique_id (id);
    playlist->filename = NULL;
    playlist->title = str_get (_(default_title));
    playlist->modified = TRUE;
    playlist->entries = index_new();
    playlist->position = NULL;
    playlist->focus = NULL;
    playlist->selected_count = 0;
    playlist->last_shuffle_num = 0;
    playlist->queued = NULL;
    playlist->total_length = 0;
    playlist->selected_length = 0;
    playlist->scanning = FALSE;
    playlist->scan_ending = FALSE;
    playlist->resume_paused = FALSE;
    playlist->resume_time = 0;

    memset (& playlist->last_update, 0, sizeof (Update));
    memset (& playlist->next_update, 0, sizeof (Update));

    g_hash_table_insert (unique_id_table, GINT_TO_POINTER (playlist->unique_id), playlist);
    return playlist;
}

static void playlist_free (Playlist * playlist)
{
    g_hash_table_insert (unique_id_table, GINT_TO_POINTER (playlist->unique_id), NULL);

    str_unref (playlist->filename);
    str_unref (playlist->title);
    index_free_full (playlist->entries, (IndexFreeFunc) entry_free);
    g_list_free (playlist->queued);
    g_slice_free (Playlist, playlist);
}

static void number_playlists (int at, int length)
{
    for (int count = 0; count < length; count ++)
    {
        Playlist * playlist = index_get (playlists, at + count);
        playlist->number = at + count;
    }
}

static Playlist * lookup_playlist (int playlist_num)
{
    return (playlists && playlist_num >= 0 && playlist_num < index_count
     (playlists)) ? index_get (playlists, playlist_num) : NULL;
}

static void number_entries (Playlist * playlist, int at, int length)
{
    for (int count = 0; count < length; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        entry->number = at + count;
    }
}

static Entry * lookup_entry (Playlist * playlist, int entry_num)
{
    return (entry_num >= 0 && entry_num < index_count (playlist->entries)) ?
     index_get (playlist->entries, entry_num) : NULL;
}

static bool_t update (void * unused)
{
    ENTER;

    for (int i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        memcpy (& p->last_update, & p->next_update, sizeof (Update));
        memset (& p->next_update, 0, sizeof (Update));
    }

    int level = update_level;
    update_level = 0;

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    LEAVE;

    hook_call ("playlist update", GINT_TO_POINTER (level));
    return FALSE;
}

static void queue_update (int level, int list, int at, int count)
{
    Playlist * p = lookup_playlist (list);

    if (p)
    {
        if (level >= PLAYLIST_UPDATE_METADATA)
        {
            p->modified = TRUE;

            if (! get_bool (NULL, "metadata_on_play"))
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
            p->next_update.after = MIN (p->next_update.after,
             index_count (p->entries) - at - count);
        }
        else
        {
            p->next_update.level = level;
            p->next_update.before = at;
            p->next_update.after = index_count (p->entries) - at - count;
        }
    }

    update_level = MAX (update_level, level);

    if (! update_source)
        update_source = g_idle_add_full (G_PRIORITY_HIGH, update, NULL, NULL);
}

bool_t playlist_update_pending (void)
{
    ENTER;
    bool_t pending = update_level ? TRUE : FALSE;
    RETURN (pending);
}

int playlist_updated_range (int playlist_num, int * at, int * count)
{
    ENTER_GET_PLAYLIST (0);

    Update * u = & playlist->last_update;

    int level = u->level;
    * at = u->before;
    * count = index_count (playlist->entries) - u->before - u->after;

    RETURN (level);
}

bool_t playlist_scan_in_progress (int playlist_num)
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
        for (playlist_num = 0; playlist_num < index_count (playlists); playlist_num ++)
        {
            Playlist * playlist = index_get (playlists, playlist_num);
            if (playlist->scanning || playlist->scan_ending)
                scanning = TRUE;
        }

        RETURN (scanning);
    }
}

static GList * scan_list_find_playlist (Playlist * playlist)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = node->data;
        if (item->playlist == playlist)
            return node;
    }

    return NULL;
}

static GList * scan_list_find_entry (Entry * entry)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = node->data;
        if (item->entry == entry)
            return node;
    }

    return NULL;
}

static GList * scan_list_find_request (ScanRequest * request)
{
    for (GList * node = scan_list; node; node = node->next)
    {
        ScanItem * item = node->data;
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
    while (scan_playlist < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, scan_playlist);

        if (playlist->scanning)
        {
            while (scan_row < index_count (playlist->entries))
            {
                Entry * entry = index_get (playlist->entries, scan_row ++);

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

    ScanItem * item = node->data;
    Playlist * playlist = item->playlist;
    Entry * entry = item->entry;
    bool_t changed = FALSE;

    scan_list = g_list_delete_link (scan_list, node);
    g_slice_free (ScanItem, item);

    if (! entry->decoder)
        entry->decoder = scan_request_get_decoder (request);

    if (! entry->tuple)
    {
        Tuple * tuple = scan_request_get_tuple (request);
        if (tuple)
        {
            entry_set_tuple (playlist, entry, tuple);
            changed = TRUE;
        }
    }

    if (! entry->decoder || ! entry->tuple)
        entry_set_failed (playlist, entry);

    if (changed)
        queue_update (PLAYLIST_UPDATE_METADATA, playlist->number, entry->number, 1);

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

    ScanItem * item = node->data;
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

    unique_id_table = g_hash_table_new (g_direct_hash, g_direct_equal);
    playlists = index_new ();

    update_level = 0;
    scan_playlist = scan_row = 0;

    LEAVE;

    /* initialize title formatter */
    playlist_reformat_titles ();
}

void playlist_end (void)
{
    ENTER;

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    active_playlist = playing_playlist = NULL;
    resume_playlist = -1;

    index_free_full (playlists, (IndexFreeFunc) playlist_free);
    playlists = NULL;

    g_hash_table_destroy (unique_id_table);
    unique_id_table = NULL;

    tuple_formatter_free (title_formatter);
    title_formatter = NULL;

    LEAVE;
}

int playlist_count (void)
{
    ENTER;
    int count = index_count (playlists);
    RETURN (count);
}

void playlist_insert_with_id (int at, int id)
{
    ENTER;

    if (at < 0 || at > index_count (playlists))
        at = index_count (playlists);

    index_insert (playlists, at, playlist_new (id));
    number_playlists (at, index_count (playlists) - at);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, -1, 0, 0);
    LEAVE;
}

void playlist_insert (int at)
{
    playlist_insert_with_id (at, -1);
}

void playlist_reorder (int from, int to, int count)
{
    ENTER;

    if (from < 0 || from + count > index_count (playlists) || to < 0 || to +
     count > index_count (playlists) || count < 0)
        RETURN ();

    Index * displaced = index_new ();

    if (to < from)
        index_copy_insert (playlists, to, displaced, -1, from - to);
    else
        index_copy_insert (playlists, from + count, displaced, -1, to - from);

    index_copy_set (playlists, from, playlists, to, count);

    if (to < from)
    {
        index_copy_set (displaced, 0, playlists, to + count, from - to);
        number_playlists (to, from + count - to);
    }
    else
    {
        index_copy_set (displaced, 0, playlists, from, to - from);
        number_playlists (from, to + count - from);
    }

    index_free (displaced);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, -1, 0, 0);
    LEAVE;
}

void playlist_delete (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    bool_t was_playing = (playlist == playing_playlist);

    index_delete_full (playlists, playlist_num, 1, (IndexFreeFunc) playlist_free);

    if (! index_count (playlists))
        index_insert (playlists, 0, playlist_new (-1));

    number_playlists (playlist_num, index_count (playlists) - playlist_num);

    if (playlist == active_playlist)
        active_playlist = index_get (playlists, MIN (playlist_num, index_count (playlists) - 1));
    if (playlist == playing_playlist)
        playing_playlist = NULL;

    queue_update (PLAYLIST_UPDATE_STRUCTURE, -1, 0, 0);
    LEAVE;

    if (was_playing)
        playback_stop ();
}

int playlist_get_unique_id (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int unique_id = playlist->unique_id;
    RETURN (unique_id);
}

int playlist_by_unique_id (int id)
{
    ENTER;

    Playlist * p = g_hash_table_lookup (unique_id_table, GINT_TO_POINTER (id));
    int num = p ? p->number : -1;

    RETURN (num);
}

void playlist_set_filename (int playlist_num, const char * filename)
{
    ENTER_GET_PLAYLIST ();

    str_unref (playlist->filename);
    playlist->filename = str_get (filename);
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, -1, 0, 0);
    LEAVE;
}

char * playlist_get_filename (int playlist_num)
{
    ENTER_GET_PLAYLIST (NULL);
    char * filename = str_ref (playlist->filename);
    RETURN (filename);
}

void playlist_set_title (int playlist_num, const char * title)
{
    ENTER_GET_PLAYLIST ();

    str_unref (playlist->title);
    playlist->title = str_get (title);
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, -1, 0, 0);
    LEAVE;
}

char * playlist_get_title (int playlist_num)
{
    ENTER_GET_PLAYLIST (NULL);
    char * title = str_ref (playlist->title);
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

void playlist_set_active (int playlist_num)
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

int playlist_get_active (void)
{
    ENTER;
    int list = active_playlist ? active_playlist->number : -1;
    RETURN (list);
}

void playlist_set_playing (int playlist_num)
{
    /* get playback state before locking playlists */
    bool_t paused = drct_get_paused ();
    int time = drct_get_time ();

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

int playlist_get_playing (void)
{
    ENTER;
    int list = playing_playlist ? playing_playlist->number: -1;
    RETURN (list);
}

int playlist_get_blank (void)
{
    int list = playlist_get_active ();
    char * title = playlist_get_title (list);

    if (strcmp (title, _(default_title)) || playlist_entry_count (list) > 0)
    {
        list = playlist_count ();
        playlist_insert (list);
    }

    str_unref (title);
    return list;
}

int playlist_get_temporary (void)
{
    int list, count = playlist_count ();
    bool_t found = FALSE;

    for (list = 0; list < count; list ++)
    {
        char * title = playlist_get_title (list);
        found = ! strcmp (title, _(temp_title));
        str_unref (title);

        if (found)
            break;
    }

    if (! found)
    {
        list = playlist_get_blank ();
        playlist_set_title (list, _(temp_title));
    }

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
    if (can_play && drct_get_playing ())
        playback_play (0, drct_get_paused ());
    else
        playlist_set_playing (-1);
}

int playlist_entry_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int count = index_count (playlist->entries);
    RETURN (count);
}

void playlist_entry_insert_batch_raw (int playlist_num, int at,
 Index * filenames, Index * tuples, Index * decoders)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;

    int number = index_count (filenames);

    Index * add = index_new ();
    index_allocate (add, number);

    for (int i = 0; i < number; i ++)
    {
        char * filename = index_get (filenames, i);
        Tuple * tuple = tuples ? index_get (tuples, i) : NULL;
        PluginHandle * decoder = decoders ? index_get (decoders, i) : NULL;
        index_insert (add, -1, entry_new (filename, tuple, decoder));
    }

    index_free (filenames);
    if (decoders)
        index_free (decoders);
    if (tuples)
        index_free (tuples);

    number = index_count (add);
    index_copy_insert (add, 0, playlist->entries, at, -1);
    index_free (add);

    number_entries (playlist, at, entries + number - at);

    for (int count = 0; count < number; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        playlist->total_length += entry->length;
    }

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, at, number);
    LEAVE;
}

void playlist_entry_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);
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
            playlist->focus = index_get (playlist->entries, at + number);
        else if (at > 0)
            playlist->focus = index_get (playlist->entries, at - 1);
        else
            playlist->focus = NULL;
    }

    for (int count = 0; count < number; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);

        if (entry->queued)
            playlist->queued = g_list_remove (playlist->queued, entry);

        if (entry->selected)
        {
            playlist->selected_count --;
            playlist->selected_length -= entry->length;
        }

        playlist->total_length -= entry->length;
    }

    index_delete_full (playlist->entries, at, number, (IndexFreeFunc) entry_free);
    number_entries (playlist, at, entries - at - number);

    if (position_changed && get_bool (NULL, "advance_on_delete"))
        can_play = next_song_locked (playlist, get_bool (NULL, "repeat"), at);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, at, 0);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (can_play);
}

char * playlist_entry_get_filename (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (NULL);
    char * filename = str_ref (entry->filename);
    RETURN (filename);
}

PluginHandle * playlist_entry_get_decoder (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, ! fast, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    RETURN (decoder);
}

Tuple * playlist_entry_get_tuple (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    Tuple * tuple = entry ? entry->tuple : NULL;

    if (tuple)
        tuple_ref (tuple);

    RETURN (tuple);
}

char * playlist_entry_get_title (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    char * title = entry ? str_ref (entry->formatted ? entry->formatted : entry->title) : NULL;

    RETURN (title);
}

void playlist_entry_describe (int playlist_num, int entry_num,
 char * * title, char * * artist, char * * album, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);

    if (title)
        * title = (entry && entry->title) ? str_ref (entry->title) : NULL;
    if (artist)
        * artist = (entry && entry->artist) ? str_ref (entry->artist) : NULL;
    if (album)
        * album = (entry && entry->album) ? str_ref (entry->album) : NULL;

    LEAVE;
}

int playlist_entry_get_length (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    int length = entry ? entry->length : 0;

    RETURN (length);
}

void playlist_set_position (int playlist_num, int entry_num)
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

int playlist_get_position (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int position = playlist->position ? playlist->position->number : -1;
    RETURN (position);
}

void playlist_set_focus (int playlist_num, int entry_num)
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
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist_num, first, last + 1 - first);

    LEAVE;
}

int playlist_get_focus (int playlist_num)
{
    ENTER_GET_PLAYLIST (-1);
    int focus = playlist->focus ? playlist->focus->number : -1;
    RETURN (focus);
}

void playlist_entry_set_selected (int playlist_num, int entry_num,
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

    queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, entry_num, 1);
    LEAVE;
}

bool_t playlist_entry_get_selected (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (FALSE);
    bool_t selected = entry->selected;
    RETURN (selected);
}

int playlist_selected_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int selected_count = playlist->selected_count;
    RETURN (selected_count);
}

void playlist_select_all (int playlist_num, bool_t selected)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);
    int first = entries, last = 0;

    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

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
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, first, last + 1 - first);

    LEAVE;
}

int playlist_shift (int playlist_num, int entry_num, int distance)
{
    ENTER_GET_ENTRY (0);

    if (! entry->selected || ! distance)
        RETURN (0);

    int entries = index_count (playlist->entries);
    int shift = 0, center, top, bottom;

    if (distance < 0)
    {
        for (center = entry_num; center > 0 && shift > distance; )
        {
            entry = index_get (playlist->entries, -- center);
            if (! entry->selected)
                shift --;
        }
    }
    else
    {
        for (center = entry_num + 1; center < entries && shift < distance; )
        {
            entry = index_get (playlist->entries, center ++);
            if (! entry->selected)
                shift ++;
        }
    }

    top = bottom = center;

    for (int i = 0; i < top; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (entry->selected)
            top = i;
    }

    for (int i = entries; i > bottom; i --)
    {
        entry = index_get (playlist->entries, i - 1);
        if (entry->selected)
            bottom = i;
    }

    Index * temp = index_new ();

    for (int i = top; i < center; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (! entry->selected)
            index_insert (temp, -1, entry);
    }

    for (int i = top; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (entry->selected)
            index_insert (temp, -1, entry);
    }

    for (int i = center; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (! entry->selected)
            index_insert (temp, -1, entry);
    }

    index_copy_set (temp, 0, playlist->entries, top, bottom - top);

    number_entries (playlist, top, bottom - top);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, top, bottom - top);

    RETURN (shift);
}

static Entry * find_unselected_focus (Playlist * playlist)
{
    if (! playlist->focus || ! playlist->focus->selected)
        return playlist->focus;

    int entries = index_count (playlist->entries);

    for (int search = playlist->focus->number + 1; search < entries; search ++)
    {
        Entry * entry = index_get (playlist->entries, search);
        if (! entry->selected)
            return entry;
    }

    for (int search = playlist->focus->number; search --;)
    {
        Entry * entry = index_get (playlist->entries, search);
        if (! entry->selected)
            return entry;
    }

    return NULL;
}

void playlist_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    if (! playlist->selected_count)
        RETURN ();

    int entries = index_count (playlist->entries);
    bool_t position_changed = FALSE;
    bool_t was_playing = FALSE;
    bool_t can_play = FALSE;

    Index * others = index_new ();
    index_allocate (others, entries - playlist->selected_count);

    if (playlist->position && playlist->position->selected)
    {
        position_changed = TRUE;
        was_playing = (playlist == playing_playlist);

        set_position (playlist, NULL, FALSE);
    }

    playlist->focus = find_unselected_focus (playlist);

    int before = 0, after = 0;
    bool_t found = FALSE;

    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (entry->selected)
        {
            if (entry->queued)
                playlist->queued = g_list_remove (playlist->queued, entry);

            playlist->total_length -= entry->length;
            entry_free (entry);

            found = TRUE;
            after = 0;
        }
        else
        {
            index_insert (others, -1, entry);

            if (found)
                after ++;
            else
                before ++;
        }
    }

    index_free (playlist->entries);
    playlist->entries = others;

    playlist->selected_count = 0;
    playlist->selected_length = 0;

    entries = index_count (playlist->entries);
    number_entries (playlist, before, entries - before);

    if (position_changed && get_bool (NULL, "advance_on_delete"))
        can_play = next_song_locked (playlist, get_bool (NULL, "repeat"), entries - after);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, before, entries - after - before);
    LEAVE;

    if (position_changed)
        hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (can_play);
}

void playlist_reverse (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    Index * reversed = index_new ();
    index_allocate (reversed, entries);

    for (int count = entries; count --; )
        index_insert (reversed, -1, index_get (playlist->entries, count));

    index_free (playlist->entries);
    playlist->entries = reversed;

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

void playlist_reverse_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    Index * reversed = index_new ();
    index_allocate (reversed, playlist->selected_count);

    for (int count = entries; count --; )
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_insert (reversed, -1, index_get (playlist->entries, count));
    }

    int count2 = 0;
    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_set (playlist->entries, count, index_get (reversed, count2 ++));
    }

    index_free (reversed);

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

void playlist_randomize (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    for (int i = 0; i < entries; i ++)
    {
        int j = i + rand () % (entries - i);

        Entry * entry = index_get (playlist->entries, j);
        index_set (playlist->entries, j, index_get (playlist->entries, i));
        index_set (playlist->entries, i, entry);
    }

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

void playlist_randomize_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    Index * selected = index_new ();
    index_allocate (selected, playlist->selected_count);

    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_insert (selected, -1, entry);
    }

    for (int i = 0; i < playlist->selected_count; i ++)
    {
        int j = i + rand () % (playlist->selected_count - i);

        Entry * entry = index_get (selected, j);
        index_set (selected, j, index_get (selected, i));
        index_set (selected, i, entry);
    }

    int count2 = 0;
    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_set (playlist->entries, count, index_get (selected, count2 ++));
    }

    index_free (selected);

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

enum {COMPARE_TYPE_FILENAME, COMPARE_TYPE_TUPLE, COMPARE_TYPE_TITLE};

typedef int (* CompareFunc) (const void * a, const void * b);

typedef struct {
    int type;
    CompareFunc func;
} CompareData;

static int compare_cb (const void * _a, const void * _b, void * _data)
{
    const Entry * a = _a, * b = _b;
    CompareData * data = _data;

    int diff = 0;

    if (data->type == COMPARE_TYPE_FILENAME)
        diff = data->func (a->filename, b->filename);
    else if (data->type == COMPARE_TYPE_TUPLE)
        diff = data->func (a->tuple, b->tuple);
    else if (data->type == COMPARE_TYPE_TITLE)
        diff = data->func (a->formatted ? a->formatted : a->filename,
         b->formatted ? b->formatted : b->filename);

    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static void sort (Playlist * playlist, CompareData * data)
{
    index_sort_with_data (playlist->entries, compare_cb, data);
    number_entries (playlist, 0, index_count (playlist->entries));

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, index_count (playlist->entries));
}

static void sort_selected (Playlist * playlist, CompareData * data)
{
    int entries = index_count (playlist->entries);

    Index * selected = index_new ();
    index_allocate (selected, playlist->selected_count);

    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_insert (selected, -1, entry);
    }

    index_sort_with_data (selected, compare_cb, data);

    int count2 = 0;
    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_set (playlist->entries, count, index_get (selected, count2 ++));
    }

    index_free (selected);

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
}

static bool_t entries_are_scanned (Playlist * playlist, bool_t selected)
{
    int entries = index_count (playlist->entries);
    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (selected && ! entry->selected)
            continue;

        if (! entry->tuple)
        {
            interface_show_error (_("The playlist cannot be sorted because "
             "metadata scanning is still in progress (or has been disabled)."));
            return FALSE;
        }
    }

    return TRUE;
}

void playlist_sort_by_filename (int playlist_num, int (* compare)
 (const char * a, const char * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_FILENAME, (CompareFunc) compare};
    sort (playlist, & data);

    LEAVE;
}

void playlist_sort_by_tuple (int playlist_num, int (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_TUPLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, & data);

    LEAVE;
}

void playlist_sort_by_title (int playlist_num, int (* compare) (const char *
 a, const char * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_TITLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_filename (int playlist_num, int (* compare)
 (const char * a, const char * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_FILENAME, (CompareFunc) compare};
    sort_selected (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_tuple (int playlist_num, int (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_TUPLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_title (int playlist_num, int (* compare)
 (const char * a, const char * b))
{
    ENTER_GET_PLAYLIST ();

    CompareData data = {COMPARE_TYPE_TITLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, & data);

    LEAVE;
}

void playlist_reformat_titles (void)
{
    ENTER;

    if (title_formatter)
        tuple_formatter_free (title_formatter);

    char * format = get_str (NULL, "generic_title_format");
    title_formatter = tuple_formatter_new (format);
    str_unref (format);

    for (int playlist_num = 0; playlist_num < index_count (playlists); playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        int entries = index_count (playlist->entries);

        for (int count = 0; count < entries; count++)
        {
            Entry * entry = index_get (playlist->entries, count);
            str_unref (entry->formatted);

            if (entry->tuple)
                entry->formatted = tuple_format_title (title_formatter, entry->tuple);
            else
                entry->formatted = NULL;
        }

        queue_update (PLAYLIST_UPDATE_METADATA, playlist_num, 0, entries);
    }

    LEAVE;
}

void playlist_trigger_scan (void)
{
    ENTER;

    for (int i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        p->scanning = TRUE;
    }

    scan_restart ();

    LEAVE;
}

static void playlist_rescan_real (int playlist_num, bool_t selected)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);

    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (! selected || entry->selected)
            entry_set_tuple (playlist, entry, NULL);
    }

    queue_update (PLAYLIST_UPDATE_METADATA, playlist->number, 0, entries);
    LEAVE;
}

void playlist_rescan (int playlist_num)
{
    playlist_rescan_real (playlist_num, FALSE);
}

void playlist_rescan_selected (int playlist_num)
{
    playlist_rescan_real (playlist_num, TRUE);
}

void playlist_rescan_file (const char * filename)
{
    ENTER;

    int num_playlists = index_count (playlists);

    for (int playlist_num = 0; playlist_num < num_playlists; playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        int num_entries = index_count (playlist->entries);

        for (int entry_num = 0; entry_num < num_entries; entry_num ++)
        {
            Entry * entry = index_get (playlist->entries, entry_num);

            if (! strcmp (entry->filename, filename))
            {
                entry_set_tuple (playlist, entry, NULL);
                queue_update (PLAYLIST_UPDATE_METADATA, playlist_num, entry_num, 1);
            }
        }
    }

    LEAVE;
}

int64_t playlist_get_total_length (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->total_length;
    RETURN (length);
}

int64_t playlist_get_selected_length (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int64_t length = playlist->selected_length;
    RETURN (length);
}

int playlist_queue_count (int playlist_num)
{
    ENTER_GET_PLAYLIST (0);
    int count = g_list_length (playlist->queued);
    RETURN (count);
}

void playlist_queue_insert (int playlist_num, int at, int entry_num)
{
    ENTER_GET_ENTRY ();

    if (entry->queued)
        RETURN ();

    if (at < 0)
        playlist->queued = g_list_append (playlist->queued, entry);
    else
        playlist->queued = g_list_insert (playlist->queued, entry, at);

    entry->queued = TRUE;

    queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, entry_num, 1);
    LEAVE;
}

void playlist_queue_insert_selected (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count(playlist->entries);
    int first = entries, last = 0;

    for (int count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (! entry->selected || entry->queued)
            continue;

        if (at < 0)
            playlist->queued = g_list_append (playlist->queued, entry);
        else
            playlist->queued = g_list_insert (playlist->queued, entry, at++);

        entry->queued = TRUE;
        first = MIN (first, entry->number);
        last = entry->number;
    }

    if (first < entries)
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, first, last + 1 - first);

    LEAVE;
}

int playlist_queue_get_entry (int playlist_num, int at)
{
    ENTER_GET_PLAYLIST (-1);

    GList * node = g_list_nth (playlist->queued, at);
    int entry_num = node ? ((Entry *) node->data)->number : -1;

    RETURN (entry_num);
}

int playlist_queue_find_entry (int playlist_num, int entry_num)
{
    ENTER_GET_ENTRY (-1);
    int pos = entry->queued ? g_list_index (playlist->queued, entry) : -1;
    RETURN (pos);
}

void playlist_queue_delete (int playlist_num, int at, int number)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);
    int first = entries, last = 0;

    if (at == 0)
    {
        while (playlist->queued && number --)
        {
            Entry * entry = playlist->queued->data;
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
            Entry * entry = anchor->next->data;
            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;

            playlist->queued = g_list_delete_link (playlist->queued, anchor->next);
        }
    }

DONE:
    if (first < entries)
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, first, last + 1 - first);

    LEAVE;
}

void playlist_queue_delete_selected (int playlist_num)
{
    ENTER_GET_PLAYLIST ();

    int entries = index_count (playlist->entries);
    int first = entries, last = 0;

    for (GList * node = playlist->queued; node; )
    {
        GList * next = node->next;
        Entry * entry = node->data;

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
        queue_update (PLAYLIST_UPDATE_SELECTION, playlist->number, first, last + 1 - first);

    LEAVE;
}

static bool_t shuffle_prev (Playlist * playlist)
{
    int entries = index_count (playlist->entries);
    Entry * found = NULL;

    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (entry->shuffle_num && (! playlist->position ||
         entry->shuffle_num < playlist->position->shuffle_num) && (! found
         || entry->shuffle_num > found->shuffle_num))
            found = entry;
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

    if (get_bool (NULL, "shuffle"))
    {
        if (! shuffle_prev (playlist))
            RETURN (FALSE);
    }
    else
    {
        if (! playlist->position || playlist->position->number == 0)
            RETURN (FALSE);

        set_position (playlist, index_get (playlist->entries,
         playlist->position->number - 1), TRUE);
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    if (was_playing)
        change_playback (TRUE);

    return TRUE;
}

static bool_t shuffle_next (Playlist * playlist)
{
    int entries = index_count (playlist->entries), choice = 0, count;
    Entry * found = NULL;

    for (count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (! entry->shuffle_num)
            choice ++;
        else if (playlist->position && entry->shuffle_num >
         playlist->position->shuffle_num && (! found || entry->shuffle_num
         < found->shuffle_num))
            found = entry;
    }

    if (found)
    {
        set_position (playlist, found, FALSE);
        return TRUE;
    }

    if (! choice)
        return FALSE;

    choice = rand () % choice;

    for (count = 0; ; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (! entry->shuffle_num)
        {
            if (! choice)
            {
                set_position (playlist, entry, TRUE);
                return TRUE;
            }

            choice --;
        }
    }
}

static void shuffle_reset (Playlist * playlist)
{
    int entries = index_count (playlist->entries);

    playlist->last_shuffle_num = 0;

    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        entry->shuffle_num = 0;
    }
}

static bool_t next_song_locked (Playlist * playlist, bool_t repeat, int hint)
{
    int entries = index_count (playlist->entries);
    if (! entries)
        return FALSE;

    if (playlist->queued)
    {
        set_position (playlist, playlist->queued->data, TRUE);
        playlist->queued = g_list_remove (playlist->queued, playlist->position);
        playlist->position->queued = FALSE;
    }
    else if (get_bool (NULL, "shuffle"))
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

        set_position (playlist, index_get (playlist->entries, hint), TRUE);
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

char * playback_entry_get_filename (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, FALSE);
    char * filename = entry ? str_ref (entry->filename) : NULL;

    RETURN (filename);
}

PluginHandle * playback_entry_get_decoder (void)
{
    ENTER;

    Entry * entry = get_playback_entry (TRUE, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    RETURN (decoder);
}

Tuple * playback_entry_get_tuple (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    Tuple * tuple = entry ? entry->tuple : NULL;

    if (tuple)
        tuple_ref (tuple);

    RETURN (tuple);
}

char * playback_entry_get_title (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    char * title = entry ? str_ref (entry->formatted ? entry->formatted : entry->title) : NULL;

    RETURN (title);
}

int playback_entry_get_length (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    int length = entry ? entry->length : 0;

    RETURN (length);
}

void playback_entry_set_tuple (Tuple * tuple)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        RETURN ();

    Entry * entry = playing_playlist->position;
    entry_set_tuple (playing_playlist, entry, tuple);

    queue_update (PLAYLIST_UPDATE_METADATA, playing_playlist->number, entry->number, 1);
    LEAVE;
}

void playlist_save_state (void)
{
    /* get playback state before locking playlists */
    bool_t paused = drct_get_paused ();
    int time = drct_get_time ();

    ENTER;

    const char * user_dir = get_path (AUD_PATH_USER_DIR);
    SCONCAT2 (path, user_dir, "/" STATE_FILE);

    FILE * handle = g_fopen (path, "w");
    if (! handle)
        RETURN ();

    fprintf (handle, "active %d\n", active_playlist ? active_playlist->number : -1);
    fprintf (handle, "playing %d\n", playing_playlist ? playing_playlist->number : -1);

    for (int playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);

        fprintf (handle, "playlist %d\n", playlist_num);

        if (playlist->filename)
            fprintf (handle, "filename %s\n", playlist->filename);

        fprintf (handle, "position %d\n", playlist->position ? playlist->position->number : -1);

        if (playlist == playing_playlist)
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

static char * parse_string (const char * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? str_get (parse_value) : NULL;
}

void playlist_load_state (void)
{
    ENTER;
    int playlist_num;

    const char * user_dir = get_path (AUD_PATH_USER_DIR);
    SCONCAT2 (path, user_dir, "/" STATE_FILE);

    FILE * handle = g_fopen (path, "r");
    if (! handle)
        RETURN ();

    parse_next (handle);

    if (parse_integer ("active", & playlist_num))
    {
        if (! (active_playlist = lookup_playlist (playlist_num)))
            active_playlist = index_get (playlists, 0);
        parse_next (handle);
    }

    if (parse_integer ("playing", & resume_playlist))
        parse_next (handle);

    while (parse_integer ("playlist", & playlist_num) && playlist_num >= 0 &&
     playlist_num < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        int entries = index_count (playlist->entries);

        parse_next (handle);

        char * s;
        if ((s = parse_string ("filename")))
        {
            str_unref (playlist->filename);
            playlist->filename = s;
            parse_next (handle);
        }

        int position = -1;
        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            set_position (playlist, index_get (playlist->entries, position), TRUE);

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

    for (int i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        memset (& p->last_update, 0, sizeof (Update));
        memset (& p->next_update, 0, sizeof (Update));
    }

    update_level = 0;

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    LEAVE;
}

void playlist_resume (void)
{
    playlist_set_playing (resume_playlist);
}
