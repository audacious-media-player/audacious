/*
 * playlist-new.c
 * Copyright 2009 John Lindgren
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

#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "audstrings.h"
#include "i18n.h"
#include "playback.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "plugin.h"
#include "probe.h"

#define STATE_FILE "playlist-state"

#define DECLARE_PLAYLIST \
    struct playlist * playlist

#define DECLARE_PLAYLIST_ENTRY \
    struct playlist * playlist; \
    struct entry * entry

#define LOOKUP_PLAYLIST \
{ \
    playlist = lookup_playlist (playlist_num); \
    g_return_if_fail (playlist != NULL); \
}

#define LOOKUP_PLAYLIST_RET(ret) \
{ \
    playlist = lookup_playlist (playlist_num); \
    g_return_val_if_fail (playlist != NULL, ret); \
}

#define LOOKUP_PLAYLIST_ENTRY \
{ \
    playlist = lookup_playlist (playlist_num); \
    g_return_if_fail (playlist != NULL); \
    entry = lookup_entry (playlist, entry_num); \
    g_return_if_fail (entry != NULL); \
}

#define LOOKUP_PLAYLIST_ENTRY_RET(ret) \
{ \
    playlist = lookup_playlist (playlist_num); \
    g_return_val_if_fail (playlist != NULL, ret); \
    entry = lookup_entry (playlist, entry_num); \
    g_return_val_if_fail (entry != NULL, ret); \
}

#define SELECTION_HAS_CHANGED \
{ \
    queue_update (PLAYLIST_UPDATE_SELECTION); \
}

#define METADATA_WILL_CHANGE \
{ \
    scan_stop (); \
}

#define METADATA_HAS_CHANGED \
{ \
    scan_reset (); \
    queue_update (PLAYLIST_UPDATE_METADATA); \
}

#define PLAYLIST_WILL_CHANGE \
{ \
    scan_stop (); \
}

#define PLAYLIST_HAS_CHANGED \
{ \
    scan_reset (); \
    queue_update (PLAYLIST_UPDATE_STRUCTURE); \
}

struct entry
{
    gint number;
    gchar *filename;
    InputPlugin *decoder;
    Tuple *tuple;
    gchar *title;
    gint length;
    gboolean failed;
    gboolean selected;
    gboolean shuffle_num;
    gboolean queued;
    gboolean segmented;
    gint start;
    gint end;
};

struct playlist
{
    gint number;
    gchar *filename;
    gchar *title;
    struct index *entries;
    struct entry *position;
    gint selected_count;
    gint shuffle_num, last_shuffle_num;
    GList *queued;
    gint64 total_length;
    gint64 selected_length;
};

static struct index *playlists;
static struct playlist *active_playlist;
static struct playlist *playing_playlist;

static gint update_source, update_level;
static gint scan_source;
static GMutex * scan_mutex;
static GCond * scan_cond;
static const gchar * scan_filename;
static InputPlugin * scan_decoder;
static Tuple * scan_tuple;
static GThread * scan_thread;
static gint scan_position, updated_ago;

static gint(*current_filename_compare) (const gchar * a, const gchar * b);
static gint(*current_tuple_compare) (const Tuple * a, const Tuple * b);

static void * scanner (void * unused);

static gchar *title_from_tuple(Tuple * tuple)
{
    const gchar *format = tuple_get_string(tuple, FIELD_FORMATTER, NULL);

    if (format == NULL)
        format = get_gentitle_format();

    return tuple_formatter_make_title_string(tuple, format);
}

static void entry_set_tuple_real (struct entry * entry, Tuple * tuple)
{
    gint start;
    gint end;

    if (entry->segmented == TRUE && entry->tuple)
    {
        if (entry->end > 0)
            return;

        entry->length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
        entry->length -= entry->start;

        tuple_associate_int(entry->tuple, FIELD_LENGTH, NULL, entry->length);

        return;
    }

    if (entry->tuple != NULL)
        tuple_free (entry->tuple);

    g_free (entry->title);
    entry->tuple = tuple;

    if (tuple == NULL)
    {
        entry->title = NULL;
        entry->length = 0;
        entry->segmented = FALSE;
        entry->start = entry->end = -1;
    }
    else
    {
        entry->title = title_from_tuple (tuple);
        entry->length = tuple_get_int (tuple, FIELD_LENGTH, NULL);
        entry->length = MAX (entry->length, 0);

        if ((tuple_get_int (tuple, FIELD_SEGMENT_START, NULL)) || (tuple_get_int (tuple, FIELD_SEGMENT_END, NULL)))
        {
            start = tuple_get_int (tuple, FIELD_SEGMENT_START, NULL);
            end = tuple_get_int (tuple, FIELD_SEGMENT_END, NULL);

            if (start == end)
                end = 0;

            entry->segmented = TRUE;
            entry->start = start ? start : 0;
            entry->end = end ? end : -1;
        }
    }
}

static void entry_set_tuple (struct playlist * playlist, struct entry * entry,
 Tuple * tuple)
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

static struct entry *entry_new(gchar * filename, InputPlugin * decoder, Tuple * tuple)
{
    struct entry *entry = g_malloc(sizeof(struct entry));

    entry->filename = filename;
    entry->decoder = decoder;
    entry->tuple = NULL;
    entry->title = NULL;
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

static void entry_free(struct entry *entry)
{
    g_free(entry->filename);

    if (entry->tuple != NULL)
        tuple_free(entry->tuple);

    g_free(entry->title);
    g_free(entry);
}

static void entry_check_has_decoder (struct entry * entry)
{
    if (entry->decoder != NULL || entry->failed)
        return;

    entry->decoder = file_probe (entry->filename, TRUE);

    if (entry->decoder == NULL)
        entry->failed = TRUE;
}

static struct playlist *playlist_new(void)
{
    struct playlist *playlist = g_malloc(sizeof(struct playlist));

    playlist->number = -1;
    playlist->filename = NULL;
    playlist->title = g_strdup(_("Untitled Playlist"));
    playlist->entries = index_new();
    playlist->position = NULL;
    playlist->selected_count = 0;
    playlist->shuffle_num = 0;
    playlist->last_shuffle_num = 0;
    playlist->queued = NULL;
    playlist->total_length = 0;
    playlist->selected_length = 0;

    return playlist;
}

static void playlist_free(struct playlist *playlist)
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
        struct playlist *playlist = index_get(playlists, at + count);

        playlist->number = at + count;
    }
}

static struct playlist *lookup_playlist(gint playlist_num)
{
    if (playlist_num < 0 || playlist_num >= index_count(playlists))
        return NULL;

    return index_get(playlists, playlist_num);
}

static void number_entries(struct playlist *playlist, gint at, gint length)
{
    gint count;

    for (count = 0; count < length; count++)
    {
        struct entry *entry = index_get(playlist->entries, at + count);

        entry->number = at + count;
    }
}

static struct entry *lookup_entry(struct playlist *playlist, gint entry_num)
{
    if (entry_num < 0 || entry_num >= index_count(playlist->entries))
        return NULL;

    return index_get(playlist->entries, entry_num);
}

static gboolean update (void * unused)
{
    hook_call ("playlist update", GINT_TO_POINTER (update_level));

    update_source = 0;
    update_level = 0;
    return FALSE;
}

static void queue_update (gint level)
{
    update_level = MAX (update_level, level);

    if (update_source == 0)
        update_source = g_idle_add_full (G_PRIORITY_HIGH_IDLE, update, NULL,
         NULL);
}

static Tuple * get_tuple (const gchar * filename, InputPlugin * decoder)
{
    if (decoder->get_song_tuple != NULL)
        return decoder->get_song_tuple (filename);

    if (decoder->probe_for_tuple != NULL)
    {
        VFSFile * handle = vfs_fopen (filename, "r");
        Tuple * tuple;

        if (handle == NULL)
            return NULL;

        tuple = decoder->probe_for_tuple (filename, handle);
        vfs_fclose (handle);
        return tuple;
    }

    return NULL;
}

/* scan_mutex must be locked! */
void scan_receive (void)
{
    struct entry * entry = index_get (active_playlist->entries, scan_position);

    entry_set_tuple (active_playlist, entry, scan_tuple);

    if (scan_tuple == NULL)
        entry->failed = TRUE;

    scan_filename = NULL;
    scan_decoder = NULL;
    scan_tuple = NULL;
}

