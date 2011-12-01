/*
 * playlist-new.c
 * Copyright 2009-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/tuple_formatter.h>

#include "config.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "plugins.h"
#include "util.h"

enum {RESUME_STOP, RESUME_PLAY, RESUME_PAUSE};

#define SCAN_THREADS 4
#define STATE_FILE "playlist-state"

#define ENTER g_mutex_lock (mutex)
#define LEAVE g_mutex_unlock (mutex)

#define LEAVE_RET_VOID do { \
    g_mutex_unlock (mutex); \
    return; \
} while (0)

#define LEAVE_RET(ret) do { \
    g_mutex_unlock (mutex); \
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

#define SELECTION_HAS_CHANGED(p, a, c) \
 queue_update (PLAYLIST_UPDATE_SELECTION, p, a, c)

#define METADATA_HAS_CHANGED(p, a, c) \
 queue_update (PLAYLIST_UPDATE_METADATA, p, a, c)

#define PLAYLIST_HAS_CHANGED(p, a, c) \
 queue_update (PLAYLIST_UPDATE_STRUCTURE, p, a, c)

typedef struct {
    gint level, before, after;
} Update;

typedef struct {
    gint number;
    gchar * filename;
    PluginHandle * decoder;
    Tuple * tuple;
    gchar * formatted, * title, * artist, * album;
    gint length;
    gboolean failed;
    gboolean selected;
    gint shuffle_num;
    gboolean queued;
    gboolean segmented;
    gint start, end;
} Entry;

typedef struct {
    gint number, unique_id;
    gchar * filename, * title;
    gboolean modified;
    struct index * entries;
    Entry * position;
    gint selected_count;
    gint last_shuffle_num;
    GList * queued;
    gint64 total_length, selected_length;
    gboolean scanning, scan_ending;
    Update next_update, last_update;
} Playlist;

static const gchar default_title[] = N_("New Playlist");
static const gchar temp_title[] = N_("Now Playing");

static GMutex * mutex;
static GCond * cond;

/* The unique ID table contains pointers to Playlist for ID's in use and NULL
 * for "dead" (previously used and therefore unavailable) ID's. */
static GHashTable * unique_id_table = NULL;
static gint next_unique_id = 1000;

static struct index * playlists = NULL;
static Playlist * active_playlist = NULL;
static Playlist * playing_playlist = NULL;

static gint update_source = 0, update_level;
static gint resume_state, resume_time;

typedef struct {
    Playlist * playlist;
    Entry * entry;
} ScanItem;

static GThread * scan_threads[SCAN_THREADS];
static gboolean scan_quit;
static gint scan_playlist, scan_row;
static GQueue scan_queue = G_QUEUE_INIT;
static ScanItem * scan_items[SCAN_THREADS];

static void * scanner (void * unused);
static void scan_trigger (void);

static gchar * title_from_tuple (Tuple * tuple)
{
    gchar * generic = get_string (NULL, "generic_title_format");
    gchar * title = tuple_formatter_make_title_string (tuple, generic);
    g_free (generic);
    return title;
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

    g_free (entry->formatted);
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

            if (tuple_get_value_type (tuple, FIELD_SEGMENT_END, NULL) ==
             TUPLE_INT)
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

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_items[i] && scan_items[i]->entry == entry)
        {
            g_slice_free (ScanItem, scan_items[i]);
            scan_items[i] = NULL;
        }
    }
}

static Entry * entry_new (gchar * filename, Tuple * tuple,
 PluginHandle * decoder)
{
    Entry * entry = g_malloc (sizeof (Entry));

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

    g_free (entry->filename);
    if (entry->tuple)
        tuple_unref (entry->tuple);

    g_free (entry->formatted);
    str_unref (entry->title);
    str_unref (entry->artist);
    str_unref (entry->album);
    g_free (entry);
}

static gint new_unique_id (gint preferred)
{
    if (preferred >= 0 && ! g_hash_table_lookup_extended (unique_id_table,
     GINT_TO_POINTER (preferred), NULL, NULL))
        return preferred;

    while (g_hash_table_lookup_extended (unique_id_table,
     GINT_TO_POINTER (next_unique_id), NULL, NULL))
        next_unique_id ++;

    return next_unique_id ++;
}

