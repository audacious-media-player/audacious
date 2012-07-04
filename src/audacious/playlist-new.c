/*
 * playlist-new.c
 * Copyright 2009-2012 John Lindgren
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

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/tuple.h>

#include "config.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "plugins.h"
#include "util.h"

enum {RESUME_STOP, RESUME_PLAY, RESUME_PAUSE};

#define SCAN_THREADS 2
#define STATE_FILE "playlist-state"

#define ENTER pthread_mutex_lock (& mutex)
#define LEAVE pthread_mutex_unlock (& mutex)

#define LEAVE_RET_VOID do { \
    pthread_mutex_unlock (& mutex); \
    return; \
} while (0)

#define LEAVE_RET(ret) do { \
    pthread_mutex_unlock (& mutex); \
    return ret; \
} while (0)

#define DECLARE_PLAYLIST \
    Playlist * playlist

#define DECLARE_PLAYLIST_ENTRY \
    Playlist * playlist; \
    Entry * entry

#define LOOKUP_PLAYLIST do { \
    if (! (playlist = lookup_playlist (playlist_num))) \
        LEAVE_RET_VOID; \
} while (0)

#define LOOKUP_PLAYLIST_RET(ret) do { \
    if (! (playlist = lookup_playlist (playlist_num))) \
        LEAVE_RET(ret); \
} while (0)

#define LOOKUP_PLAYLIST_ENTRY do { \
    LOOKUP_PLAYLIST; \
    if (! (entry = lookup_entry (playlist, entry_num))) \
        LEAVE_RET_VOID; \
} while (0)

#define LOOKUP_PLAYLIST_ENTRY_RET(ret) do { \
    LOOKUP_PLAYLIST_RET(ret); \
    if (! (entry = lookup_entry (playlist, entry_num))) \
        LEAVE_RET(ret); \
} while (0)

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
    bool_t segmented;
    int start, end;
} Entry;

typedef struct {
    int number, unique_id;
    char * filename, * title;
    bool_t modified;
    Index * entries;
    Entry * position;
    int selected_count;
    int last_shuffle_num;
    GList * queued;
    int64_t total_length, selected_length;
    bool_t scanning, scan_ending;
    Update next_update, last_update;
    int resume_state, resume_time;
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

static int update_source = 0, update_level;

typedef struct {
    Playlist * playlist;
    Entry * entry;
} ScanItem;

static pthread_t scan_threads[SCAN_THREADS];
static bool_t scan_quit;
static int scan_playlist, scan_row;
static GQueue scan_queue = G_QUEUE_INIT;
static ScanItem * scan_items[SCAN_THREADS];

static void * scanner (void * unused);
static void scan_trigger (void);

static char * title_format;

static char * title_from_tuple (Tuple * tuple)
{
    if (! title_format)
        title_format = get_string (NULL, "generic_title_format");

    return tuple_format_title (tuple, title_format);
}

static void entry_set_tuple_real (Entry * entry, Tuple * tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (entry->segmented && entry->tuple)
    {
        if (tuple)
            tuple_unref (tuple);
        return;
    }

    if (entry->tuple)
        tuple_unref (entry->tuple);
    entry->tuple = tuple;

    str_unref (entry->formatted);
    str_unref (entry->title);
    str_unref (entry->artist);
    str_unref (entry->album);

    describe_song (entry->filename, tuple, & entry->title, & entry->artist, & entry->album);

    if (! tuple)
    {
        entry->formatted = NULL;
        entry->length = 0;
        entry->segmented = FALSE;
        entry->start = 0;
        entry->end = -1;
    }
    else
    {
        entry->formatted = title_from_tuple (tuple);
        entry->length = tuple_get_int (tuple, FIELD_LENGTH, NULL);
        if (entry->length < 0)
            entry->length = 0;

        if (tuple_get_value_type (tuple, FIELD_SEGMENT_START, NULL) == TUPLE_INT)
        {
            entry->segmented = TRUE;
            entry->start = tuple_get_int (tuple, FIELD_SEGMENT_START, NULL);

            if (tuple_get_value_type (tuple, FIELD_SEGMENT_END, NULL) == TUPLE_INT)
                entry->end = tuple_get_int (tuple, FIELD_SEGMENT_END, NULL);
            else
                entry->end = -1;
        }
        else
            entry->segmented = FALSE;
    }
}

static void entry_set_tuple (Playlist * playlist, Entry * entry, Tuple * tuple)
{
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

static void entry_cancel_scan (Entry * entry)
{
    GList * next;
    for (GList * node = scan_queue.head; node; node = next)
    {
        ScanItem * item = node->data;
        next = node->next;

        if (item->entry == entry)
        {
            g_queue_delete_link (& scan_queue, node);
            g_slice_free (ScanItem, item);
        }
    }

    for (int i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_items[i] && scan_items[i]->entry == entry)
        {
            g_slice_free (ScanItem, scan_items[i]);
            scan_items[i] = NULL;
        }
    }
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
    entry->segmented = FALSE;
    entry->start = 0;
    entry->end = -1;

    entry_set_tuple_real (entry, tuple);
    return entry;
}

static void entry_free (Entry * entry)
{
    entry_cancel_scan (entry);

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
    playlist->selected_count = 0;
    playlist->last_shuffle_num = 0;
    playlist->queued = NULL;
    playlist->total_length = 0;
    playlist->selected_length = 0;
    playlist->scanning = FALSE;
    playlist->scan_ending = FALSE;
    playlist->resume_state = RESUME_STOP;
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

    for (int count = 0; count < index_count (playlist->entries); count ++)
        entry_free (index_get (playlist->entries, count));

    index_free (playlist->entries);
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
                scan_trigger ();
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
    LEAVE_RET (pending);
}

int playlist_updated_range (int playlist_num, int * at, int * count)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    Update * u = & playlist->last_update;

    int level = u->level;
    * at = u->before;
    * count = index_count (playlist->entries) - u->before - u->after;

    LEAVE_RET (level);
}

bool_t playlist_scan_in_progress (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    bool_t scanning = playlist->scanning || playlist->scan_ending;

    LEAVE_RET (scanning);
}

static bool_t entry_scan_is_queued (Entry * entry)
{
    for (GList * node = scan_queue.head; node; node = node->next)
    {
        ScanItem * item = node->data;
        if (item->entry == entry)
            return TRUE;
    }

    for (int i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_items[i] && scan_items[i]->entry == entry)
            return TRUE;
    }

    return FALSE;
}

static void entry_queue_scan (Playlist * playlist, Entry * entry)
{
    if (entry_scan_is_queued (entry))
        return;

    ScanItem * item = g_slice_new (ScanItem);
    item->playlist = playlist;
    item->entry = entry;
    g_queue_push_tail (& scan_queue, item);

    pthread_cond_broadcast (& cond);
}

static void check_scan_complete (Playlist * p)
{
    if (! p->scan_ending)
        return;

    for (GList * node = scan_queue.head; node; node = node->next)
    {
        ScanItem * item = node->data;
        if (item->playlist == p)
            return;
    }

    for (int i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_items[i] && scan_items[i]->playlist == p)
            return;
    }

    p->scan_ending = FALSE;

    event_queue_cancel ("playlist scan complete", NULL);
    event_queue ("playlist scan complete", NULL);
}

static ScanItem * entry_find_to_scan (void)
{
    ScanItem * item = g_queue_pop_head (& scan_queue);
    if (item)
        return item;

    while (scan_playlist < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, scan_playlist);

        if (playlist->scanning)
        {
            while (scan_row < index_count (playlist->entries))
            {
                Entry * entry = index_get (playlist->entries, scan_row);

                if (! entry->tuple && ! entry_scan_is_queued (entry))
                {
                    item = g_slice_new (ScanItem);
                    item->playlist = playlist;
                    item->entry = entry;
                    return item;
                }

                scan_row ++;
            }

            playlist->scanning = FALSE;
            playlist->scan_ending = TRUE;
            check_scan_complete (playlist);
        }

        scan_playlist ++;
        scan_row = 0;
    }

    return NULL;
}

static void * scanner (void * data)
{
    ENTER;

    int i = GPOINTER_TO_INT (data);

    while (! scan_quit)
    {
        if (! scan_items[i])
            scan_items[i] = entry_find_to_scan ();

        if (! scan_items[i])
        {
            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        Playlist * playlist = scan_items[i]->playlist;
        Entry * entry = scan_items[i]->entry;
        char * filename = str_ref (entry->filename);
        PluginHandle * decoder = entry->decoder;
        bool_t need_tuple = entry->tuple ? FALSE : TRUE;

        LEAVE;

        if (! decoder)
            decoder = file_find_decoder (filename, FALSE);

        Tuple * tuple = (need_tuple && decoder) ? file_read_tuple (filename, decoder) : NULL;

        ENTER;

        str_unref (filename);

        if (! scan_items[i]) /* scan canceled */
        {
            if (tuple)
                tuple_unref (tuple);
            continue;
        }

        entry->decoder = decoder;

        if (tuple)
        {
            entry_set_tuple (playlist, entry, tuple);
            queue_update (PLAYLIST_UPDATE_METADATA, playlist->number, entry->number, 1);
        }
        else if (need_tuple || ! decoder)
        {
            entry_set_failed (playlist, entry);
            queue_update (PLAYLIST_UPDATE_METADATA, playlist->number, entry->number, 1);
        }

        g_slice_free (ScanItem, scan_items[i]);
        scan_items[i] = NULL;

        pthread_cond_broadcast (& cond);
        check_scan_complete (playlist);
    }

    LEAVE_RET (NULL);
}

