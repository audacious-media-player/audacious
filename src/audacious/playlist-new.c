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

#include <assert.h>
#include <stdlib.h>
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

#define DECLARE_PLAYLIST \
    Playlist * playlist

#define DECLARE_PLAYLIST_ENTRY \
    Playlist * playlist; \
    Entry * entry

#define LOOKUP_PLAYLIST do { \
    playlist = lookup_playlist (playlist_num); \
    g_return_if_fail (playlist != NULL); \
} while (0)

#define LOOKUP_PLAYLIST_RET(ret) do { \
    playlist = lookup_playlist (playlist_num); \
    g_return_val_if_fail (playlist != NULL, ret); \
} while (0)

#define LOOKUP_PLAYLIST_ENTRY do { \
    playlist = lookup_playlist (playlist_num); \
    g_return_if_fail (playlist != NULL); \
    entry = lookup_entry (playlist, entry_num); \
    g_return_if_fail (entry != NULL); \
} while (0)

#define LOOKUP_PLAYLIST_ENTRY_RET(ret) do { \
    playlist = lookup_playlist (playlist_num); \
    g_return_val_if_fail (playlist != NULL, ret); \
    entry = lookup_entry (playlist, entry_num); \
    g_return_val_if_fail (entry != NULL, ret); \
} while (0)

#define SELECTION_HAS_CHANGED(p, a, c) \
 queue_update (PLAYLIST_UPDATE_SELECTION, p, a, c)

#define METADATA_WILL_CHANGE scan_stop ()

#define METADATA_HAS_CHANGED(p, a, c) do { \
    scan_reset (); \
    queue_update (PLAYLIST_UPDATE_METADATA, p, a, c); \
} while (0)

#define PLAYLIST_WILL_CHANGE scan_stop ()

#define PLAYLIST_HAS_CHANGED(p, a, c) do { \
    scan_reset (); \
    queue_update (PLAYLIST_UPDATE_STRUCTURE, p, a, c); \
} while (0)

typedef struct {
    gint number;
    gchar *filename;
    PluginHandle * decoder;
    Tuple *tuple;
    gchar * formatted, * title, * artist, * album;
    gint length;
    gboolean failed;
    gboolean selected;
    gint shuffle_num;
    gboolean queued;
    gboolean segmented;
    gint start;
    gint end;
} Entry;

typedef struct {
    gint number;
    gint unique_id;
    gchar *filename;
    gchar *title;
    struct index *entries;
    Entry * position;
    gint selected_count;
    gint last_shuffle_num;
    GList *queued;
    gint64 total_length;
    gint64 selected_length;
} Playlist;

static gint next_unique_id = 1000;

static struct index * playlists = NULL;
static Playlist * active_playlist = NULL;
static Playlist * playing_playlist = NULL;

static gint update_source, update_level;
static Playlist * update_playlist;
static gint unchanged_before, unchanged_after;

static gint scan_source;
static GMutex * scan_mutex;
static GCond * scan_conds[SCAN_THREADS];
static const gchar * scan_filenames[SCAN_THREADS];
static PluginHandle * scan_decoders[SCAN_THREADS];
static Tuple * scan_tuples[SCAN_THREADS];
static gboolean scan_quit;
static GThread * scan_threads[SCAN_THREADS];
static gint scan_positions[SCAN_THREADS];

static void * scanner (void * unused);

static gchar *title_from_tuple(Tuple * tuple)
{
    const gchar *format = tuple_get_string(tuple, FIELD_FORMATTER, NULL);

    if (format == NULL)
        format = get_gentitle_format();

    return tuple_formatter_make_title_string(tuple, format);
}