static Playlist * playlist_new (gint id)
{
    Playlist * playlist = g_malloc (sizeof (Playlist));

    playlist->number = -1;
    playlist->unique_id = new_unique_id (id);
    playlist->filename = NULL;
    playlist->title = g_strdup(_(default_title));
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

    memset (& playlist->last_update, 0, sizeof (Update));
    memset (& playlist->next_update, 0, sizeof (Update));

    g_hash_table_insert (unique_id_table, GINT_TO_POINTER (playlist->unique_id), playlist);
    return playlist;
}

static void playlist_free (Playlist * playlist)
{
    g_hash_table_insert (unique_id_table, GINT_TO_POINTER (playlist->unique_id), NULL);

    g_free (playlist->filename);
    g_free (playlist->title);

    for (gint count = 0; count < index_count (playlist->entries); count ++)
        entry_free (index_get (playlist->entries, count));

    index_free (playlist->entries);
    g_list_free (playlist->queued);
    g_free (playlist);
}

static void number_playlists (gint at, gint length)
{
    for (gint count = 0; count < length; count ++)
    {
        Playlist * playlist = index_get (playlists, at + count);
        playlist->number = at + count;
    }
}

static Playlist * lookup_playlist (gint playlist_num)
{
    return (playlists && playlist_num >= 0 && playlist_num < index_count
     (playlists)) ? index_get (playlists, playlist_num) : NULL;
}

static void number_entries (Playlist * playlist, gint at, gint length)
{
    for (gint count = 0; count < length; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        entry->number = at + count;
    }
}

static Entry * lookup_entry (Playlist * playlist, gint entry_num)
{
    return (entry_num >= 0 && entry_num < index_count (playlist->entries)) ?
     index_get (playlist->entries, entry_num) : NULL;
}

static gboolean update (void * unused)
{
    ENTER;

    for (gint i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        memcpy (& p->last_update, & p->next_update, sizeof (Update));
        memset (& p->next_update, 0, sizeof (Update));
    }

    gint level = update_level;
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

static void queue_update (gint level, gint list, gint at, gint count)
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
        update_source = g_idle_add_full (G_PRIORITY_HIGH_IDLE, update, NULL, NULL);
}

gboolean playlist_update_pending (void)
{
    ENTER;
    gboolean pending = update_level ? TRUE : FALSE;
    LEAVE_RET (pending);
}

gint playlist_updated_range (gint playlist_num, gint * at, gint * count)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    Update * u = & playlist->last_update;

    gint level = u->level;
    * at = u->before;
    * count = index_count (playlist->entries) - u->before - u->after;

    LEAVE_RET (level);
}

gboolean playlist_scan_in_progress (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    gboolean scanning = playlist->scanning || playlist->scan_ending;

    LEAVE_RET (scanning);
}

static gboolean entry_scan_is_queued (Entry * entry)
{
    for (GList * node = scan_queue.head; node; node = node->next)
    {
        ScanItem * item = node->data;
        if (item->entry == entry)
            return TRUE;
    }

    for (gint i = 0; i < SCAN_THREADS; i ++)
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

    g_cond_broadcast (cond);
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

    for (gint i = 0; i < SCAN_THREADS; i ++)
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

    gint i = GPOINTER_TO_INT (data);

    while (! scan_quit)
    {
        if (! scan_items[i])
            scan_items[i] = entry_find_to_scan ();

        if (! scan_items[i])
        {
            g_cond_wait (cond, mutex);
            continue;
        }

        Playlist * playlist = scan_items[i]->playlist;
        Entry * entry = scan_items[i]->entry;
        gchar * filename = g_strdup (entry->filename);
        PluginHandle * decoder = entry->decoder;
        gboolean need_tuple = entry->tuple ? FALSE : TRUE;

        LEAVE;

        if (! decoder)
            decoder = file_find_decoder (filename, FALSE);

        Tuple * tuple = (need_tuple && decoder) ? file_read_tuple (filename, decoder) : NULL;

        ENTER;

        g_free (filename);

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
        else if (need_tuple)
            entry_set_failed (playlist, entry);

        g_slice_free (ScanItem, scan_items[i]);
        scan_items[i] = NULL;

        g_cond_broadcast (cond);
        check_scan_complete (playlist);
    }

    LEAVE_RET (NULL);
}

static void scan_trigger (void)
{
    scan_playlist = 0;
    scan_row = 0;
    g_cond_broadcast (cond);
}

/* mutex may be unlocked during the call */
static Entry * get_entry (gint playlist_num, gint entry_num,
 gboolean need_decoder, gboolean need_tuple)
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
            g_cond_wait (cond, mutex);
            continue;
        }

        return entry;
    }
}