static void scan_trigger (void)
{
    scan_playlist = 0;
    scan_row = 0;
    pthread_cond_broadcast (& cond);
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
            entry_queue_scan (playlist, entry);
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
            entry_queue_scan (playing_playlist, entry);
            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        return entry;
    }
}

static void get_resume_state (int * state, int * time)
{
    if (playback_get_playing ())
    {
        * state = playback_get_paused () ? RESUME_PAUSE : RESUME_PLAY;
        * time = playback_get_time ();
    }
    else
    {
        * state = RESUME_STOP;
        * time = 0;
    }
}

static void resume (int state, int time)
{
    if (state == RESUME_PLAY || state == RESUME_PAUSE)
        playback_play (time, state == RESUME_PAUSE);
}

void playlist_init (void)
{
    srand (time (NULL));

    ENTER;

    unique_id_table = g_hash_table_new (g_direct_hash, g_direct_equal);
    playlists = index_new ();

    update_level = 0;

    scan_quit = FALSE;
    scan_playlist = scan_row = 0;

    for (int i = 0; i < SCAN_THREADS; i ++)
        pthread_create (& scan_threads[i], NULL, scanner, GINT_TO_POINTER (i));

    LEAVE;
}

void playlist_end (void)
{
    ENTER;

    scan_quit = TRUE;
    pthread_cond_broadcast (& cond);

    LEAVE;

    for (int i = 0; i < SCAN_THREADS; i ++)
        pthread_join (scan_threads[i], NULL);

    ENTER;

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    active_playlist = playing_playlist = NULL;

    for (int i = 0; i < index_count (playlists); i ++)
        playlist_free (index_get (playlists, i));

    index_free (playlists);
    playlists = NULL;

    g_hash_table_destroy (unique_id_table);
    unique_id_table = NULL;

    g_free (title_format);
    title_format = NULL;

    LEAVE;
}