static gboolean scan_next (void * unused)
{
    gint entries;

    g_mutex_lock (scan_mutex);

    if (scan_filename != NULL)
    {
        scan_receive ();
        scan_position ++;
        updated_ago ++;
    }

    entries = index_count (active_playlist->entries);

    for (; scan_position < entries; scan_position ++)
    {
        struct entry * entry = index_get (active_playlist->entries,
         scan_position);

        if (entry->tuple != NULL)
            continue;

        entry_check_has_decoder (entry);

        if (! entry->failed)
        {
            scan_filename = entry->filename;
            scan_decoder = entry->decoder;
            g_cond_signal (scan_cond);
            break;
        }
    }

    if (updated_ago >= 10 || (scan_position == entries && updated_ago > 0))
    {
        queue_update (PLAYLIST_UPDATE_METADATA);
        updated_ago = 0;
    }

    scan_source = 0;
    g_mutex_unlock (scan_mutex);
    return FALSE;
}

static void scan_continue (void)
{
    scan_source = g_idle_add_full (G_PRIORITY_LOW, scan_next, NULL, NULL);
}

static void scan_reset (void)
{
    scan_position = 0;
    updated_ago = 0;
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

    if (scan_filename != NULL)
        scan_receive ();

    g_mutex_unlock (scan_mutex);
}