/* mutex may be unlocked during the call */
static Entry * get_playback_entry (gboolean need_decoder, gboolean need_tuple)
{
    while (1)
    {
        Entry * entry = playing_playlist ? playing_playlist->position : NULL;

        if (! entry || entry->failed)
            return entry;

        if ((need_decoder && ! entry->decoder) || (need_tuple && ! entry->tuple))
        {
            entry_queue_scan (playing_playlist, entry);
            g_cond_wait (cond, mutex);
            continue;
        }

        return entry;
    }
}

void playlist_init (void)
{
    srand (time (NULL));

    mutex = g_mutex_new ();
    cond = g_cond_new ();

    ENTER;

    unique_id_table = g_hash_table_new (g_direct_hash, g_direct_equal);
    playlists = index_new ();

    update_level = 0;

    scan_quit = FALSE;
    scan_playlist = scan_row = 0;

    for (gint i = 0; i < SCAN_THREADS; i ++)
        scan_threads[i] = g_thread_create (scanner, GINT_TO_POINTER (i), TRUE, NULL);

    LEAVE;
}

void playlist_end (void)
{
    ENTER;

    scan_quit = TRUE;
    g_cond_broadcast (cond);

    LEAVE;

    for (gint i = 0; i < SCAN_THREADS; i ++)
        g_thread_join (scan_threads[i]);

    ENTER;

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    active_playlist = playing_playlist = NULL;

    for (gint i = 0; i < index_count (playlists); i ++)
        playlist_free (index_get (playlists, i));

    index_free (playlists);
    playlists = NULL;

    g_hash_table_destroy (unique_id_table);
    unique_id_table = NULL;

    LEAVE;

    g_mutex_free (mutex);
    g_cond_free (cond);
}

gint playlist_count (void)
{
    ENTER;
    gint count = index_count (playlists);
    LEAVE_RET (count);
}