static void entry_set_tuple_real (Entry * entry, Tuple * tuple)
{
    /* Hack: We cannot refresh segmented entries (since their info is read from
     * the cue sheet when it is first loaded), so leave them alone. -jlindgren */
    if (entry->segmented && ! tuple)
        return;

    if (entry->tuple != NULL)
        tuple_free (entry->tuple);
    entry->tuple = tuple;

    g_free (entry->formatted);
    stringpool_unref (entry->title);
    stringpool_unref (entry->artist);
    stringpool_unref (entry->album);

    if (tuple == NULL)
    {
        entry->formatted = NULL;
        entry->title = NULL;
        entry->artist = NULL;
        entry->album = NULL;
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
        entry->length = MAX (entry->length, 0);

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
    if (entry->tuple != NULL)
    {
        playlist->total_length -= entry->length;

        if (entry->selected)
            playlist->selected_length -= entry->length;
    }

    entry_set_tuple_real (entry, tuple);

    if (tuple != NULL)
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

static Entry * entry_new (gchar * filename, PluginHandle * decoder, Tuple *
 tuple)
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
    g_free(entry->filename);

    if (entry->tuple != NULL)
        tuple_free(entry->tuple);

    g_free (entry->formatted);
    stringpool_unref (entry->title);
    stringpool_unref (entry->artist);
    stringpool_unref (entry->album);
    g_free (entry);
}

static void entry_check_has_decoder (Playlist * playlist, Entry * entry)
{
    if (entry->decoder != NULL || entry->failed)
        return;

    entry->decoder = file_find_decoder (entry->filename, FALSE);
    if (! entry->decoder)
        entry_set_failed (playlist, entry);
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
    gint count;

    g_free(playlist->filename);
    g_free(playlist->title);

    for (count = 0; count < index_count(playlist->entries); count++)
        entry_free(index_get(playlist->entries, count));

    index_free(playlist->entries);
    g_list_free(playlist->queued);
    g_free(playlist);
}

static void number_playlists(gint at, gint length)
{
    gint count;

    for (count = 0; count < length; count++)
    {
        Playlist * playlist = index_get (playlists, at + count);
        playlist->number = at + count;
    }
}

static Playlist * lookup_playlist (gint playlist_num)
{
    /* Not initted or already shut down */
    if (! playlists)
        return NULL;

    if (playlist_num < 0 || playlist_num >= index_count(playlists))
        return NULL;

    return index_get(playlists, playlist_num);
}

static void number_entries (Playlist * playlist, gint at, gint length)
{
    gint count;

    for (count = 0; count < length; count++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        entry->number = at + count;
    }
}

static Entry * lookup_entry (Playlist * playlist, gint entry_num)
{
    if (entry_num < 0 || entry_num >= index_count(playlist->entries))
        return NULL;

    return index_get(playlist->entries, entry_num);
}

static gboolean update (void * unused)
{
    hook_call ("playlist update", GINT_TO_POINTER (update_level));
    update_source = 0;
    return FALSE;
}

static void queue_update (gint level, Playlist * list, gint at, gint count)
{
    if (! update_source)
    {
        update_source = g_idle_add_full (G_PRIORITY_HIGH_IDLE, update, NULL,
         NULL);
        update_level = 0;
        update_playlist = list;
        unchanged_before = list ? index_count (list->entries) : 0;
        unchanged_after = list ? index_count (list->entries) : 0;
    }

    update_level = MAX (update_level, level);

    if (list && list == update_playlist)
    {
        unchanged_before = MIN (unchanged_before, at);
        unchanged_after = MIN (unchanged_after, index_count (list->entries) - at
         - count);
    }
    else
    {
        update_playlist = NULL;
        unchanged_before = 0;
        unchanged_after = 0;
    }
}

gboolean playlist_update_pending (void)
{
    return update_source ? TRUE : FALSE;
}

gboolean playlist_update_range (gint * playlist, gint * at, gint * count)
{
    g_return_val_if_fail (update_source, FALSE);

    if (! update_playlist)
        return FALSE;

    * playlist = update_playlist->number;
    * at = unchanged_before;
    * count = index_count (update_playlist->entries) - unchanged_before -
     unchanged_after;
    return TRUE;
}

/* scan_mutex must be locked! */
void scan_receive (void)
{
    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (! scan_filenames[i] || scan_decoders[i])
            continue; /* thread not in use or still working */

        Entry * entry = index_get (active_playlist->entries, scan_positions[i]);

        if (scan_tuples[i])
            entry_set_tuple (active_playlist, entry, scan_tuples[i]);
        else
            entry_set_failed (active_playlist, entry);

        scan_filenames[i] = NULL;
        scan_tuples[i] = NULL;

        queue_update (PLAYLIST_UPDATE_METADATA, active_playlist,
         scan_positions[i], 1);
    }
}

static gboolean scan_next (void * unused)
{
    gint entries = index_count (active_playlist->entries);
    gint search = 0;

    g_mutex_lock (scan_mutex);

    if (scan_source)
    {
        g_source_remove (scan_source);
        scan_source = 0;
    }

    scan_receive ();

    for (gint i = 0; i < SCAN_THREADS; i ++)
        search = MAX (search, scan_positions[i] + 1);

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (scan_filenames[i])
            continue; /* thread already in use */

        for (; search < entries; search ++)
        {
            Entry * entry = index_get (active_playlist->entries, search);

            if (entry->tuple)
                continue;

            entry_check_has_decoder (active_playlist, entry);
            if (entry->failed)
                continue;

            vfs_prepare_filename (entry->filename);

            scan_positions[i] = search;
            scan_filenames[i] = entry->filename;
            scan_decoders[i] = entry->decoder;
            g_cond_signal (scan_conds[i]);

            search ++;
            break;
        }
    }

    g_mutex_unlock (scan_mutex);
    return FALSE;
}

static void scan_continue (void)
{
    if (! scan_source)
        scan_source = g_idle_add_full (G_PRIORITY_LOW, scan_next, NULL, NULL);
}