/* scan_mutex must be locked! */
static void * scanner (void * unused)
{
    for (;;)
    {
        g_cond_wait (scan_cond, scan_mutex);

        if (scan_filename == NULL)
            return NULL;

        scan_tuple = get_tuple (scan_filename, scan_decoder);
        scan_continue ();
    }
}

static void check_scanned (struct playlist * playlist, struct entry * entry)
{
    if (entry->tuple != NULL || entry->failed)
        return;

    METADATA_WILL_CHANGE;

    entry_check_has_decoder (entry);

    if (entry->tuple == NULL && ! entry->failed) /* scanner did it for us? */
    {
        entry_set_tuple (playlist, entry, get_tuple (entry->filename,
         entry->decoder));

        if (entry->tuple == NULL)
            entry->failed = TRUE;
    }

    METADATA_HAS_CHANGED;
}

void playlist_init (void)
{
    struct playlist * playlist;

    srandom (time (NULL));

    playlists = index_new ();
    playlist = playlist_new ();
    index_append (playlists, playlist);
    playlist->number = 0;
    active_playlist = playlist;
    playing_playlist = NULL;

    update_source = 0;
    update_level = 0;

    scan_mutex = g_mutex_new ();
    scan_cond = g_cond_new ();
    scan_filename = NULL;
    scan_decoder = NULL;
    scan_tuple = NULL;
    scan_source = 0;
    g_mutex_lock (scan_mutex);
    scan_thread = g_thread_create (scanner, NULL, TRUE, NULL);
    scan_reset ();
}

void playlist_end(void)
{
    gint count;

    scan_stop ();
    g_mutex_lock (scan_mutex);
    scan_filename = NULL; /* tell scanner to quit */
    g_cond_signal (scan_cond);
    g_mutex_unlock (scan_mutex);
    g_thread_join (scan_thread);

    if (update_source != 0)
        g_source_remove(update_source);

    for (count = 0; count < index_count(playlists); count++)
        playlist_free(index_get(playlists, count));

    index_free(playlists);
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

    PLAYLIST_HAS_CHANGED;
    hook_call ("playlist insert", GINT_TO_POINTER (at));
}

void playlist_delete (gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    hook_call ("playlist delete", GINT_TO_POINTER (playlist_num));

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

    PLAYLIST_HAS_CHANGED;
}

void playlist_set_filename(gint playlist_num, const gchar * filename)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;
    METADATA_WILL_CHANGE;

    g_free(playlist->filename);
    playlist->filename = g_strdup(filename);

    METADATA_HAS_CHANGED;
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
    METADATA_WILL_CHANGE;

    g_free(playlist->title);
    playlist->title = g_strdup(title);

    METADATA_HAS_CHANGED;
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

    PLAYLIST_HAS_CHANGED;
}

gint playlist_get_active(void)
{
    return (active_playlist == NULL) ? -1 : active_playlist->number;
}