void playlist_insert_with_id (gint at, gint id)
{
    ENTER;

    if (at < 0 || at > index_count (playlists))
        at = index_count (playlists);

    index_insert (playlists, at, playlist_new (id));
    number_playlists (at, index_count (playlists) - at);

    PLAYLIST_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

void playlist_insert (gint at)
{
    playlist_insert_with_id (at, -1);
}

void playlist_reorder (gint from, gint to, gint count)
{
    ENTER;
    if (from < 0 || from + count > index_count (playlists) || to < 0 || to +
     count > index_count (playlists) || count < 0)
        LEAVE_RET_VOID;

    struct index * displaced = index_new ();

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

    PLAYLIST_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

void playlist_delete (gint playlist_num)
{
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
        active_playlist = index_get (playlists, MIN (playlist_num, index_count
         (playlists) - 1));
    if (playlist == playing_playlist)
        playing_playlist = NULL;

    PLAYLIST_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

gint playlist_get_unique_id (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    gint unique_id = playlist->unique_id;

    LEAVE_RET (unique_id);
}

gint playlist_by_unique_id (gint id)
{
    ENTER;

    Playlist * p = g_hash_table_lookup (unique_id_table, GINT_TO_POINTER (id));
    gint num = p ? p->number : -1;

    LEAVE_RET (num);
}

void playlist_set_filename (gint playlist_num, const gchar * filename)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    g_free (playlist->filename);
    playlist->filename = g_strdup (filename);
    playlist->modified = TRUE;

    METADATA_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

gchar * playlist_get_filename (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (NULL);

    gchar * filename = g_strdup (playlist->filename);

    LEAVE_RET (filename);
}

void playlist_set_title (gint playlist_num, const gchar * title)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    g_free (playlist->title);
    playlist->title = g_strdup (title);
    playlist->modified = TRUE;

    METADATA_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

gchar * playlist_get_title (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (NULL);

    gchar * title = g_strdup (playlist->title);

    LEAVE_RET (title);
}

void playlist_set_modified (gint playlist_num, gboolean modified)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    playlist->modified = modified;

    LEAVE;
}

gboolean playlist_get_modified (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    gboolean modified = playlist->modified;

    LEAVE_RET (modified);
}

void playlist_set_active (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gboolean changed = FALSE;

    if (playlist != active_playlist)
    {
        changed = TRUE;
        active_playlist = playlist;
    }

    LEAVE;

    if (changed)
        hook_call ("playlist activate", NULL);
}

gint playlist_get_active (void)
{
    ENTER;
    gint list = active_playlist ? active_playlist->number : -1;
    LEAVE_RET (list);
}

void playlist_set_playing (gint playlist_num)
{
    if (playback_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;

    if (playlist_num < 0)
        playlist = NULL;
    else
        LOOKUP_PLAYLIST;

    playing_playlist = playlist;

    LEAVE;

    hook_call ("playlist set playing", NULL);
}

gint playlist_get_playing (void)
{
    ENTER;
    gint list = playing_playlist ? playing_playlist->number: -1;
    LEAVE_RET (list);
}

gint playlist_get_temporary (void)
{
    gint temp = -1;
    gboolean need_rename = TRUE;

    ENTER;

    for (gint i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);

        if (! strcmp (p->title, _(temp_title)))
        {
            temp = p->number;
            need_rename = FALSE;
            break;
        }
    }

    if (temp < 0 && active_playlist && ! strcmp (active_playlist->title,
     _(default_title)) && index_count (active_playlist->entries) == 0)
        temp = active_playlist->number;

    LEAVE;

    if (temp < 0)
    {
        temp = playlist_count ();
        playlist_insert (temp);
    }

    if (need_rename)
        playlist_set_title (temp, _(temp_title));

    return temp;
}

/* If we are already at the song or it is already at the top of the shuffle
 * list, we let it be.  Otherwise, we move it to the top. */
static void set_position (Playlist * playlist, Entry * entry)
{
    if (entry == playlist->position)
        return;

    playlist->position = entry;

    if (! entry)
        return;

    if (! entry->shuffle_num || entry->shuffle_num != playlist->last_shuffle_num)
    {
        playlist->last_shuffle_num ++;
        entry->shuffle_num = playlist->last_shuffle_num;
    }
}

gint playlist_entry_count (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    gint count = index_count (playlist->entries);

    LEAVE_RET (count);
}

void playlist_entry_insert_batch_raw (gint playlist_num, gint at,
 struct index * filenames, struct index * tuples, struct index * decoders)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;

    gint number = index_count (filenames);

    struct index * add = index_new ();
    index_allocate (add, number);

    for (gint i = 0; i < number; i ++)
    {
        gchar * filename = index_get (filenames, i);
        uri_check_utf8 (& filename, TRUE);
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

    for (gint count = 0; count < number; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        playlist->total_length += entry->length;
    }

    PLAYLIST_HAS_CHANGED (playlist->number, at, number);
    LEAVE;
}

void playlist_entry_delete (gint playlist_num, gint at, gint number)
{
    if (playback_get_playing () && playlist_num == playlist_get_playing () &&
     playlist_get_position (playlist_num) >= at && playlist_get_position
     (playlist_num) < at + number)
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    if (playlist->position && playlist->position->number >= at &&
     playlist->position->number < at + number)
        set_position (playlist, NULL);

    for (gint count = 0; count < number; count ++)
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

    PLAYLIST_HAS_CHANGED (playlist->number, at, 0);
    LEAVE;
}

gchar * playlist_entry_get_filename (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    gchar * filename = g_strdup (entry->filename);

    LEAVE_RET (filename);
}

PluginHandle * playlist_entry_get_decoder (gint playlist_num, gint entry_num, gboolean fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, ! fast, FALSE);
    PluginHandle * decoder = entry ? entry->decoder : NULL;

    LEAVE_RET (decoder);
}

Tuple * playlist_entry_get_tuple (gint playlist_num, gint entry_num, gboolean fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    Tuple * tuple = entry ? entry->tuple : NULL;

    if (tuple)
        tuple_ref (tuple);

    LEAVE_RET (tuple);
}

gchar * playlist_entry_get_title (gint playlist_num, gint entry_num, gboolean fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    gchar * title = entry ? g_strdup (entry->formatted ? entry->formatted : entry->filename) : NULL;

    LEAVE_RET (title);
}

void playlist_entry_describe (gint playlist_num, gint entry_num,
 gchar * * title, gchar * * artist, gchar * * album, gboolean fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    * title = (entry && entry->title) ? g_strdup (entry->title) : NULL;
    * artist = (entry && entry->artist) ? g_strdup (entry->artist) : NULL;
    * album = (entry && entry->album) ? g_strdup (entry->album) : NULL;

    LEAVE;
}

gint playlist_entry_get_length (gint playlist_num, gint entry_num, gboolean fast)
{
    ENTER;

    Entry * entry = get_entry (playlist_num, entry_num, FALSE, ! fast);
    gint length = entry ? entry->length : 0;

    LEAVE_RET (length);
}

void playlist_set_position (gint playlist_num, gint entry_num)
{
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

    set_position (playlist, entry);
    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
}

gint playlist_get_position (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    gint position = playlist->position ? playlist->position->number : -1;

    LEAVE_RET (position);
}

void playlist_entry_set_selected (gint playlist_num, gint entry_num,
 gboolean selected)
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

    SELECTION_HAS_CHANGED (playlist->number, entry_num, 1);
    LEAVE;
}