int playlist_count (void)
{
    ENTER;
    int count = index_count (playlists);
    LEAVE_RET (count);
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
        LEAVE_RET_VOID;

    Index * displaced = index_new ();

    if (to < from)
        index_copy_append (playlists, to, displaced, from - to);
    else
        index_copy_append (playlists, from + count, displaced, to - from);

    index_move (playlists, from, to, count);

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
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    index_delete (playlists, playlist_num, 1);
    playlist_free (playlist);

    if (! index_count (playlists))
        index_insert (playlists, 0, playlist_new (-1));

    number_playlists (playlist_num, index_count (playlists) - playlist_num);

    if (playlist == active_playlist)
        active_playlist = index_get (playlists, MIN (playlist_num, index_count (playlists) - 1));
    if (playlist == playing_playlist)
        playing_playlist = NULL;

    queue_update (PLAYLIST_UPDATE_STRUCTURE, -1, 0, 0);
    LEAVE;
}

int playlist_get_unique_id (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    int unique_id = playlist->unique_id;

    LEAVE_RET (unique_id);
}

int playlist_by_unique_id (int id)
{
    ENTER;

    Playlist * p = g_hash_table_lookup (unique_id_table, GINT_TO_POINTER (id));
    int num = p ? p->number : -1;

    LEAVE_RET (num);
}