void playlist_set_playing(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    if (playing_playlist != NULL && playback_get_playing ())
        playback_stop();

    playing_playlist = playlist;
}

gint playlist_get_playing(void)
{
    return (playing_playlist == NULL) ? -1 : playing_playlist->number;
}

static void set_position (struct playlist * playlist, struct entry * entry)
{
    playlist->position = entry;

    if (entry == NULL)
        return;

    if (! entry->shuffle_num || entry->shuffle_num != playlist->last_shuffle_num)
    {
        playlist->last_shuffle_num ++;
        entry->shuffle_num = playlist->last_shuffle_num;
    }

    playlist->shuffle_num = entry->shuffle_num;
}

gint playlist_entry_count(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return index_count(playlist->entries);
}

static void make_entries (gchar * filename, InputPlugin * decoder, Tuple *
 tuple, struct index * list)
{
    if (tuple == NULL && decoder == NULL)
        decoder = file_probe (filename, TRUE);

    if (tuple == NULL && decoder != NULL && decoder->have_subtune &&
     decoder->get_song_tuple != NULL)
        tuple = decoder->get_song_tuple (filename);

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

    playlist_entry_insert_batch(playlist_num, at, filenames, tuples);
}

void playlist_entry_insert_batch(gint playlist_num, gint at, struct index *filenames, struct index *tuples)
{
    DECLARE_PLAYLIST;
    gint entries, number, count;
    struct index *add;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    entries = index_count (playlist->entries);

    if (at < 0 || at > entries)
        at = entries;

    number = index_count(filenames);
    add = index_new();

    for (count = 0; count < number; count++)
        make_entries(index_get(filenames, count), NULL, (tuples == NULL) ? NULL : index_get(tuples, count), add);

    index_free(filenames);

    if (tuples != NULL)
        index_free(tuples);

    number = index_count(add);

    if (at == entries)
        index_merge_append(playlist->entries, add);
    else
        index_merge_insert(playlist->entries, at, add);

    index_free(add);

    number_entries(playlist, at, entries + number - at);

    for (count = 0; count < number; count++)
    {
        struct entry *entry = index_get(playlist->entries, at + count);

        playlist->total_length += entry->length;
    }

    PLAYLIST_HAS_CHANGED;
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
        struct entry *entry = index_get(playlist->entries, at + count);

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

    index_delete(playlist->entries, at, number);
    number_entries(playlist, at, entries - number - at);

    if (stop && playback_get_playing ())
        playback_stop ();

    PLAYLIST_HAS_CHANGED;
}

const gchar *playlist_entry_get_filename(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    return entry->filename;
}

InputPlugin *playlist_entry_get_decoder(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    entry_check_has_decoder (entry);

    return entry->decoder;
}

void playlist_entry_set_tuple (gint playlist_num, gint entry_num, Tuple * tuple)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY;
    METADATA_WILL_CHANGE;

    entry_set_tuple (playlist, entry, tuple);

    METADATA_HAS_CHANGED;
}

const Tuple *playlist_entry_get_tuple(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    check_scanned (playlist, entry);
    return entry->tuple;
}

const gchar *playlist_entry_get_title(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (NULL);

    check_scanned (playlist, entry);
    return (entry->title == NULL) ? entry->filename : entry->title;
}

gint playlist_entry_get_length(gint playlist_num, gint entry_num)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY_RET (0);

    check_scanned (playlist, entry);
    return entry->length;
}

void playlist_entry_set_segmentation(gint playlist_num, gint entry_num, gint start, gint end)
{
    DECLARE_PLAYLIST_ENTRY;

    LOOKUP_PLAYLIST_ENTRY;
    if (entry == NULL)
        return;

    if (start != -1)
    {
        entry->segmented = TRUE;
        entry->start = start;
        entry->end = end;
    }
    else
    {
        entry->segmented = FALSE;
        entry->start = entry->end = -1;
    }
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

    SELECTION_HAS_CHANGED;

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

    SELECTION_HAS_CHANGED;
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

void playlist_select_all(gint playlist_num, gboolean selected)
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;

    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        entry->selected = selected;
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

    SELECTION_HAS_CHANGED;
}