gboolean playlist_entry_get_selected (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (FALSE);

    gboolean selected = entry->selected;

    LEAVE_RET (selected);
}

gint playlist_selected_count (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    gint selected_count = playlist->selected_count;

    LEAVE_RET (selected_count);
}

void playlist_select_all (gint playlist_num, gboolean selected)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);
    gint first = entries, last = 0;

    for (gint count = 0; count < entries; count ++)
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
        SELECTION_HAS_CHANGED (playlist->number, first, last + 1 - first);

    LEAVE;
}

gint playlist_shift (gint playlist_num, gint entry_num, gint distance)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! entry->selected || ! distance)
        LEAVE_RET (0);

    gint entries = index_count (playlist->entries);
    gint shift = 0, center, top, bottom;

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

    for (gint i = 0; i < top; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (entry->selected)
            top = i;
    }

    for (gint i = entries; i > bottom; i --)
    {
        entry = index_get (playlist->entries, i - 1);
        if (entry->selected)
            bottom = i;
    }

    struct index * temp = index_new ();

    for (gint i = top; i < center; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (! entry->selected)
            index_append (temp, entry);
    }

    for (gint i = top; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (entry->selected)
            index_append (temp, entry);
    }

    for (gint i = center; i < bottom; i ++)
    {
        entry = index_get (playlist->entries, i);
        if (! entry->selected)
            index_append (temp, entry);
    }

    index_copy_set (temp, 0, playlist->entries, top, bottom - top);

    number_entries (playlist, top, bottom - top);
    PLAYLIST_HAS_CHANGED (playlist->number, top, bottom - top);

    LEAVE_RET (shift);
}

void playlist_delete_selected (gint playlist_num)
{
    if (playback_get_playing () && playlist_num == playlist_get_playing () &&
     playlist_get_position (playlist_num) >= 0 && playlist_entry_get_selected
     (playlist_num, playlist_get_position (playlist_num)))
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (! playlist->selected_count)
        LEAVE_RET_VOID;

    gint entries = index_count (playlist->entries);

    struct index * others = index_new ();
    index_allocate (others, entries - playlist->selected_count);

    if (playlist->position && playlist->position->selected)
        set_position (playlist, NULL);

    gint before = 0, after = 0;
    gboolean found = FALSE;

    for (gint count = 0; count < entries; count++)
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
    PLAYLIST_HAS_CHANGED (playlist->number, before, index_count
     (playlist->entries) - after - before);
    LEAVE;
}

void playlist_reverse (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);

    struct index * reversed = index_new ();
    index_allocate (reversed, entries);

    for (gint count = entries; count --; )
        index_append (reversed, index_get (playlist->entries, count));

    index_free (playlist->entries);
    playlist->entries = reversed;

    number_entries (playlist, 0, entries);
    PLAYLIST_HAS_CHANGED (playlist->number, 0, entries);
    LEAVE;
}

void playlist_randomize (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);

    for (gint i = 0; i < entries; i ++)
    {
        gint j = i + rand () % (entries - i);

        struct entry * entry = index_get (playlist->entries, j);
        index_set (playlist->entries, j, index_get (playlist->entries, i));
        index_set (playlist->entries, i, entry);
    }

    number_entries (playlist, 0, entries);
    PLAYLIST_HAS_CHANGED (playlist->number, 0, entries);
    LEAVE;
}