static void scan_reset (void)
{
    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        assert (! scan_filenames[i]); /* scan in progress == very, very bad */
        scan_positions[i] = -1;
    }

    scan_continue ();
}

static void scan_stop (void)
{
    g_mutex_lock (scan_mutex);

    if (scan_source != 0)
    {
        g_source_remove (scan_source);
        scan_source = 0;
    }

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        if (! scan_filenames[i])
            continue;

        while (scan_decoders[i])
            g_cond_wait (scan_conds[i], scan_mutex);
    }

    scan_receive ();
    g_mutex_unlock (scan_mutex);
}

static void * scanner (void * data)
{
    gint i = GPOINTER_TO_INT (data);

    g_mutex_lock (scan_mutex);
    g_cond_signal (scan_conds[i]);

    while (1)
    {
        g_cond_wait (scan_conds[i], scan_mutex);

        if (scan_quit)
            break;

        if (! scan_filenames[i])
            continue;

        scan_tuples[i] = file_read_tuple (scan_filenames[i], scan_decoders[i]);
        scan_decoders[i] = NULL;
        g_cond_signal (scan_conds[i]);
        scan_continue ();
    }

    g_mutex_unlock (scan_mutex);
    return NULL;
}

/* As soon as we know the caller is looking for metadata, we start the threaded
 * scanner.  Though it may be faster in the short run simply to scan the entry
 * we are concerned with in the main thread, this is better in the long run
 * because the scanner can work on the following entries while the caller is
 * processing this one. */
static gboolean scan_threaded (Playlist * playlist, Entry * entry)
{
    gint i;

    if (playlist != active_playlist)
        return FALSE;

    scan_next (NULL);

    if (entry->tuple)
        return TRUE;

    for (i = 0; i < SCAN_THREADS; i ++)
    {
        if (entry->number == scan_positions[i])
            goto FOUND;
    }

    return FALSE;

FOUND:
    g_mutex_lock (scan_mutex);
    scan_receive ();

    while (scan_filenames[i])
    {
        g_cond_wait (scan_conds[i], scan_mutex);
        scan_receive ();
    }

    g_mutex_unlock (scan_mutex);
    return TRUE;
}

static void check_scanned (Playlist * playlist, Entry * entry)
{
    if (entry->tuple)
        return;
    if (scan_threaded (playlist, entry))
        return;

    entry_check_has_decoder (playlist, entry);
    if (entry->failed)
        return;

    entry_set_tuple (playlist, entry, file_read_tuple (entry->filename,
     entry->decoder));
    if (! entry->tuple)
        entry_set_failed (playlist, entry);

    queue_update (PLAYLIST_UPDATE_METADATA, playlist, entry->number, 1);
}

static void check_selected_scanned (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    for (gint count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            check_scanned (playlist, entry);
    }
}

static void check_all_scanned (Playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    for (gint count = 0; count < entries; count++)
        check_scanned (playlist, index_get (playlist->entries, count));
}

void playlist_init (void)
{
    srand (time (NULL));

    playlists = index_new ();
    Playlist * playlist = playlist_new ();
    index_append (playlists, playlist);
    playlist->number = 0;
    active_playlist = playlist;
    playing_playlist = NULL;

    update_source = 0;

    scan_mutex = g_mutex_new ();
    memset (scan_filenames, 0, sizeof scan_filenames);
    memset (scan_decoders, 0, sizeof scan_decoders);
    memset (scan_tuples, 0, sizeof scan_tuples);
    scan_source = 0;
    scan_quit = FALSE;

    g_mutex_lock (scan_mutex);

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        scan_conds[i] = g_cond_new ();
        scan_threads[i] = g_thread_create (scanner, GINT_TO_POINTER (i), TRUE,
         NULL);
        g_cond_wait (scan_conds[i], scan_mutex);
    }

    g_mutex_unlock (scan_mutex);

    scan_reset ();
}

void playlist_end(void)
{
    gint count;

    scan_stop ();
    scan_quit = TRUE;

    for (gint i = 0; i < SCAN_THREADS; i ++)
    {
        g_mutex_lock (scan_mutex);
        g_cond_signal (scan_conds[i]);
        g_mutex_unlock (scan_mutex);
        g_thread_join (scan_threads[i]);
        g_cond_free (scan_conds[i]);
    }

    g_mutex_free (scan_mutex);

    if (update_source != 0)
        g_source_remove(update_source);

    for (count = 0; count < index_count(playlists); count++)
        playlist_free(index_get(playlists, count));

    index_free(playlists);
    playlists = NULL;
    active_playlist = NULL;
    playing_playlist = NULL;
}

gint playlist_count(void)
{
    return index_count(playlists);
}