gint playlist_shift (gint playlist_num, gint entry_num, gint distance)
{
    DECLARE_PLAYLIST_ENTRY;
    gint entries, first, last, shift, count;
    struct index *move, *others;

    LOOKUP_PLAYLIST_ENTRY_RET (0);

    if (! entry->selected)
        return 0;

    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);
    shift = 0;

    for (first = entry_num; first > 0; first--)
    {
        entry = index_get(playlist->entries, first - 1);

        if (!entry->selected)
        {
            if (shift <= distance)
                break;

            shift--;
        }
    }

    for (last = entry_num; last < entries - 1; last++)
    {
        entry = index_get(playlist->entries, last + 1);

        if (!entry->selected)
        {
            if (shift >= distance)
                break;

            shift++;
        }
    }

    move = index_new();
    others = index_new();

    for (count = first; count <= last; count++)
    {
        entry = index_get(playlist->entries, count);
        index_append(entry->selected ? move : others, entry);
    }

    if (shift < 0)
    {
        index_merge_append(move, others);
        index_free(others);
    }
    else
    {
        index_merge_append(others, move);
        index_free(move);
        move = others;
    }

    for (count = first; count <= last; count++)
        index_set(playlist->entries, count, index_get(move, count - first));

    index_free(move);

    number_entries(playlist, first, 1 + last - first);

    PLAYLIST_HAS_CHANGED;
    return shift;
}

gint playlist_shift_selected(gint playlist_num, gint distance)
{
    DECLARE_PLAYLIST;
    gint entries, shifted, count;
    struct entry *entry;

    LOOKUP_PLAYLIST_RET (0);
    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);

    if (entries == 0)
        return 0;

    for (shifted = 0; shifted > distance; shifted--)
    {
        entry = index_get(playlist->entries, 0);

        if (entry->selected)
            break;

        for (count = 1; count < entries; count++)
        {
            entry = index_get(playlist->entries, count);

            if (!entry->selected)
                continue;

            index_set(playlist->entries, count, index_get(playlist->entries, count - 1));
            index_set(playlist->entries, count - 1, entry);
        }
    }

    for (; shifted < distance; shifted++)
    {
        entry = index_get(playlist->entries, entries - 1);

        if (entry->selected)
            break;

        for (count = entries - 1; count--;)
        {
            entry = index_get(playlist->entries, count);

            if (!entry->selected)
                continue;

            index_set(playlist->entries, count, index_get(playlist->entries, count + 1));
            index_set(playlist->entries, count + 1, entry);
        }
    }

    number_entries(playlist, 0, entries);

    PLAYLIST_HAS_CHANGED;
    return shifted;
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
        struct entry *entry = index_get(playlist->entries, count);

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
    playlist->selected_length = 0;

    if (stop && playback_get_playing ())
        playback_stop ();

    PLAYLIST_HAS_CHANGED;
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

    PLAYLIST_HAS_CHANGED;
}

void playlist_randomize(gint playlist_num)
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        gint replace = count + random() % (entries - count);
        struct entry *entry = index_get(playlist->entries, replace);

        index_set(playlist->entries, replace, index_get(playlist->entries, count));
        index_set(playlist->entries, count, entry);
    }

    number_entries(playlist, 0, entries);

    PLAYLIST_HAS_CHANGED;
}

static gint filename_compare(const void **a, const void **b)
{
    const struct entry *entry_a = *a, *entry_b = *b;

    return current_filename_compare(entry_a->filename, entry_b->filename);
}

static gint tuple_compare(const void **a, const void **b)
{
    const struct entry *entry_a = *a, *entry_b = *b;

    if (entry_a->tuple == NULL)
        return (entry_b->tuple == NULL) ? 0 : -1;
    if (entry_b->tuple == NULL)
        return 1;

    return current_tuple_compare(entry_a->tuple, entry_b->tuple);
}

static void sort(struct playlist *playlist, gint(*compare) (const void **a, const void **b))
{
    PLAYLIST_WILL_CHANGE;

    index_sort(playlist->entries, compare);
    number_entries(playlist, 0, index_count(playlist->entries));

    PLAYLIST_HAS_CHANGED;
}