static gint filename_compare (const void * _a, const void * _b, void * _compare)
{
    const Entry * a = _a, * b = _b;
    gint (* compare) (const gchar * a, const gchar * b) = _compare;

    gint diff = compare (a->filename, b->filename);
    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static gint tuple_compare (const void * _a, const void * _b, void * _compare)
{
    const Entry * a = _a, * b = _b;
    gint (* compare) (const Tuple * a, const Tuple * b) = _compare;

    if (! a->tuple)
        return b->tuple ? -1 : 0;
    if (! b->tuple)
        return 1;

    gint diff = compare (a->tuple, b->tuple);
    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static gint title_compare (const void * _a, const void * _b, void * _compare)
{
    const Entry * a = _a, * b = _b;
    gint (* compare) (const gchar * a, const gchar * b) = _compare;

    gint diff = compare (a->formatted ? a->formatted : a->filename, b->formatted
     ? b->formatted : b->filename);
    if (diff)
        return diff;

    /* preserve order of "equal" entries */
    return a->number - b->number;
}

static void sort (Playlist * playlist, gint (* compare) (const void * a,
 const void * b, void * inner), void * inner)
{
    index_sort_with_data (playlist->entries, compare, inner);
    number_entries (playlist, 0, index_count (playlist->entries));

    PLAYLIST_HAS_CHANGED (playlist->number, 0, index_count (playlist->entries));
}

static void sort_selected (Playlist * playlist, gint (* compare) (const void *
 a, const void * b, void * inner), void * inner)
{
    gint entries = index_count (playlist->entries);

    struct index * selected = index_new ();
    index_allocate (selected, playlist->selected_count);

    for (gint count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_append (selected, entry);
    }

    index_sort_with_data (selected, compare, inner);

    gint count2 = 0;
    for (gint count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_set (playlist->entries, count, index_get (selected, count2 ++));
    }

    index_free (selected);

    number_entries (playlist, 0, entries);
    PLAYLIST_HAS_CHANGED (playlist->number, 0, entries);
}

static gboolean entries_are_scanned (Playlist * playlist, gboolean selected)
{
    gint entries = index_count (playlist->entries);
    for (gint count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (selected && ! entry->selected)
            continue;

        if (! entry->tuple)
        {
            event_queue ("interface show error", _("The playlist cannot be "
             "sorted because metadata scanning is still in progress (or has "
             "been disabled)."));
            return FALSE;
        }
    }

    return TRUE;
}

void playlist_sort_by_filename (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    sort (playlist, filename_compare, compare);

    LEAVE;
}

void playlist_sort_by_tuple (gint playlist_num, gint (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, tuple_compare, compare);

    LEAVE;
}

void playlist_sort_by_title (gint playlist_num, gint (* compare) (const gchar *
 a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (entries_are_scanned (playlist, FALSE))
        sort (playlist, title_compare, compare);

    LEAVE;
}

void playlist_sort_selected_by_filename (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    sort_selected (playlist, filename_compare, compare);

    LEAVE;
}

void playlist_sort_selected_by_tuple (gint playlist_num, gint (* compare)
 (const Tuple * a, const Tuple * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (entries_are_scanned (playlist, TRUE))
        sort_selected (playlist, tuple_compare, compare);

    LEAVE;
}

void playlist_sort_selected_by_title (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    if (entries_are_scanned (playlist, TRUE))
        sort (playlist, title_compare, compare);

    LEAVE;
}

void playlist_reformat_titles (void)
{
    ENTER;

    for (gint playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries);

        for (gint count = 0; count < entries; count++)
        {
            Entry * entry = index_get (playlist->entries, count);
            g_free (entry->formatted);
            entry->formatted = entry->tuple ? title_from_tuple (entry->tuple) :
             NULL;
        }

        METADATA_HAS_CHANGED (playlist_num, 0, entries);
    }

    LEAVE;
}

void playlist_trigger_scan (void)
{
    ENTER;

    for (gint i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        p->scanning = TRUE;
    }

    scan_trigger ();

    LEAVE;
}

static void playlist_rescan_real (gint playlist_num, gboolean selected)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);

    for (gint count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (! selected || entry->selected)
        {
            entry_set_tuple (playlist, entry, NULL);
            entry->failed = FALSE;
        }
    }

    METADATA_HAS_CHANGED (playlist->number, 0, entries);
    LEAVE;
}

void playlist_rescan (gint playlist_num)
{
    playlist_rescan_real (playlist_num, FALSE);
}

void playlist_rescan_selected (gint playlist_num)
{
    playlist_rescan_real (playlist_num, TRUE);
}

void playlist_rescan_file (const gchar * filename)
{
    ENTER;

    gint num_playlists = index_count (playlists);

    gchar * copy = NULL;
    if (! uri_is_utf8 (filename, TRUE))
        filename = copy = uri_to_utf8 (filename);

    for (gint playlist_num = 0; playlist_num < num_playlists; playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint num_entries = index_count (playlist->entries);

        for (gint entry_num = 0; entry_num < num_entries; entry_num ++)
        {
            Entry * entry = index_get (playlist->entries, entry_num);

            if (! strcmp (entry->filename, filename))
            {
                entry_set_tuple (playlist, entry, NULL);
                entry->failed = FALSE;

                METADATA_HAS_CHANGED (playlist_num, entry_num, 1);
            }
        }
    }

    g_free (copy);

    LEAVE;
}

gint64 playlist_get_total_length (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    gint64 length = playlist->total_length;

    LEAVE_RET (length);
}

gint64 playlist_get_selected_length (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    gint64 length = playlist->selected_length;

    LEAVE_RET (length);
}

gint playlist_queue_count (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    gint count = g_list_length (playlist->queued);

    LEAVE_RET (count);
}

void playlist_queue_insert (gint playlist_num, gint at, gint entry_num)
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

    SELECTION_HAS_CHANGED (playlist->number, entry_num, 1);
    LEAVE;
}