void playlist_set_filename (int playlist_num, const char * filename)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    str_unref (playlist->filename);
    playlist->filename = str_get (filename);
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, -1, 0, 0);
    LEAVE;
}

char * playlist_get_filename (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (NULL);

    char * filename = str_ref (playlist->filename);

    LEAVE_RET (filename);
}

void playlist_set_title (int playlist_num, const char * title)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    str_unref (playlist->title);
    playlist->title = str_get (title);
    playlist->modified = TRUE;

    queue_update (PLAYLIST_UPDATE_METADATA, -1, 0, 0);
    LEAVE;
}

char * playlist_get_title (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (NULL);

    char * title = str_ref (playlist->title);

    LEAVE_RET (title);
}

void playlist_set_modified (int playlist_num, bool_t modified)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    playlist->modified = modified;

    LEAVE;
}

bool_t playlist_get_modified (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    bool_t modified = playlist->modified;

    LEAVE_RET (modified);
}

void playlist_set_active (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
    LEAVE_RET (list);
}

void playlist_set_playing (int playlist_num)
{
    if (playlist_num == playlist_get_playing ())
        return;

    /* get playback state and stop playback before locking playlists */
    int state, time;
    get_resume_state (& state, & time);

    if (state != RESUME_STOP)
        playback_stop ();

    ENTER;

    if (playing_playlist)
    {
        playing_playlist->resume_state = state;
        playing_playlist->resume_time = time;
    }

    DECLARE_PLAYLIST;

    if (playlist_num < 0)
        playlist = NULL;
    else
        LOOKUP_PLAYLIST;

    playing_playlist = playlist;

    if (playlist)
    {
        state = playlist->resume_state;
        time = playlist->resume_time;
    }
    else
    {
        state = RESUME_STOP;
        time = 0;
    }

    LEAVE;

    hook_call ("playlist set playing", NULL);

    /* resume playback after unlocking playlists */
    resume (state, time);
}

int playlist_get_playing (void)
{
    ENTER;
    int list = playing_playlist ? playing_playlist->number: -1;
    LEAVE_RET (list);
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
    playlist->resume_state = RESUME_STOP;
    playlist->resume_time = 0;

    /* move entry to top of shuffle list */
    if (entry && update_shuffle)
        entry->shuffle_num = ++ playlist->last_shuffle_num;
}

int playlist_entry_count (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    int count = index_count (playlist->entries);

    LEAVE_RET (count);
}

