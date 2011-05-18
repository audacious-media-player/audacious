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

#include <limits.h>
#include <time.h>

#include <glib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/stringpool.h>
#include <libaudcore/tuple_formatter.h>

#include "audconfig.h"
#include "config.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "playlist-utils.h"
#include "plugins.h"
#include "util.h"

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

#define METADATA_HAS_CHANGED(p, a, c) do { \
    scan_trigger (); \
    queue_update (PLAYLIST_UPDATE_METADATA, p, a, c); \
} while (0)

#define PLAYLIST_HAS_CHANGED(p, a, c) do { \
    scan_trigger (); \
    queue_update (PLAYLIST_UPDATE_STRUCTURE, p, a, c); \
} while (0)

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
    struct index * entries;
    Entry * position;
    gint selected_count;
    gint last_shuffle_num;
    GList * queued;
    gint64 total_length, selected_length;
} Playlist;

static GMutex * mutex;
static GCond * cond;

static gint next_unique_id = 1000;

static struct index * playlists = NULL;
static Playlist * active_playlist = NULL;
static Playlist * playing_playlist = NULL;

static gint update_source = 0;

struct {
    gboolean pending;
    gint level, playlist, before, after;
} next_update, last_update;

static GThread * scan_threads[SCAN_THREADS];
static gboolean scan_quit;
static Playlist * scan_playlists[SCAN_THREADS];
static Entry * scan_entries[SCAN_THREADS];
static gint scan_playlist, scan_row;

static void * scanner (void * unused);

static gchar * title_from_tuple (Tuple * tuple)
{
    const gchar * format = tuple_get_string (tuple, FIELD_FORMATTER, NULL);
    if (! format)
        format = get_gentitle_format ();

    return tuple_formatter_make_title_string (tuple, format);
}

static void entry_set_tuple_real (Entry * entry, Tuple * tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (entry->segmented && ! tuple)
        return;

    if (entry->tuple)
        tuple_free (entry->tuple);
    entry->tuple = tuple;

    g_free (entry->formatted);
    stringpool_unref (entry->title);
    stringpool_unref (entry->artist);
    stringpool_unref (entry->album);

    if (! tuple)
    {
        entry->formatted = NULL;
        entry->title = entry->artist = entry->album = NULL;
        entry->length = 0;
        entry->segmented = FALSE;
        entry->start = entry->end = -1;
    }
    else
    {
        entry->formatted = title_from_tuple (tuple);
        describe_song (entry->filename, tuple, & entry->title, & entry->artist,
         & entry->album);
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
    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_entries[i] == entry)
        {
            scan_playlists[i] = NULL;
            scan_entries[i] = NULL;
        }
    }
}

static void playlist_cancel_scan (Playlist * playlist)
{
    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_playlists[i] == playlist)
        {
            scan_playlists[i] = NULL;
            scan_entries[i] = NULL;
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
    entry->start = entry->end = -1;

    entry_set_tuple_real (entry, tuple);
    return entry;
}

static void entry_free (Entry * entry)
{
    entry_cancel_scan (entry);

    g_free (entry->filename);
    if (entry->tuple)
        tuple_free (entry->tuple);

    g_free (entry->formatted);
    stringpool_unref (entry->title);
    stringpool_unref (entry->artist);
    stringpool_unref (entry->album);
    g_free (entry);
}

static Playlist * playlist_new (void)
{
    Playlist * playlist = g_malloc (sizeof (Playlist));

    playlist->number = -1;
    playlist->unique_id = next_unique_id ++;
    playlist->filename = NULL;
    playlist->title = g_strdup(_("Untitled Playlist"));
    playlist->entries = index_new();
    playlist->position = NULL;
    playlist->selected_count = 0;
    playlist->last_shuffle_num = 0;
    playlist->queued = NULL;
    playlist->total_length = 0;
    playlist->selected_length = 0;

    return playlist;
}