void playlist_queue_insert_selected (gint playlist_num, gint at)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count(playlist->entries);
    gint first = entries, last = 0;

    for (gint count = 0; count < entries; count++)
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
        SELECTION_HAS_CHANGED (playlist->number, first, last + 1 - first);

    LEAVE;
}

gint playlist_queue_get_entry (gint playlist_num, gint at)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);

    GList * node = g_list_nth (playlist->queued, at);
    gint entry_num = node ? ((Entry *) node->data)->number : -1;

    LEAVE_RET (entry_num);
}

gint playlist_queue_find_entry (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    gint pos = entry->queued ? g_list_index (playlist->queued, entry) : -1;

    LEAVE_RET (pos);
}

void playlist_queue_delete (gint playlist_num, gint at, gint number)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);
    gint first = entries, last = 0;

    if (at == 0)
    {
        while (playlist->queued && number --)
        {
            Entry * entry = playlist->queued->data;
            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;

            playlist->queued = g_list_delete_link (playlist->queued,
             playlist->queued);
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

            playlist->queued = g_list_delete_link (playlist->queued,
             anchor->next);
        }
    }

DONE:
    if (first < entries)
        SELECTION_HAS_CHANGED (playlist->number, first, last + 1 - first);

    LEAVE;
}

void playlist_queue_delete_selected (gint playlist_num)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);
    gint first = entries, last = 0;

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
        SELECTION_HAS_CHANGED (playlist->number, first, last + 1 - first);

    LEAVE;
}

static gboolean shuffle_prev (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    Entry * found = NULL;

    for (gint count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (entry->shuffle_num && (! playlist->position ||
         entry->shuffle_num < playlist->position->shuffle_num) && (! found
         || entry->shuffle_num > found->shuffle_num))
            found = entry;
    }

    if (! found)
        return FALSE;

    playlist->position = found;
    return TRUE;
}

gboolean playlist_prev_song (gint playlist_num)
{
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
         playlist->position->number - 1));
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

static gboolean shuffle_next (Playlist * playlist)
{
    gint entries = index_count (playlist->entries), choice = 0, count;
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
        playlist->position = found;
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
                set_position (playlist, entry);
                return TRUE;
            }

            choice --;
        }
    }
}

static void shuffle_reset (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);

    playlist->last_shuffle_num = 0;

    for (gint count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        entry->shuffle_num = 0;
    }
}

gboolean playlist_next_song (gint playlist_num, gboolean repeat)
{
    if (playback_get_playing () && playlist_num == playlist_get_playing ())
        playback_stop ();

    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (FALSE);

    gint entries = index_count(playlist->entries);

    if (! entries)
        LEAVE_RET (FALSE);

    if (playlist->queued)
    {
        set_position (playlist, playlist->queued->data);
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
            set_position (playlist, index_get (playlist->entries, 0));
        else if (playlist->position->number == entries - 1)
        {
            if (! repeat)
                LEAVE_RET (FALSE);

            set_position (playlist, index_get (playlist->entries, 0));
        }
        else
            set_position (playlist, index_get (playlist->entries,
             playlist->position->number + 1));
    }

    LEAVE;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

gint playback_entry_get_position (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, FALSE);
    gint entry_num = entry ? entry->number : -1;

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

gchar * playback_entry_get_title (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    gchar * title = entry ? g_strdup (entry->formatted ? entry->formatted : entry->filename) : NULL;

    LEAVE_RET (title);
}

gint playback_entry_get_length (void)
{
    ENTER;

    Entry * entry = get_playback_entry (FALSE, TRUE);
    gint length = entry->length;

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

    METADATA_HAS_CHANGED (playing_playlist->number, entry->number, 1);
    LEAVE;
}

gint playback_entry_get_start_time (void)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        LEAVE_RET (0);

    gint start = playing_playlist->position->start;
    LEAVE_RET (start);
}