void playlist_entry_insert_batch_raw (int playlist_num, int at,
 Index * filenames, Index * tuples, Index * decoders)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
        index_append (add, entry_new (filename, tuple, decoder));
    }

    index_free (filenames);
    if (decoders)
        index_free (decoders);
    if (tuples)
        index_free (tuples);

    number = index_count (add);
    index_merge_insert (playlist->entries, at, add);
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
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing () &&
     playlist_get_position (playlist_num) >= at && playlist_get_position
     (playlist_num) < at + number)
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    int entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    if (playlist->position && playlist->position->number >= at &&
     playlist->position->number < at + number)
        set_position (playlist, NULL, FALSE);

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
        entry_free (entry);
    }

    index_delete (playlist->entries, at, number);
    number_entries (playlist, at, entries - at - number);

    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, at, 0);
    LEAVE;
}

char * playlist_entry_get_filename (int playlist_num, int entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    char * filename = str_ref (entry->filename);

    LEAVE_RET (filename);
}

PluginHandle * playlist_entry_get_decoder (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, ! fast, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    LEAVE_RET (decoder);
}

Tuple * playlist_entry_get_tuple (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    Tuple * tuple = entry ? entry->tuple : NULL;

    if (tuple)
        tuple_ref (tuple);

    LEAVE_RET (tuple);
}

char * playlist_entry_get_title (int playlist_num, int entry_num, bool_t fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    char * title = entry ? str_ref (entry->formatted ? entry->formatted : entry->title) : NULL;

    LEAVE_RET (title);
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

    LEAVE_RET (length);
}

void playlist_set_position (int playlist_num, int entry_num)
{
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST_ENTRY;

    if (entry_num == -1)
    {
        LOOKUP_PLAYLIST;
        entry = NULL;
    }
    else
        LOOKUP_PLAYLIST_ENTRY;

    set_position (playlist, entry, TRUE);
    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
}

int playlist_get_position (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    int position = playlist->position ? playlist->position->number : -1;

    LEAVE_RET (position);
}

void playlist_entry_set_selected (int playlist_num, int entry_num,
 bool_t selected)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY;

    if (entry->selected == selected)
        LEAVE_RET_VOID;

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
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (FALSE);

    bool_t selected = entry->selected;

    LEAVE_RET (selected);
}

int playlist_selected_count (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    int selected_count = playlist->selected_count;

    LEAVE_RET (selected_count);
}

void playlist_select_all (int playlist_num, bool_t selected)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! entry->selected || ! distance)
        LEAVE_RET (0);

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
            index_append (temp, entry);
    }

    for (int i = top; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (entry->selected)
            index_append (temp, entry);
    }

    for (int i = center; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (! entry->selected)
            index_append (temp, entry);
    }

    index_copy_set (temp, 0, playlist->entries, top, bottom - top);

    number_entries (playlist, top, bottom - top);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, top, bottom - top);

    LEAVE_RET (shift);
}

void playlist_delete_selected (int playlist_num)
{
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing () &&
     playlist_get_position (playlist_num) >= 0 && playlist_entry_get_selected
     (playlist_num, playlist_get_position (playlist_num)))
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (! playlist->selected_count)
        LEAVE_RET_VOID;

    int entries = index_count (playlist->entries);

    Index * others = index_new ();
    index_allocate (others, entries - playlist->selected_count);

    if (playlist->position && playlist->position->selected)
        set_position (playlist, NULL, FALSE);

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
            index_append (others, entry);

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

    number_entries (playlist, before, index_count (playlist->entries) - before);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, before,
     index_count (playlist->entries) - after - before);
    LEAVE;
}

void playlist_reverse (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    int entries = index_count (playlist->entries);

    Index * reversed = index_new ();
    index_allocate (reversed, entries);

    for (int count = entries; count --; )
        index_append (reversed, index_get (playlist->entries, count));

    index_free (playlist->entries);
    playlist->entries = reversed;

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

void playlist_randomize (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    int entries = index_count (playlist->entries);

    for (int i = 0; i < entries; i ++)
    {
        int j = i + rand () % (entries - i);

        struct entry * entry = index_get (playlist->entries, j);
        index_set (playlist->entries, j, index_get (playlist->entries, i));
        index_set (playlist->entries, i, entry);
    }

    number_entries (playlist, 0, entries);
    queue_update (PLAYLIST_UPDATE_STRUCTURE, playlist->number, 0, entries);
    LEAVE;
}

enum {
 COMPARE_TYPE_FILENAME,
 COMPARE_TYPE_TUPLE,
 COMPARE_TYPE_TITLE};

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
            index_append (selected, entry);
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
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_FILENAME, (CompareFunc) compare};
    sort (playlist, & data);

    LEAVE;
}