void playlist_insert(gint at)
{
    PLAYLIST_WILL_CHANGE;

    if (at < 0 || at > index_count(playlists))
        at = index_count(playlists);

    if (at == index_count(playlists))
        index_append(playlists, playlist_new());
    else
        index_insert(playlists, at, playlist_new());

    number_playlists(at, index_count(playlists) - at);

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

void playlist_reorder (gint from, gint to, gint count)
{
    struct index * displaced;

    g_return_if_fail (from >= 0 && from + count <= index_count (playlists));
    g_return_if_fail (to >= 0 && to + count <= index_count (playlists));
    g_return_if_fail (count >= 0);

    PLAYLIST_WILL_CHANGE;

    displaced = index_new ();

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

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

void playlist_delete (gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    if (playlist == playing_playlist)
    {
        if (playback_get_playing ())
            playback_stop ();

        playing_playlist = NULL;
    }

    PLAYLIST_WILL_CHANGE;

    playlist_free(playlist);
    index_delete(playlists, playlist_num, 1);
    number_playlists(playlist_num, index_count(playlists) - playlist_num);

    if (index_count(playlists) == 0)
        playlist_insert(0);

    if (playlist == active_playlist)
        active_playlist = index_get (playlists, MIN (playlist_num, index_count
         (playlists) - 1));

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

gint playlist_get_unique_id (gint playlist_num)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (-1);
    return playlist->unique_id;
}

gint playlist_by_unique_id (gint id)
{
    g_return_val_if_fail (playlists, -1);
    gint n = index_count (playlists);

    for (gint i = 0; i < n; i ++)
    {
        Playlist * p = index_get (playlists, i);
        if (p->unique_id == id)
            return p->number;
    }

    return -1;
}

void playlist_set_filename(gint playlist_num, const gchar * filename)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    g_free(playlist->filename);
    playlist->filename = g_strdup(filename);

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

const gchar *playlist_get_filename(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (NULL);

    return playlist->filename;
}

void playlist_set_title(gint playlist_num, const gchar * title)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    g_free(playlist->title);
    playlist->title = g_strdup(title);

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

const gchar *playlist_get_title(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (NULL);

    return playlist->title;
}

void playlist_set_active(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    active_playlist = playlist;

    PLAYLIST_HAS_CHANGED (NULL, 0, 0);
}

gint playlist_get_active(void)
{
    return (active_playlist == NULL) ? -1 : active_playlist->number;
}

void playlist_set_playing(gint playlist_num)
{
    DECLARE_PLAYLIST;

    if (playlist_num == -1)
        playlist = NULL;
    else
        LOOKUP_PLAYLIST;

    if (playing_playlist != NULL && playback_get_playing ())
        playback_stop();

    playing_playlist = playlist;
}

gint playlist_get_playing(void)
{
    return (playing_playlist == NULL) ? -1 : playing_playlist->number;
}

/* If we are already at the song or it is already at the top of the shuffle
 * list, we let it be.  Otherwise, we move it to the top. */
static void set_position (Playlist * playlist, Entry * entry)
{
    if (entry == playlist->position)
        return;

    playlist->position = entry;

    if (entry == NULL)
        return;

    if (! entry->shuffle_num || entry->shuffle_num != playlist->last_shuffle_num)
    {
        playlist->last_shuffle_num ++;
        entry->shuffle_num = playlist->last_shuffle_num;
    }
}

gint playlist_entry_count(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return index_count(playlist->entries);
}

static void make_entries (gchar * filename, PluginHandle * decoder, Tuple *
 tuple, struct index * list)
{
    uri_check_utf8 (& filename, TRUE);

    if (! tuple && ! decoder)
        decoder = file_find_decoder (filename, TRUE);

    if (! tuple && decoder && input_plugin_has_subtunes (decoder) && ! strchr
     (filename, '?'))
        tuple = file_read_tuple (filename, decoder);

    if (tuple != NULL && tuple->nsubtunes > 0)
    {
        gint subtune;

        for (subtune = 0; subtune < tuple->nsubtunes; subtune++)
        {
            gchar *name = g_strdup_printf("%s?%d", filename, (tuple->subtunes == NULL) ? 1 + subtune : tuple->subtunes[subtune]);
            make_entries(name, decoder, NULL, list);
        }

        g_free(filename);
        tuple_free(tuple);
    }
    else
        index_append(list, entry_new(filename, decoder, tuple));
}

void playlist_entry_insert(gint playlist_num, gint at, gchar * filename, Tuple * tuple)
{
    struct index *filenames = index_new();
    struct index *tuples = index_new();

    index_append(filenames, filename);
    index_append(tuples, tuple);

    playlist_entry_insert_batch_with_decoders (playlist_num, at, filenames,
     NULL, tuples);
}

void playlist_entry_insert_batch (gint playlist_num, gint at,
 struct index * filenames, struct index * tuples)
{
    playlist_entry_insert_batch_with_decoders (playlist_num, at, filenames,
     NULL, tuples);
}

void playlist_entry_insert_batch_with_decoders (gint playlist_num, gint at,
 struct index * filenames, struct index * decoders, struct index * tuples)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    gint entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;

    gint number = index_count (filenames);
    struct index * add = index_new ();

    /* Preallocate space to avoid reallocs.  (The actual number of entries may
     * turn out to be greater due to subtunes.) */
    index_allocate (add, number);

    for (gint count = 0; count < number; count ++)
        make_entries (index_get (filenames, count), decoders ? index_get
         (decoders, count) : NULL, tuples ? index_get (tuples, count) : NULL,
         add);

    index_free (filenames);
    if (decoders)
        index_free (decoders);
    if (tuples)
        index_free (tuples);

    number = index_count (add);
    index_merge_insert (playlist->entries, at, add);
    index_free (add);

    number_entries(playlist, at, entries + number - at);

    for (gint count = 0; count < number; count ++)
    {
        Entry * entry = index_get (playlist->entries, at + count);
        playlist->total_length += entry->length;
    }

    PLAYLIST_HAS_CHANGED (playlist, at, number);
}

void playlist_entry_delete(gint playlist_num, gint at, gint number)
{
    DECLARE_PLAYLIST;
    gboolean stop = FALSE;
    gint entries, count;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    for (count = 0; count < number; count++)
    {
        Entry * entry = index_get (playlist->entries, at + count);

        if (entry == playlist->position)
        {
            stop = (playlist == playing_playlist);
            set_position (playlist, NULL);
        }

        if (entry->selected)
        {
            playlist->selected_count--;
            playlist->selected_length -= entry->length;
        }

        if (entry->queued)
            playlist->queued = g_list_remove(playlist->queued, entry);

        playlist->total_length -= entry->length;

        entry_free(entry);
    }

    index_delete (playlist->entries, at, number);
    number_entries (playlist, at, entries - at - number);

    if (stop && playback_get_playing ())
        playback_stop ();

    PLAYLIST_HAS_CHANGED (playlist, at, 0);
}

const gchar *playlist_entry_get_filename(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    return entry->filename;
}

PluginHandle * playlist_entry_get_decoder (gint playlist_num, gint entry_num,
 gboolean fast)
{
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! fast)
        entry_check_has_decoder (playlist, entry);

    return entry->decoder;
}