static void playlist_free (Playlist * playlist)
{
    playlist_cancel_scan (playlist);

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

    if (update_source)
    {
        g_source_remove (update_source);
        update_source = 0;
    }

    memcpy (& last_update, & next_update, sizeof last_update);
    memset (& next_update, 0, sizeof next_update);

    LEAVE;

    hook_call ("playlist update", GINT_TO_POINTER (last_update.level));
    return FALSE;
}

static void queue_update (gint level, gint list, gint at, gint count)
{
    Playlist * playlist = lookup_playlist (list);

    if (next_update.pending)
    {
        next_update.level = MAX (next_update.level, level);

        if (playlist && list == next_update.playlist)
        {
            next_update.before = MIN (next_update.before, at);
            next_update.after = MIN (next_update.after, index_count
             (playlist->entries) - at - count);
        }
        else
        {
            next_update.playlist = -1;
            next_update.before = 0;
            next_update.after = 0;
        }
    }
    else
    {
        next_update.pending = TRUE;
        next_update.level = level;
        next_update.playlist = list;
        next_update.before = playlist ? at : 0;
        next_update.after = playlist ? index_count (playlist->entries)
         - at - count: 0;
    }

    if (! update_source)
        update_source = g_idle_add_full (G_PRIORITY_HIGH_IDLE, update, NULL,
         NULL);
}

gboolean playlist_update_pending (void)
{
    ENTER;
    gboolean pending = next_update.pending;
    LEAVE_RET (pending);
}

gboolean playlist_update_range (gint * playlist_num, gint * at, gint * count)
{
    ENTER;

    if (! last_update.pending)
        LEAVE_RET (FALSE);

    Playlist * playlist = lookup_playlist (last_update.playlist);
    if (! playlist)
        LEAVE_RET (FALSE);

    * playlist_num = last_update.playlist;
    * at = last_update.before;
    * count = index_count (playlist->entries) - last_update.before -
     last_update.after;
    LEAVE_RET (TRUE);
}

static gboolean entry_scan_in_progress (Entry * entry)
{
    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_entries[i] == entry)
            return TRUE;
    }

    return FALSE;
}

static gboolean entry_find_to_scan (gint i)
{
    while (scan_playlist < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, scan_playlist);

        if (scan_row < index_count (playlist->entries))
        {
            Entry * entry = index_get (playlist->entries, scan_row);

            if (! entry->tuple && ! entry_scan_in_progress (entry))
            {
                scan_playlists[i] = playlist;
                scan_entries[i] = entry;
                return TRUE;
            }

            scan_row ++;
        }
        else
        {
            /* scan the active playlist first, then all the others */
            if (scan_playlist == active_playlist->number)
                scan_playlist = 0;
            else
                scan_playlist ++;

            if (scan_playlist == active_playlist->number)
                scan_playlist ++;

            scan_row = 0;
        }
    }

    return FALSE;
}

static void * scanner (void * data)
{
    ENTER;
    g_cond_broadcast (cond);

    gint i = GPOINTER_TO_INT (data);

    while (! scan_quit)
    {
        if (! entry_find_to_scan (i))
        {
            g_cond_wait (cond, mutex);
            continue;
        }

        gchar * filename = g_strdup (scan_entries[i]->filename);
        PluginHandle * decoder = scan_entries[i]->decoder;

        LEAVE;

        if (! decoder)
            decoder = file_find_decoder (filename, FALSE);

        Tuple * tuple = decoder ? file_read_tuple (filename, decoder) : NULL;

        ENTER;

        g_free (filename);

        if (! scan_entries[i]) /* entry deleted */
        {
            if (tuple)
                tuple_free (tuple);
            continue;
        }

        scan_entries[i]->decoder = decoder;

        if (tuple)
            entry_set_tuple (scan_playlists[i], scan_entries[i], tuple);
        else
            entry_set_failed (scan_playlists[i], scan_entries[i]);

        queue_update (PLAYLIST_UPDATE_METADATA, scan_playlists[i]->number,
         scan_entries[i]->number, 1);

        scan_playlists[i] = NULL;
        scan_entries[i] = NULL;

        g_cond_broadcast (cond);
    }

    LEAVE_RET (NULL);
}