static void sort_selected(struct playlist *playlist, gint(*compare) (const void **a, const void **b))
{
    gint entries, count, count2;
    struct index *selected;

    PLAYLIST_WILL_CHANGE;

    entries = index_count(playlist->entries);
    selected = index_new();

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        if (entry->selected)
            index_append(selected, entry);
    }

    index_sort(selected, compare);

    count2 = 0;

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        if (entry->selected)
            index_set(playlist->entries, count, index_get(selected, count2++));
    }

    index_free(selected);
    number_entries(playlist, 0, entries);

    PLAYLIST_HAS_CHANGED;
}

void playlist_sort_by_filename(gint playlist_num, gint(*compare) (const gchar * a, const gchar * b))
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    current_filename_compare = compare;
    sort(playlist, filename_compare);
}

void playlist_sort_by_tuple(gint playlist_num, gint(*compare) (const Tuple * a, const Tuple * b))
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        check_scanned (playlist, entry);
    }

    current_tuple_compare = compare;
    sort(playlist, tuple_compare);
}

void playlist_sort_selected_by_filename(gint playlist_num, gint(*compare) (const gchar * a, const gchar * b))
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST;

    current_filename_compare = compare;
    sort_selected(playlist, filename_compare);
}

void playlist_sort_selected_by_tuple(gint playlist_num, gint(*compare) (const Tuple * a, const Tuple * b))
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        if (entry->selected)
            check_scanned (playlist, entry);
    }

    current_tuple_compare = compare;
    sort_selected(playlist, tuple_compare);
}

void playlist_reformat_titles()
{
    gint playlist_num;

    METADATA_WILL_CHANGE;

    for (playlist_num = 0; playlist_num < index_count(playlists); playlist_num++)
    {
        struct playlist *playlist = index_get(playlists, playlist_num);
        gint entries = index_count(playlist->entries);
        gint count;

        for (count = 0; count < entries; count++)
        {
            struct entry *entry = index_get(playlist->entries, count);

            g_free(entry->title);
            entry->title = (entry->tuple == NULL) ? NULL : title_from_tuple(entry->tuple);
        }
    }

    METADATA_HAS_CHANGED;
}

void playlist_rescan(gint playlist_num)
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    METADATA_WILL_CHANGE;

    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        entry_set_tuple (playlist, entry, NULL);
        entry->failed = FALSE;
    }

    METADATA_HAS_CHANGED;
}

gint64 playlist_get_total_length(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return playlist->total_length;
}

gint64 playlist_get_selected_length(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (0);

    return playlist->selected_length;
}

void playlist_set_shuffle (gboolean on) /* currently not needed */
{
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

    SELECTION_HAS_CHANGED;
}

void playlist_queue_insert_selected (gint playlist_num, gint at)
{
    DECLARE_PLAYLIST;
    gint entries, count;

    LOOKUP_PLAYLIST;
    entries = index_count(playlist->entries);

    for (count = 0; count < entries; count++)
    {
        struct entry *entry = index_get(playlist->entries, count);

        if (!entry->selected || entry->queued)
            continue;

        if (at < 0)
            playlist->queued = g_list_append(playlist->queued, entry);
        else
            playlist->queued = g_list_insert(playlist->queued, entry, at++);

        entry->queued = TRUE;
    }

    SELECTION_HAS_CHANGED;
}

gint playlist_queue_get_entry(gint playlist_num, gint at)
{
    DECLARE_PLAYLIST;
    GList *node;
    struct entry *entry;

    LOOKUP_PLAYLIST_RET (-1);
    node = g_list_nth(playlist->queued, at);

    if (node == NULL)
        return -1;

    entry = node->data;
    return entry->number;
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

    if (at == 0)
    {
        while (playlist->queued != NULL && number--)
        {
            struct entry *entry = playlist->queued->data;

            playlist->queued = g_list_delete_link(playlist->queued, playlist->queued);

            entry->queued = FALSE;
        }
    }
    else
    {
        GList *anchor = g_list_nth(playlist->queued, at - 1);

        if (anchor == NULL)
            goto DONE;

        while (anchor->next != NULL && number--)
        {
            struct entry *entry = anchor->next->data;

            playlist->queued = g_list_delete_link(playlist->queued, anchor->next);

            entry->queued = FALSE;
        }
    }

DONE:
    SELECTION_HAS_CHANGED;
}