void playlist_entry_set_tuple (gint playlist_num, gint entry_num, Tuple * tuple)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY;
    METADATA_WILL_CHANGE;

    entry_set_tuple (playlist, entry, tuple);

    METADATA_HAS_CHANGED (playlist, entry_num, 1);
}

const Tuple * playlist_entry_get_tuple (gint playlist_num, gint entry_num,
 gboolean fast)
{
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! fast)
        check_scanned (playlist, entry);

    return entry->tuple;
}

const gchar * playlist_entry_get_title (gint playlist_num, gint entry_num,
 gboolean fast)
{
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    if (! fast)
        check_scanned (playlist, entry);

    return (entry->formatted == NULL) ? entry->filename : entry->formatted;
}

void playlist_entry_describe (gint playlist_num, gint entry_num,
 const gchar * * title, const gchar * * artist, const gchar * * album,
 gboolean fast)
{
    * title = * artist = * album = NULL;
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY;

    if (! fast)
        check_scanned (playlist, entry);

    if (entry->title)
    {
        * title = entry->title;
        * artist = entry->artist;
        * album = entry->album;
    }
    else
        * title = entry->filename;
}

gint playlist_entry_get_length (gint playlist_num, gint entry_num, gboolean fast)
{
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! fast)
        check_scanned (playlist, entry);

    return entry->length;
}

gboolean playlist_entry_is_segmented(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (FALSE);

    return entry->segmented;
}

gint playlist_entry_get_start_time (gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    return entry->start;
}

gint playlist_entry_get_end_time (gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    return entry->end;
}

void playlist_set_position (gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    if (entry_num == -1)
    {
        LOOKUP_PLAYLIST;
        entry = NULL;
    }
    else
        LOOKUP_PLAYLIST_ENTRY;

    if (playlist == playing_playlist && playback_get_playing ())
        playback_stop ();

    set_position (playlist, entry);

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
}

gint playlist_get_position(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (-1);

    return (playlist->position == NULL) ? -1 : playlist->position->number;
}

void playlist_entry_set_selected(gint playlist_num, gint entry_num, gboolean selected)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY;

    if (entry->selected == selected)
        return;

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

    SELECTION_HAS_CHANGED (playlist, entry_num, 1);
}

gboolean playlist_entry_get_selected(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (FALSE);

    return entry->selected;
}

gint playlist_selected_count(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return playlist->selected_count;
}