static void scan_trigger (void)
{
    scan_playlist = active_playlist->number;
    scan_row = 0;
    g_cond_broadcast (cond);
}

static void check_scanned (Playlist * playlist, Entry * entry)
{
    if (entry->tuple)
        return;

    while (entry_scan_in_progress (entry))
        g_cond_wait (cond, mutex);

    if (entry->tuple)
        return;

    if (! entry->decoder)
        entry->decoder = file_find_decoder (entry->filename, FALSE);

    entry_set_tuple (playlist, entry, entry->decoder ? file_read_tuple
     (entry->filename, entry->decoder) : NULL);

    if (! entry->tuple)
        entry_set_failed (playlist, entry);

    queue_update (PLAYLIST_UPDATE_METADATA, playlist->number, entry->number, 1);
}

static void check_selected_scanned (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    for (gint count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            check_scanned (playlist, entry);
    }
}

static void check_all_scanned (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    for (gint count = 0; count < entries; count ++)
        check_scanned (playlist, index_get (playlist->entries, count));
}

void playlist_init (void)
{
    srand (time (NULL));

    mutex = g_mutex_new ();
    cond = g_cond_new ();

    ENTER;

    playlists = index_new ();
    Playlist * playlist = playlist_new ();
    index_append (playlists, playlist);
    playlist->number = 0;
    active_playlist = playlist;

    memset (& last_update, 0, sizeof last_update);
    memset (& next_update, 0, sizeof next_update);

    scan_quit = FALSE;
    memset (scan_playlists, 0, sizeof scan_playlists);
    memset (scan_entries, 0, sizeof scan_entries);
    scan_playlist = scan_row = 0;

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        scan_threads[i] = g_thread_create (scanner, GINT_TO_POINTER (i), TRUE,
         NULL);
        g_cond_wait (cond, mutex);
    }

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

void playlist_insert (gint at)
{
    ENTER;

    if (at < 0 || at > index_count (playlists))
        at = index_count (playlists);

    index_insert (playlists, at, playlist_new ());
    number_playlists (at, index_count (playlists) - at);

    PLAYLIST_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

void playlist_reorder (gint from, gint to, gint count)
{
    ENTER;
    if (from < 0 || from + count >= index_count (playlists) || to < 0 || to +
     count >= index_count (playlists) || count < 0)
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
        index_insert (playlists, 0, playlist_new ());

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

    for (gint i = 0; i < index_count (playlists); i ++)
    {
        Playlist * p = index_get (playlists, i);
        if (p->unique_id == id)
            LEAVE_RET (p->number);
    }

    LEAVE_RET (-1);
}

void playlist_set_filename (gint playlist_num, const gchar * filename)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    g_free (playlist->filename);
    playlist->filename = g_strdup (filename);

    METADATA_HAS_CHANGED (playlist_num, 0, 0);
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

    METADATA_HAS_CHANGED (playlist_num, 0, 0);
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
    gint list = active_playlist->number;
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
}

gint playlist_get_playing (void)
{
    ENTER;
    gint list = playing_playlist ? playing_playlist->number: -1;
    LEAVE_RET (list);
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

PluginHandle * playlist_entry_get_decoder (gint playlist_num, gint entry_num,
 gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! entry->decoder && ! fast)
    {
        while (entry_scan_in_progress (entry))
            g_cond_wait (cond, mutex);

        if (! entry->decoder)
            entry->decoder = file_find_decoder (entry->filename, FALSE);
    }

    PluginHandle * decoder = entry->decoder;

    LEAVE_RET (decoder);
}

void playlist_entry_set_tuple (gint playlist_num, gint entry_num, Tuple * tuple)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY;

    while (entry_scan_in_progress (entry))
        g_cond_wait (cond, mutex);

    entry_set_tuple (playlist, entry, tuple);

    METADATA_HAS_CHANGED (playlist->number, entry_num, 1);
    LEAVE;
}