void playlist_sort_by_tuple (int playlist_num, int (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_TUPLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, & data);

    LEAVE;
}

void playlist_sort_by_title (int playlist_num, int (* compare) (const char *
 a, const char * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_TITLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_filename (int playlist_num, int (* compare)
 (const char * a, const char * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_FILENAME, (CompareFunc) compare};
    sort_selected (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_tuple (int playlist_num, int (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_TUPLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, & data);

    LEAVE;
}

void playlist_sort_selected_by_title (int playlist_num, int (* compare)
 (const char * a, const char * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    CompareData data = {COMPARE_TYPE_TITLE, (CompareFunc) compare};
    if (entries_are_scanned (playlist, TRUE))
        sort (playlist, & data);

    LEAVE;
}

void playlist_reformat_titles (void)
{
    ENTER;

    g_free (title_format);
    title_format = NULL;

    for (int playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        int entries = index_count (playlist->entries);

        for (int count = 0; count < entries; count++)
        {
            Entry * entry = index_get (playlist->entries, count);
            str_unref (entry->formatted);
            entry->formatted = entry->tuple ? title_from_tuple (entry->tuple) : NULL;
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

    scan_trigger ();

    LEAVE;
}

static void playlist_rescan_real (int playlist_num, bool_t selected)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    int entries = index_count (playlist->entries);

    for (int count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (! selected || entry->selected)
        {
            entry_set_tuple (playlist, entry, NULL);
            entry->failed = FALSE;
        }
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
                entry->failed = FALSE;

                queue_update (PLAYLIST_UPDATE_METADATA, playlist_num, entry_num, 1);
            }
        }
    }

    LEAVE;
}

int64_t playlist_get_total_length (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    int64_t length = playlist->total_length;

    LEAVE_RET (length);
}

int64_t playlist_get_selected_length (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    int64_t length = playlist->selected_length;

    LEAVE_RET (length);
}

int playlist_queue_count (int playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    int count = g_list_length (playlist->queued);

    LEAVE_RET (count);
}

void playlist_queue_insert (int playlist_num, int at, int entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY;

    if (entry->queued)
        LEAVE_RET_VOID;

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
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    GList * node = g_list_nth (playlist->queued, at);
    int entry_num = node ? ((Entry *) node->data)->number : -1;

    LEAVE_RET (entry_num);
}

int playlist_queue_find_entry (int playlist_num, int entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    int pos = entry->queued ? g_list_index (playlist->queued, entry) : -1;

    LEAVE_RET (pos);
}

void playlist_queue_delete (int playlist_num, int at, int number)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

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
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    if (get_bool (NULL, "shuffle"))
    {
        if (! shuffle_prev (playlist))
            LEAVE_RET (FALSE);
    }
    else
    {
        if (! playlist->position || playlist->position->number == 0)
            LEAVE_RET (FALSE);

        set_position (playlist, index_get (playlist->entries,
         playlist->position->number - 1), TRUE);
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
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

bool_t playlist_next_song (int playlist_num, bool_t repeat)
{
    /* stop playback before locking playlists */
    if (playback_get_playing () && playlist_num == playlist_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    int entries = index_count(playlist->entries);

    if (! entries)
        LEAVE_RET (FALSE);

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
                LEAVE_RET (FALSE);

            shuffle_reset (playlist);

            if (! shuffle_next (playlist))
                LEAVE_RET (FALSE);
        }
    }
    else
    {
        if (! playlist->position)
            set_position (playlist, index_get (playlist->entries, 0), TRUE);
        else if (playlist->position->number == entries - 1)
        {
            if (! repeat)
                LEAVE_RET (FALSE);

            set_position (playlist, index_get (playlist->entries, 0), TRUE);
        }
        else
            set_position (playlist, index_get (playlist->entries,
             playlist->position->number + 1), TRUE);
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

int playback_entry_get_position (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, FALSE);
    int entry_num = entry ? entry->number : -1;

    LEAVE_RET (entry_num);
}

PluginHandle * playback_entry_get_decoder (void)
{
    ENTER;

    Entry * entry = get_playback_entry (TRUE, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    LEAVE_RET (decoder);
}

Tuple * playback_entry_get_tuple (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    Tuple * tuple = entry ? entry->tuple : NULL;

    if (tuple)
        tuple_ref (tuple);

    LEAVE_RET (tuple);
}

char * playback_entry_get_title (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    char * title = entry ? str_ref (entry->formatted ? entry->formatted : entry->title) : NULL;

    LEAVE_RET (title);
}

int playback_entry_get_length (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    int length = entry->length;

    LEAVE_RET (length);
}

void playback_entry_set_tuple (Tuple * tuple)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        LEAVE_RET_VOID;

    Entry * entry = playing_playlist->position;
    entry_cancel_scan (entry);
    entry_set_tuple (playing_playlist, entry, tuple);

    queue_update (PLAYLIST_UPDATE_METADATA, playing_playlist->number, entry->number, 1);
    LEAVE;
}

int playback_entry_get_start_time (void)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        LEAVE_RET (0);

    int start = playing_playlist->position->start;
    LEAVE_RET (start);
}

int playback_entry_get_end_time (void)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        LEAVE_RET (-1);

    int end = playing_playlist->position->end;
    LEAVE_RET (end);
}

void playlist_save_state (void)
{
    /* get playback state before locking playlists */
    int state, time;
    get_resume_state (& state, & time);

    ENTER;

    char * path = g_strdup_printf ("%s/" STATE_FILE, get_path (AUD_PATH_USER_DIR));
    FILE * handle = fopen (path, "w");
    g_free (path);
    if (! handle)
        LEAVE_RET_VOID;

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
            playlist->resume_state = state;
            playlist->resume_time = time;
        }

        fprintf (handle, "resume-state %d\n", playlist->resume_state);
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

    char * path = g_strdup_printf ("%s/" STATE_FILE, get_path (AUD_PATH_USER_DIR));
    FILE * handle = fopen (path, "r");
    g_free (path);
    if (! handle)
        LEAVE_RET_VOID;

    parse_next (handle);

    if (parse_integer ("active", & playlist_num))
    {
        if (! (active_playlist = lookup_playlist (playlist_num)))
            active_playlist = index_get (playlists, 0);
        parse_next (handle);
    }

    if (parse_integer ("playing", & playlist_num))
    {
        playing_playlist = lookup_playlist (playlist_num);
        parse_next (handle);
    }

    while (parse_integer ("playlist", & playlist_num) && playlist_num >= 0 &&
     playlist_num < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        int entries = index_count (playlist->entries), position;
        char * s;

        parse_next (handle);

        if ((s = parse_string ("filename")))
        {
            str_unref (playlist->filename);
            playlist->filename = s;
            parse_next (handle);
        }

        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            set_position (playlist, index_get (playlist->entries, position), TRUE);

        if (parse_integer ("resume-state", & playlist->resume_state))
            parse_next (handle);
        if (parse_integer ("resume-time", & playlist->resume_time))
            parse_next (handle);
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
    int state = RESUME_STOP, time = 0;

    ENTER;

    if (playing_playlist)
    {
        state = playing_playlist->resume_state;
        time = playing_playlist->resume_time;
    }

    LEAVE;

    /* resume playback after unlocking playlists */
    if (state == RESUME_PLAY && ! get_bool (NULL, "resume_playback_on_startup"))
        state = RESUME_PAUSE;

    resume (state, time);
}