static gboolean shuffle_prev (struct playlist * playlist)
{
    gint entries = index_count (playlist->entries), count;
    struct entry * found = NULL;

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->shuffle_num && entry->shuffle_num < playlist->shuffle_num &&
         (found == NULL || entry->shuffle_num > found->shuffle_num))
            found = entry;
    }

    if (found == NULL)
        return FALSE;

    playlist->position = found;
    playlist->shuffle_num = found->shuffle_num;
    return TRUE;
}

gboolean playlist_prev_song(gint playlist_num)
{
    DECLARE_PLAYLIST;

    LOOKUP_PLAYLIST_RET (FALSE);

    if (playlist->queued != NULL)
    {
        if (playlist->position == playlist->queued->data)
            return FALSE;

        set_position (playlist, playlist->queued->data);
    }
    else if (cfg.shuffle)
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

    SELECTION_HAS_CHANGED;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

static gboolean shuffle_next (struct playlist * playlist)
{
    gint entries = index_count (playlist->entries), choice = 0, count;
    struct entry * found = NULL;

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (! entry->shuffle_num)
            choice ++;
        else if (entry->shuffle_num > playlist->shuffle_num && (found == NULL ||
         entry->shuffle_num < found->shuffle_num))
            found = entry;
    }

    if (found != NULL)
    {
        playlist->position = found;
        playlist->shuffle_num = found->shuffle_num;
        return TRUE;
    }

    if (! choice)
        return FALSE;

    choice = random () % choice;

    for (count = 0; ; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

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

static void shuffle_reset (struct playlist * playlist)
{
    gint entries = index_count (playlist->entries), count;

    playlist->shuffle_num = 0;
    playlist->last_shuffle_num = 0;

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

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

    if (playlist->position != NULL && playlist->position->queued)
    {
        playlist->queued = g_list_remove(playlist->queued, playlist->position);
        playlist->position->queued = FALSE;
    }

    if (playlist->queued != NULL)
        set_position (playlist, playlist->queued->data);
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

    SELECTION_HAS_CHANGED;

    hook_call ("playlist position", GINT_TO_POINTER (playlist_num));
    return TRUE;
}

gboolean playlist_update_pending (void)
{
    return (update_source != 0);
}

void playlist_save_state (void)
{
    gchar scratch[512];
    FILE * handle;
    gint playlist_num;

    snprintf (scratch, sizeof scratch, "%s/" STATE_FILE,
     aud_paths[BMP_PATH_USER_DIR]);
    handle = fopen (scratch, "w");

    if (handle == NULL)
        return;

    fprintf (handle, "active %d\n", playlist_get_active ());
    fprintf (handle, "playing %d\n", playlist_get_playing ());

    for (playlist_num = 0; playlist_num < index_count (playlists);
     playlist_num ++)
    {
        struct playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries), count;

        fprintf (handle, "playlist %d\n", playlist_num);
        fprintf (handle, "position %d\n", playlist_get_position (playlist_num));
        fprintf (handle, "shuffled %d\n", playlist->shuffle_num);
        fprintf (handle, "last-shuffled %d\n", playlist->last_shuffle_num);

        for (count = 0; count < entries; count ++)
        {
            struct entry * entry = index_get (playlist->entries, count);

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
     aud_paths[BMP_PATH_USER_DIR]);
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
        struct playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries), position, count;

        parse_next (handle);

        if (parse_integer ("position", & position))
            parse_next (handle);

        if (position >= 0 && position < entries)
            playlist->position = index_get (playlist->entries, position);

        if (parse_integer ("shuffled", & playlist->shuffle_num))
            parse_next (handle);

        if (parse_integer ("last-shuffled", & playlist->last_shuffle_num))
            parse_next (handle);
        else /* compatibility with 2.3 beta */
            playlist->last_shuffle_num = playlist->shuffle_num;

        for (count = 0; count < entries; count ++)
        {
            struct entry * entry = index_get (playlist->entries, count);

            if (parse_integer ("S", & entry->shuffle_num))
                parse_next (handle);
        }
    }

    fclose (handle);
}