Tuple * playlist_entry_get_tuple (gint playlist_num, gint entry_num,
 gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! fast)
        check_scanned (playlist, entry);

    Tuple * tuple = entry->tuple;
    if (tuple)
        mowgli_object_ref (tuple);

    LEAVE_RET (tuple);
}

gchar * playlist_entry_get_title (gint playlist_num, gint entry_num,
 gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! fast)
        check_scanned (playlist, entry);

    gchar * title = g_strdup (entry->formatted ? entry->formatted :
     entry->filename);

    LEAVE_RET (title);
}

void playlist_entry_describe (gint playlist_num, gint entry_num,
 gchar * * title, gchar * * artist, gchar * * album, gboolean fast)
{
    * title = * artist = * album = NULL;

    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY;

    if (! fast)
        check_scanned (playlist, entry);

    if (entry->title)
    {
        * title = g_strdup (entry->title);
        * artist = g_strdup (entry->artist);
        * album = g_strdup (entry->album);
    }
    else
        * title = g_strdup (entry->filename);

    LEAVE;
}

gint playlist_entry_get_length (gint playlist_num, gint entry_num, gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! fast)
        check_scanned (playlist, entry);

    gint length = entry->length;

    LEAVE_RET (length);
}

gboolean playlist_entry_is_segmented (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (FALSE);

    gboolean segmented = entry->segmented;

    LEAVE_RET (segmented);
}

gint playlist_entry_get_start_time (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    gint start = entry->start;

    LEAVE_RET (start);
}

gint playlist_entry_get_end_time (gint playlist_num, gint entry_num)
{
    ENTER;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    gint end = entry->end;

    LEAVE_RET (end);
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
        LEAVE;

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

    check_all_scanned (playlist);
    sort (playlist, tuple_compare, compare);

    LEAVE;
}

void playlist_sort_by_title (gint playlist_num, gint (* compare) (const gchar *
 a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    check_all_scanned (playlist);
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

    check_selected_scanned (playlist);
    sort_selected (playlist, tuple_compare, compare);

    LEAVE;
}

void playlist_sort_selected_by_title (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    check_selected_scanned (playlist);
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
    }

    METADATA_HAS_CHANGED (-1, 0, 0);
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
            }
        }
    }

    g_free (copy);

    METADATA_HAS_CHANGED (-1, 0, 0);
    LEAVE;
}

gint64 playlist_get_total_length (gint playlist_num, gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    if (! fast)
        check_all_scanned (playlist);

    gint64 length = playlist->total_length;

    LEAVE_RET (length);
}

gint64 playlist_get_selected_length (gint playlist_num, gboolean fast)
{
    ENTER;
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    if (! fast)
        check_selected_scanned (playlist);

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

    if (cfg.shuffle)
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
    else if (cfg.shuffle)
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

void playlist_save_state (void)
{
    ENTER;

    gchar scratch[PATH_MAX];
    snprintf (scratch, sizeof scratch, "%s/" STATE_FILE,
     get_path (AUD_PATH_USER_DIR));

    FILE * handle = fopen (scratch, "w");
    if (! handle)
        return;

    fprintf (handle, "active %d\n", active_playlist->number);
    fprintf (handle, "playing %d\n", playing_playlist ? playing_playlist->number
     : -1);

    for (gint playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries);

        fprintf (handle, "playlist %d\n", playlist_num);

        if (playlist->title)
            fprintf (handle, "title %s\n", playlist->title);
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

    gchar scratch[PATH_MAX];
    snprintf (scratch, sizeof scratch, "%s/" STATE_FILE,
     get_path (AUD_PATH_USER_DIR));

    FILE * handle = fopen (scratch, "r");
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
        gint entries = index_count (playlist->entries), position, count;
        gchar * s;

        parse_next (handle);

        if ((s = parse_string ("title")))
        {
            g_free (playlist->title);
            playlist->title = s;
            parse_next (handle);
        }

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
    LEAVE;
}