void playlist_select_all (gint playlist_num, gboolean selected)
{
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
        SELECTION_HAS_CHANGED (playlist, first, last + 1 - first);
}

gint playlist_shift (gint playlist_num, gint entry_num, gint distance)
{
    DECLARE_PLAYLIST_ENTRY;
    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! entry->selected || ! distance)
        return 0;

    PLAYLIST_WILL_CHANGE;

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
    PLAYLIST_HAS_CHANGED (playlist, top, bottom - top);

    return shift;
}

void playlist_delete_selected(gint playlist_num)
{
    DECLARE_PLAYLIST;
    gboolean stop = FALSE;
    gint entries, count;
    struct index *others;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    entries = index_count (playlist->entries);
    others = index_new();

    for (count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (entry->selected)
        {
            if (entry == playlist->position)
            {
                stop = (playlist == playing_playlist);
                set_position (playlist, NULL);
            }

            if (entry->queued)
                playlist->queued = g_list_remove(playlist->queued, entry);

            playlist->total_length -= entry->length;

            entry_free(entry);
        }
        else
            index_append(others, entry);
    }

    index_free(playlist->entries);
    playlist->entries = others;

    number_entries(playlist, 0, index_count(playlist->entries));
    playlist->selected_count = 0;
    playlist->selected_length = 0;

    if (stop && playback_get_playing ())
        playback_stop ();

    PLAYLIST_HAS_CHANGED (playlist, 0, index_count (playlist->entries));
}

void playlist_reverse(gint playlist_num)
{
    DECLARE_PLAYLIST;
    gint entries, count;
    struct index *reversed;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);
    reversed = index_new();
    count = entries;

    while (count--)
        index_append(reversed, index_get(playlist->entries, count));

    index_free(playlist->entries);
    playlist->entries = reversed;

    number_entries(playlist, 0, entries);

    PLAYLIST_HAS_CHANGED (playlist, 0, entries);
}

void playlist_randomize (gint playlist_num)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    gint entries = index_count (playlist->entries);

    for (gint i = 0; i < entries; i ++)
    {
        gint j = i + rand () % (entries - i);

        struct entry * entry = index_get (playlist->entries, j);
        index_set (playlist->entries, j, index_get (playlist->entries, i));
        index_set (playlist->entries, i, entry);
    }

    number_entries (playlist, 0, entries);
    PLAYLIST_HAS_CHANGED (playlist, 0, entries);
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

    if (a->tuple == NULL)
        return (b->tuple == NULL) ? 0 : -1;
    if (b->tuple == NULL)
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
    PLAYLIST_WILL_CHANGE;

    index_sort_with_data (playlist->entries, compare, inner);
    number_entries (playlist, 0, index_count (playlist->entries));

    PLAYLIST_HAS_CHANGED (playlist, 0, index_count (playlist->entries));
}

static void sort_selected (Playlist * playlist, gint (* compare) (const void *
 a, const void * b, void * inner), void * inner)
{
    gint entries, count, count2;
    struct index *selected;

    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);
    selected = index_new();

    for (count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_append(selected, entry);
    }

    index_sort_with_data (selected, compare, inner);

    count2 = 0;

    for (count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (entry->selected)
            index_set(playlist->entries, count, index_get(selected, count2++));
    }

    index_free(selected);
    number_entries(playlist, 0, entries);

    PLAYLIST_HAS_CHANGED (playlist, 0, entries);
}

void playlist_sort_by_filename (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    sort (playlist, filename_compare, compare);
}

void playlist_sort_by_tuple (gint playlist_num, gint (* compare)
 (const Tuple * a, const Tuple * b))
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    check_all_scanned (playlist);
    sort (playlist, tuple_compare, compare);
}

void playlist_sort_by_title (gint playlist_num, gint (* compare) (const gchar *
 a, const gchar * b))
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    check_all_scanned (playlist);
    sort (playlist, title_compare, compare);
}

void playlist_sort_selected_by_filename (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    sort_selected (playlist, filename_compare, compare);
}

void playlist_sort_selected_by_tuple (gint playlist_num, gint (* compare)
 (const Tuple * a, const Tuple * b))
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    check_selected_scanned (playlist);
    sort_selected (playlist, tuple_compare, compare);
}

void playlist_sort_selected_by_title (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;
    check_selected_scanned (playlist);
    sort (playlist, title_compare, compare);
}

void playlist_reformat_titles (void)
{
    gint playlist_num;

    METADATA_WILL_CHANGE;

    for (playlist_num = 0; playlist_num < index_count(playlists); playlist_num++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count(playlist->entries);
        gint count;

        for (count = 0; count < entries; count++)
        {
            Entry * entry = index_get (playlist->entries, count);
            g_free(entry->formatted);
            entry->formatted = (entry->tuple == NULL) ? NULL : title_from_tuple(entry->tuple);
        }
    }

    METADATA_HAS_CHANGED (NULL, 0, 0);
}