gint playback_entry_get_end_time (void)
{
    ENTER;
    if (! playing_playlist || ! playing_playlist->position)
        LEAVE_RET (-1);

    gint end = playing_playlist->position->end;
    LEAVE_RET (end);
}

void playlist_save_state (void)
{
    ENTER;

    gchar * path = g_strdup_printf ("%s/" STATE_FILE, get_path (AUD_PATH_USER_DIR));
    FILE * handle = fopen (path, "w");
    g_free (path);
    if (! handle)
        LEAVE_RET_VOID;

    resume_state = playback_get_playing () ? (playback_get_paused () ?
     RESUME_PAUSE : RESUME_PLAY) : RESUME_STOP;
    resume_time = playback_get_playing () ? playback_get_time () : 0;

    fprintf (handle, "resume-state %d\n", resume_state);
    fprintf (handle, "resume-time %d\n", resume_time);

    fprintf (handle, "active %d\n", active_playlist ? active_playlist->number : -1);
    fprintf (handle, "playing %d\n", playing_playlist ? playing_playlist->number : -1);

    for (gint playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries);

        fprintf (handle, "playlist %d\n", playlist_num);

        if (playlist->filename)
            fprintf (handle, "filename %s\n", playlist->filename);

        fprintf (handle, "position %d\n", playlist->position ?
         playlist->position->number : -1);
        fprintf (handle, "last-shuffled %d\n", playlist->last_shuffle_num);

        for (gint count = 0; count < entries; count ++)
        {
            Entry * entry = index_get (playlist->entries, count);
            fprintf (handle, "S %d\n", entry->shuffle_num);
        }
    }

    fclose (handle);
    LEAVE;
}

static gchar parse_key[512];
static gchar * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = NULL;

    if (! fgets (parse_key, sizeof parse_key, handle))
        return;

    gchar * space = strchr (parse_key, ' ');
    if (! space)
        return;

    * space = 0;
    parse_value = space + 1;

    gchar * newline = strchr (parse_value, '\n');
    if (newline)
        * newline = 0;
}

static gboolean parse_integer (const gchar * key, gint * value)
{
    return (parse_value && ! strcmp (parse_key, key) && sscanf (parse_value,
     "%d", value) == 1);
}

static gchar * parse_string (const gchar * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? g_strdup (parse_value) :
     NULL;
}

void playlist_load_state (void)
{
    ENTER;
    gint playlist_num;

    gchar * path = g_strdup_printf ("%s/" STATE_FILE, get_path (AUD_PATH_USER_DIR));
    FILE * handle = fopen (path, "r");
    g_free (path);
    if (! handle)
        LEAVE_RET_VOID;

    parse_next (handle);

    if (parse_integer ("resume-state", & resume_state))
        parse_next (handle);
    if (parse_integer ("resume-time", & resume_time))
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
        gint entries = index_count (playlist->entries), position, count;
        gchar * s;

        parse_next (handle);

        if ((s = parse_string ("filename")))
        {
            g_free (playlist->filename);
            playlist->filename = s;
            parse_next (handle);
        }

        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            playlist->position = index_get (playlist->entries, position);

        if (parse_integer ("last-shuffled", & playlist->last_shuffle_num))
            parse_next (handle);

        for (count = 0; count < entries; count ++)
        {
            Entry * entry = index_get (playlist->entries, count);
            if (parse_integer ("S", & entry->shuffle_num))
                parse_next (handle);
        }
    }

    fclose (handle);

    /* clear updates queued during init sequence */

    for (gint i = 0; i < index_count (playlists); i ++)
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
    if (resume_state == RESUME_PLAY || resume_state == RESUME_PAUSE)
        playback_play (resume_time, resume_state == RESUME_PAUSE);
}