void playlist_rescan_real (gint playlist_num, gboolean onlyselected)
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    METADATA_WILL_CHANGE;

    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);
        if (! onlyselected || entry->selected)
        {
            entry_set_tuple (playlist, entry, NULL);
            entry->failed = FALSE;
        }
    }

    METADATA_HAS_CHANGED (playlist, 0, entries);
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
    gint num_playlists = index_count (playlists);
    gint playlist_num;

    METADATA_WILL_CHANGE;

    gchar * copy = NULL;
    if (! uri_is_utf8 (filename, TRUE))
        filename = copy = uri_to_utf8 (filename);

    for (playlist_num = 0; playlist_num < num_playlists; playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint num_entries = index_count (playlist->entries);
        gint entry_num;

        for (entry_num = 0; entry_num < num_entries; entry_num ++)
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

    METADATA_HAS_CHANGED (NULL, 0, 0);
}

gint64 playlist_get_total_length (gint playlist_num, gboolean fast)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    if (! fast)
        check_all_scanned (playlist);

    return playlist->total_length;
}

gint64 playlist_get_selected_length (gint playlist_num, gboolean fast)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST_RET (0);

    if (! fast)
        check_selected_scanned (playlist);

    return playlist->selected_length;
}

gint playlist_queue_count(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return g_list_length (playlist->queued);
}

void playlist_queue_insert(gint playlist_num, gint at, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY;

    if (entry->queued)
        return;

    if (at < 0)
        playlist->queued = g_list_append(playlist->queued, entry);
    else
        playlist->queued = g_list_insert(playlist->queued, entry, at);

    entry->queued = TRUE;

    SELECTION_HAS_CHANGED (playlist, entry_num, 1);
}

void playlist_queue_insert_selected (gint playlist_num, gint at)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count(playlist->entries);
    gint first = entries, last = 0;

    for (gint count = 0; count < entries; count++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (!entry->selected || entry->queued)
            continue;

        if (at < 0)
            playlist->queued = g_list_append(playlist->queued, entry);
        else
            playlist->queued = g_list_insert(playlist->queued, entry, at++);

        entry->queued = TRUE;
        first = MIN (first, entry->number);
        last = entry->number;
    }

    if (first < entries)
        SELECTION_HAS_CHANGED (playlist, first, last + 1 - first);
}

gint playlist_queue_get_entry(gint playlist_num, gint at)
{
    DECLARE_PLAYLIST;
    GList *node;

    LOOKUP_PLAYLIST_RET (-1);
    node = g_list_nth(playlist->queued, at);

    if (node == NULL)
        return -1;

    return ((Entry *) node->data)->number;
}

gint playlist_queue_find_entry(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (-1);

    if (! entry->queued)
        return -1;

    return g_list_index(playlist->queued, entry);
}

void playlist_queue_delete(gint playlist_num, gint at, gint number)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);
    gint first = entries, last = 0;

    if (at == 0)
    {
        while (playlist->queued != NULL && number--)
        {
            Entry * entry = playlist->queued->data;

            playlist->queued = g_list_delete_link(playlist->queued, playlist->queued);

            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;
        }
    }
    else
    {
        GList *anchor = g_list_nth(playlist->queued, at - 1);

        if (anchor == NULL)
            goto DONE;

        while (anchor->next != NULL && number--)
        {
            Entry * entry = anchor->next->data;

            playlist->queued = g_list_delete_link(playlist->queued, anchor->next);

            entry->queued = FALSE;
            first = MIN (first, entry->number);
            last = entry->number;
        }
    }

DONE:
    if (first < entries)
        SELECTION_HAS_CHANGED (playlist, first, last + 1 - first);
}

void playlist_queue_delete_selected (gint playlist_num)
{
    DECLARE_PLAYLIST;
    LOOKUP_PLAYLIST;

    gint entries = index_count (playlist->entries);
    gint first = entries, last = 0;

    for (GList * node = playlist->queued; node != NULL; )
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
        SELECTION_HAS_CHANGED (playlist, first, last + 1 - first);
}

static gboolean shuffle_prev (Playlist * playlist)
{
    gint entries = index_count (playlist->entries), count;
    Entry * found = NULL;

    for (count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);

        if (entry->shuffle_num && (playlist->position == NULL ||
         entry->shuffle_num < playlist->position->shuffle_num) && (found == NULL
         || entry->shuffle_num > found->shuffle_num))
            found = entry;
    }

    if (found == NULL)
        return FALSE;

    playlist->position = found;
    return TRUE;
}

gboolean playlist_prev_song(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (FALSE);

    if (cfg.shuffle)
    {
        if (! shuffle_prev (playlist))
            return FALSE;
    }
    else
    {
        if (playlist->position == NULL || playlist->position->number == 0)
            return FALSE;

        set_position (playlist, index_get (playlist->entries,
         playlist->position->number - 1));
    }

    if (playlist == playing_playlist && playback_get_playing ())
        playback_stop();

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
        else if (playlist->position != NULL && entry->shuffle_num >
         playlist->position->shuffle_num && (found == NULL || entry->shuffle_num
         < found->shuffle_num))
            found = entry;
    }

    if (found != NULL)
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
    gint entries = index_count (playlist->entries), count;

    playlist->last_shuffle_num = 0;

    for (count = 0; count < entries; count ++)
    {
        Entry * entry = index_get (playlist->entries, count);
        entry->shuffle_num = 0;
    }
}

gboolean playlist_next_song(gint playlist_num, gboolean repeat)
{
    DECLARE_PLAYLIST;
    gint entries;

    LOOKUP_PLAYLIST_RET (FALSE);
    entries = index_count(playlist->entries);

    if (entries == 0)
        return FALSE;

    /* If we have a song in queue, jump to it, _then_ remove it from queue */
    if (playlist->queued != NULL)
    {
        set_position (playlist, playlist->queued->data);

        playlist->queued = g_list_remove(playlist->queued, playlist->position);
        playlist->position->queued = FALSE;
    }
    else if (cfg.shuffle)
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
        if (playlist->position == NULL)
            set_position (playlist, index_get (playlist->entries, 0));
        else if (playlist->position->number == entries - 1)
        {
            if (!repeat)
                return FALSE;

            set_position (playlist, index_get (playlist->entries, 0));
        }
        else
            set_position (playlist, index_get (playlist->entries,
             playlist->position->number + 1));
    }

    if (playlist == playing_playlist && playback_get_playing ())
        playback_stop();

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

void playlist_save_state (void)
{
    gchar scratch[512];
    FILE * handle;
    gint playlist_num;

    snprintf (scratch, sizeof scratch, "%s/" STATE_FILE,
     get_path (AUD_PATH_USER_DIR));
    handle = fopen (scratch, "w");

    if (handle == NULL)
        return;

    fprintf (handle, "active %d\n", playlist_get_active ());
    fprintf (handle, "playing %d\n", playlist_get_playing ());

    for (playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries), count;

        fprintf (handle, "playlist %d\n", playlist_num);
        fprintf (handle, "position %d\n", playlist_get_position (playlist_num));
        fprintf (handle, "last-shuffled %d\n", playlist->last_shuffle_num);

        for (count = 0; count < entries; count ++)
        {
            Entry * entry = index_get (playlist->entries, count);
            fprintf (handle, "S %d\n", entry->shuffle_num);
        }
    }

    fclose (handle);
}

static gchar parse_key[32];
static gchar * parse_value;

static void parse_next (FILE * handle)
{
    gchar * found;

    parse_value = NULL;

    if (fgets (parse_key, sizeof parse_key, handle) == NULL)
        return;

    found = strchr (parse_key, ' ');

    if (found == NULL)
        return;

    * found = 0;
    parse_value = found + 1;

    found = strchr (parse_value, '\n');

    if (found != NULL)
        * found = 0;
}

static gboolean parse_integer (const gchar * key, gint * value)
{
    return (parse_value != NULL && ! strcmp (parse_key, key) && sscanf
     (parse_value, "%d", value) == 1);
}

void playlist_load_state (void)
{
    gchar scratch[512];
    FILE * handle;
    gint playlist_num;

    snprintf (scratch, sizeof scratch, "%s/" STATE_FILE,
     get_path (AUD_PATH_USER_DIR));
    handle = fopen (scratch, "r");

    if (handle == NULL)
        return;

    parse_next (handle);

    if (parse_integer ("active", & playlist_num))
    {
        playlist_set_active (playlist_num);
        parse_next (handle);
    }

    if (parse_integer ("playing", & playlist_num))
    {
        playlist_set_playing (playlist_num);
        parse_next (handle);
    }

    while (parse_integer ("playlist", & playlist_num) && playlist_num >= 0 &&
     playlist_num < index_count (playlists))
    {
        Playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries), position, count;

        parse_next (handle);

        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            playlist->position = index_get (playlist->entries, position);

        gint obsolete = 0;
        if (parse_integer ("shuffled", & obsolete)) /* compatibility with 2.3 */
            parse_next (handle);

        if (parse_integer ("last-shuffled", & playlist->last_shuffle_num))
            parse_next (handle);
        else /* compatibility with 2.3 beta */
            playlist->last_shuffle_num = obsolete;

        for (count = 0; count < entries; count ++)
        {
            Entry * entry = index_get (playlist->entries, count);
            if (parse_integer ("S", & entry->shuffle_num))
                parse_next (handle);
        }
    }

    fclose (handle);
}
