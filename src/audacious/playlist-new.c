/*
 * playlist-new.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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
#include <glib/gi18n.h>

#include "audstrings.h"
#include "playback.h"
#include "playlist-new.h"
#include "playlist-utils.h"

struct entry
{
    gint number;
    gchar * filename;
    InputPlugin * decoder;
    Tuple * tuple;
    gchar * title;
    glong length;
    gboolean failed;
    gboolean selected;
    gboolean queued;
};

struct playlist
{
    gint number;
    gchar * filename;
    gchar * title;
    struct index * entries;
    struct entry * position;
    gint selected_count;
    struct index * shuffled;
    gint shuffle_position;
    GList * queued;
    glong total_length;
    glong selected_length;
};

static struct index * playlists;
static struct playlist * active_playlist;
static struct playlist * playing_playlist;

static gint update_source;
static gint scan_source;
static gint scan_position;

static gint (* current_filename_compare) (const gchar * a, const gchar * b);
static gint (* current_tuple_compare) (const Tuple * a, const Tuple * b);

static gchar * title_from_tuple (Tuple * tuple)
{
    const gchar * format = tuple_get_string (tuple, FIELD_FORMATTER, NULL);

    if (format == NULL)
        format = get_gentitle_format ();

    return convert_title_text (tuple_formatter_make_title_string (tuple, format));
}

static void entry_set_tuple (struct entry * entry, Tuple * tuple)
{
    if (entry->tuple != NULL)
        tuple_free (entry->tuple);

    entry->tuple = tuple;
    g_free (entry->title);
    entry->title = NULL;
    entry->length = 0;

    if (tuple != NULL)
    {
        entry->title = title_from_tuple (tuple);
        entry->length = tuple_get_int (tuple, FIELD_LENGTH, NULL);

        if (entry->length < 0)
            entry->length = 0;
    }
}

static struct entry * entry_new (gchar * filename, InputPlugin * decoder, Tuple
 * tuple)
{
    struct entry * entry = g_malloc (sizeof (struct entry));

    entry->filename = filename;
    entry->decoder = decoder;
    entry->tuple = NULL;
    entry->title = NULL;
    entry->failed = FALSE;
    entry->number = -1;
    entry->selected = FALSE;
    entry->queued = FALSE;

    entry_set_tuple (entry, tuple);
    return entry;
}

static void entry_free (struct entry * entry)
{
    g_free (entry->filename);

    if (entry->tuple != NULL)
        tuple_free (entry->tuple);

    g_free (entry->title);
    g_free (entry);
}

static struct playlist * playlist_new (void)
{
    struct playlist * playlist = g_malloc (sizeof (struct playlist));

    playlist->number = -1;
    playlist->filename = NULL;
    playlist->title = g_strdup (_ ("Untitled Playlist"));
    playlist->entries = index_new ();
    playlist->position = NULL;
    playlist->selected_count = 0;
    playlist->shuffled = NULL;
    playlist->shuffle_position = -1;
    playlist->queued = NULL;
    playlist->total_length = 0;
    playlist->selected_length = 0;

    return playlist;
}

static void playlist_free (struct playlist * playlist)
{
    gint count;

    g_free (playlist->filename);
    g_free (playlist->title);

    for (count = 0; count < index_count (playlist->entries); count ++)
        entry_free (index_get (playlist->entries, count));

    index_free (playlist->entries);

    if (playlist->shuffled != NULL)
        index_free (playlist->shuffled);

    g_list_free (playlist->queued);
    g_free (playlist);
}

static void number_playlists (gint at, gint length)
{
    gint count;

    for (count = 0; count < length; count ++)
    {
        struct playlist * playlist = index_get (playlists, at + count);

        playlist->number = at + count;
    }
}

static struct playlist * lookup_playlist (gint playlist_num)
{
    if (playlist_num < 0 || playlist_num >= index_count (playlists))
        return NULL;

    return index_get (playlists, playlist_num);
}

static void number_entries (struct playlist * playlist, gint at, gint length)
{
    gint count;

    for (count = 0; count < length; count ++)
    {
        struct entry * entry = index_get (playlist->entries, at + count);

        entry->number = at + count;
    }
}

static struct entry * lookup_entry (struct playlist * playlist, gint entry_num)
{
    if (entry_num < 0 || entry_num >= index_count (playlist->entries))
        return NULL;

    return index_get (playlist->entries, entry_num);
}

static void clear_shuffle (struct playlist * playlist)
{
    if (playlist->shuffled == NULL)
        return;

    index_free (playlist->shuffled);
    playlist->shuffled = NULL;
    playlist->shuffle_position = -1;
}

static void create_shuffle (struct playlist * playlist)
{
    gint entries = index_count (playlist->entries);
    gint count;

    if (playlist->shuffled != NULL)
        clear_shuffle (playlist);

    playlist->shuffled = index_new ();

    if (playlist->position == NULL)
    {
        index_merge_append (playlist->shuffled, playlist->entries);
        playlist->shuffle_position = -1;
        count = 0;
    }
    else
    {
        index_append (playlist->shuffled, playlist->position);
        playlist->shuffle_position = 0;

        for (count = 0; count < entries; count ++)
        {
            struct entry * entry = index_get (playlist->entries, count);

            if (entry != playlist->position)
                index_append (playlist->shuffled, entry);
        }

        count = 1;
    }

    for (; count < entries; count ++)
    {
        gint replace = count + random () % (entries - count);
        struct entry * entry = index_get (playlist->shuffled, replace);

        index_set (playlist->shuffled, replace, index_get (playlist->shuffled,
         count));
        index_set (playlist->shuffled, count, entry);
    }
}

static gboolean update (void * unused)
{
    hook_call ("playlist update", NULL);

    update_source = 0;
    return FALSE;
}

static void queue_update (void)
{
    if (update_source == 0)
        update_source = g_idle_add_full (G_PRIORITY_HIGH_IDLE, update, NULL,
         NULL);
}

static void scan_entry (struct playlist * playlist, struct entry * entry)
{
    if (entry->decoder == NULL)
    {
        entry->decoder = filename_find_decoder (entry->filename);

        if (entry->decoder == NULL)
            goto FAILED;
    }

    if (entry->tuple == NULL)
    {
        if (entry->decoder->get_song_tuple == NULL)
            goto FAILED;

        entry_set_tuple (entry, entry->decoder->get_song_tuple (entry->filename));

        if (entry->tuple == NULL)
            goto FAILED;

        playlist->total_length += entry->length;

        if (entry->selected)
            playlist->selected_length += entry->length;

        if (playlist == active_playlist)
            queue_update ();
    }

    return;

FAILED:
    entry->failed = TRUE;
}

static gboolean scan (void * unused)
{
    gint entries, scanned = 0;

    if (active_playlist == NULL)
        goto DONE;

    entries = index_count (active_playlist->entries);

    while (scan_position < entries && scanned < 10)
    {
        struct entry * entry = index_get (active_playlist->entries,
         scan_position ++);

        if (entry->tuple != NULL || entry->failed)
            continue;

        scan_entry (active_playlist, entry);
        scanned ++;
    }

    if (scan_position < entries)
        return TRUE;

DONE:
    scan_source = 0;
    return FALSE;
}

static void queue_scan (void)
{
    scan_position = 0;

    if (scan_source == 0)
        scan_source = g_idle_add_full (G_PRIORITY_LOW, scan, NULL, NULL);
}

void playlist_init (void)
{
    playlists = index_new ();
    playlist_insert (0);

    active_playlist = index_get (playlists, 0);
    playing_playlist = NULL;

    update_source = 0;
    scan_source = 0;
    scan_position = 0;
}

void playlist_end (void)
{
    gint count;

    if (update_source != 0)
        g_source_remove (update_source);
    if (scan_source != 0)
        g_source_remove (scan_source);

    for (count = 0; count < index_count (playlists); count ++)
        playlist_free (index_get (playlists, count));

    index_free (playlists);
}

gint playlist_count (void)
{
    return index_count (playlists);
}

void playlist_insert (gint at)
{
    if (at < 0 || at > index_count (playlists))
        at = index_count (playlists);

    if (at == index_count (playlists))
        index_append (playlists, playlist_new ());
    else
        index_insert (playlists, at, playlist_new ());

    number_playlists (at, index_count (playlists) - at);

    hook_call ("playlist insert", GINT_TO_POINTER (at));
    queue_update ();
}

void playlist_delete (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    if (playlist == active_playlist)
        active_playlist = NULL;

    if (playlist == playing_playlist)
    {
        playback_stop ();
        playing_playlist = NULL;
    }

    hook_call ("playlist delete", GINT_TO_POINTER (playlist_num));

    playlist_free (playlist);
    index_delete (playlists, playlist_num, 1);
    number_playlists (playlist_num, index_count (playlists) - playlist_num);

    queue_update ();

    if (index_count (playlists) == 0)
        playlist_insert (0);

    if (active_playlist == NULL)
        playlist_set_active (MIN (playlist_num, index_count (playlists) - 1));
}

void playlist_set_filename (gint playlist_num, const gchar * filename)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    g_free (playlist->filename);
    playlist->filename = g_strdup (filename);

    queue_update ();
}

const gchar * playlist_get_filename (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return NULL;

    return playlist->filename;
}

void playlist_set_title (gint playlist_num, const gchar * title)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    g_free (playlist->title);
    playlist->title = g_strdup (title);

    queue_update ();
}

const gchar * playlist_get_title (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return NULL;

    return playlist->title;
}

void playlist_set_active (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    active_playlist = playlist;

    queue_update ();
    queue_scan ();
}

gint playlist_get_active (void)
{
    return (active_playlist == NULL) ? -1 : active_playlist->number;
}

void playlist_set_playing (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    if (playing_playlist != NULL)
        playback_stop ();

    playing_playlist = playlist;
}

gint playlist_get_playing (void)
{
    return (playing_playlist == NULL) ? -1 : playing_playlist->number;
}

gint playlist_entry_count (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return 0;

    return index_count (playlist->entries);
}

static void make_entries (gchar * filename, InputPlugin * decoder, Tuple *
 tuple, struct index * list)
{
    if (tuple == NULL)
    {
        if (decoder == NULL)
            decoder = filename_find_decoder (filename);

        if (decoder != NULL && decoder->have_subtune && decoder->get_song_tuple
         != NULL)
            tuple = decoder->get_song_tuple (filename);
    }

    if (tuple != NULL && tuple->nsubtunes > 0)
    {
        gint subtune;

        for (subtune = 0; subtune < tuple->nsubtunes; subtune ++)
        {
            gchar * name = g_strdup_printf ("%s?%d", filename, (tuple->subtunes
             == NULL) ? 1 + subtune : tuple->subtunes[subtune]);
            make_entries (name, decoder, NULL, list);
        }

        g_free (filename);
        tuple_free (tuple);
    }
    else
        index_append (list, entry_new (filename, decoder, tuple));
}

void playlist_entry_insert (gint playlist_num, gint at, gchar * filename, Tuple
 * tuple)
{
    struct index * filenames = index_new ();
    struct index * tuples = index_new ();

    index_append (filenames, filename);
    index_append (tuples, tuple);

    playlist_entry_insert_batch (playlist_num, at, filenames, tuples);
}

void playlist_entry_insert_batch (gint playlist_num, gint at, struct index *
 filenames, struct index * tuples)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gboolean shuffle;
    gint entries, number, count;
    struct index * add;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);
    shuffle = (playlist->shuffled != NULL);

    if (shuffle)
        clear_shuffle (playlist);

    if (at < 0 || at > entries)
        at = entries;

    number = index_count (filenames);
    add = index_new ();

    for (count = 0; count < number; count ++)
        make_entries (index_get (filenames, count), NULL, (tuples == NULL) ?
         NULL : index_get (tuples, count), add);

    index_free (filenames);

    if (tuples != NULL)
        index_free (tuples);

    number = index_count (add);

    if (at == entries)
        index_merge_append (playlist->entries, add);
    else
        index_merge_insert (playlist->entries, at, add);

    index_free (add);

    number_entries (playlist, at, entries + number - at);

    for (count = 0; count < number; count ++)
    {
        struct entry * entry = index_get (playlist->entries, at + count);

        playlist->total_length += entry->length;
    }

    if (shuffle)
        create_shuffle (playlist);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

void playlist_entry_delete (gint playlist_num, gint at, gint number)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gboolean shuffle, stop = FALSE;
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);
    shuffle = (playlist->shuffled != NULL);

    if (shuffle)
        clear_shuffle (playlist);

    if (at < 0 || at > entries)
        at = entries;
    if (number < 0 || number > entries - at)
        number = entries - at;

    for (count = 0; count < number; count ++)
    {
        struct entry * entry = index_get (playlist->entries, at + count);

        if (entry == playlist->position)
        {
            stop = (playlist == playing_playlist);
            playlist->position = NULL;
        }

        if (entry->selected)
        {
            playlist->selected_count --;
            playlist->selected_length -= entry->length;
        }

        if (entry->queued)
            playlist->queued = g_list_remove (playlist->queued, entry);

        playlist->total_length -= entry->length;

        entry_free (entry);
    }

    index_delete (playlist->entries, at, number);
    number_entries (playlist, at, entries - number - at);

    if (shuffle)
        create_shuffle (playlist);

    if (stop)
        playback_stop ();

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

const gchar * playlist_entry_get_filename (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return NULL;

    entry = lookup_entry (playlist, entry_num);

    return (entry == NULL) ? NULL : entry->filename;
}

InputPlugin * playlist_entry_get_decoder (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return NULL;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL)
        return NULL;

    if (entry->decoder == NULL && ! entry->failed)
        scan_entry (playlist, entry);

    return entry->decoder;
}

const Tuple * playlist_entry_get_tuple (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return NULL;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL)
        return NULL;

    if (entry->tuple == NULL && ! entry->failed)
        scan_entry (playlist, entry);

    return entry->tuple;
}

const gchar * playlist_entry_get_title (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return NULL;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL)
        return NULL;

    if (entry->tuple == NULL && ! entry->failed)
        scan_entry (playlist, entry);

    return (entry->title == NULL) ? entry->filename : entry->title;
}

glong playlist_entry_get_length (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return 0;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL)
        return 0;

    if (entry->tuple == 0 && ! entry->failed)
        scan_entry (playlist, entry);

    return entry->length;
}

void playlist_set_position (gint playlist_num, gint position)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;
    gboolean shuffle;

    if (playlist == NULL)
        return;

    entry = lookup_entry (playlist, position);

    if (entry == NULL)
        return;

    shuffle = (playlist->shuffled != NULL);

    if (shuffle)
        clear_shuffle (playlist);

    playlist->position = entry;

    if (shuffle)
        create_shuffle (playlist);

    if (playlist == playing_playlist)
        playback_stop ();

    if (playlist == active_playlist)
    {
        hook_call ("playlist position", NULL);
        queue_update ();
    }
}

gint playlist_get_position (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    return (playlist == NULL || playlist->position == NULL) ? -1 :
     playlist->position->number;
}

void playlist_entry_set_selected (gint playlist_num, gint entry_num, gboolean
 selected)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL || entry->selected == selected)
        return;

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

    if (playlist == active_playlist)
        queue_update ();
}

gboolean playlist_entry_get_selected (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return FALSE;

    entry = lookup_entry (playlist, entry_num);

    return (entry == NULL) ? FALSE : entry->selected;
}

gint playlist_selected_count (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    return (playlist == NULL) ? 0 : playlist->selected_count;
}

void playlist_select_all (gint playlist_num, gboolean selected)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

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

    if (playlist == active_playlist)
        queue_update ();
}

gint playlist_shift (gint playlist_num, gint position, gint distance)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, first, last, shift, count;
    struct index * move, * others;
    struct entry * entry;

    if (playlist == NULL)
        return 0;

    entry = lookup_entry (playlist, position);

    if (entry == NULL || ! entry->selected)
        return 0;

    entries = index_count (playlist->entries);
    shift = 0;

    for (first = position; first > 0; first --)
    {
        entry = index_get (playlist->entries, first - 1);

        if (! entry->selected)
        {
            if (shift <= distance)
                break;

            shift --;
        }
    }

    for (last = position; last < entries - 1; last ++)
    {
        entry = index_get (playlist->entries, last + 1);

        if (! entry->selected)
        {
            if (shift >= distance)
                break;

            shift ++;
        }
    }

    move = index_new ();
    others = index_new ();

    for (count = first; count <= last; count ++)
    {
        entry = index_get (playlist->entries, count);
        index_append (entry->selected ? move : others, entry);
    }

    if (shift < 0)
    {
        index_merge_append (move, others);
        index_free (others);
    }
    else
    {
        index_merge_append (others, move);
        index_free (move);
        move = others;
    }

    for (count = first; count <= last; count ++)
        index_set (playlist->entries, count, index_get (move, count - first));

    index_free (move);

    number_entries (playlist, first, 1 + last - first);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }

    return shift;
}

gint playlist_shift_selected (gint playlist_num, gint distance)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, shifted, count;
    struct entry * entry;

    if (playlist == NULL)
        return 0;

    entries = index_count (playlist->entries);

    if (entries == 0)
        return 0;

    for (shifted = 0; shifted > distance; shifted --)
    {
        entry = index_get (playlist->entries, 0);

        if (entry->selected)
            break;

        for (count = 1; count < entries; count ++)
        {
            entry = index_get (playlist->entries, count);

            if (! entry->selected)
                continue;

            index_set (playlist->entries, count, index_get (playlist->entries,
             count - 1));
            index_set (playlist->entries, count - 1, entry);
        }
    }

    for (; shifted < distance; shifted ++)
    {
        entry = index_get (playlist->entries, entries - 1);

        if (entry->selected)
            break;

        for (count = entries - 1; count --; )
        {
            entry = index_get (playlist->entries, count);

            if (! entry->selected)
                continue;

            index_set (playlist->entries, count, index_get (playlist->entries,
             count + 1));
            index_set (playlist->entries, count + 1, entry);
        }
    }

    number_entries (playlist, 0, entries);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }

    return shifted;
}

void playlist_delete_selected (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gboolean shuffle, stop = FALSE;
    gint entries, count;
    struct index * others;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);
    shuffle = (playlist->shuffled != NULL);

    if (shuffle)
        clear_shuffle (playlist);

    others = index_new ();

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->selected)
        {
            if (entry == playlist->position)
            {
                stop = (playlist == playing_playlist);
                playlist->position = NULL;
            }

            if (entry->queued)
                playlist->queued = g_list_remove (playlist->queued, entry);

            playlist->total_length -= entry->length;

            entry_free (entry);
        }
        else
            index_append (others, entry);
    }

    index_free (playlist->entries);
    playlist->entries = others;

    number_entries (playlist, 0, index_count (playlist->entries));
    playlist->selected_length = 0;

    if (shuffle)
        create_shuffle (playlist);

    if (stop)
        playback_stop ();

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

void playlist_reverse (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;
    struct index * reversed;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);
    reversed = index_new ();
    count = entries;

    while (count --)
        index_append (reversed, index_get (playlist->entries, count));

    index_free (playlist->entries);
    playlist->entries = reversed;

    number_entries (playlist, 0, entries);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

void playlist_randomize (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        gint replace = count + random () % (entries - count);
        struct entry * entry = index_get (playlist->entries, replace);

        index_set (playlist->entries, replace, index_get (playlist->entries,
         count));
        index_set (playlist->entries, count, entry);
    }

    number_entries (playlist, 0, entries);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

static gint filename_compare (const void * * a, const void * * b)
{
    const struct entry * entry_a = * a, * entry_b = * b;

    return current_filename_compare (entry_a->filename, entry_b->filename);
}

static gint tuple_compare (const void * * a, const void * * b)
{
    const struct entry * entry_a = * a, * entry_b = * b;

    if (entry_a->tuple == NULL)
        return (entry_b->tuple == NULL) ? 0 : -1;
    if (entry_b->tuple == NULL)
        return 1;

    return current_tuple_compare (entry_a->tuple, entry_b->tuple);
}

static void sort (struct playlist * playlist, gint (* compare) (const void * *
 a, const void * * b))
{
    index_sort (playlist->entries, compare);
    number_entries (playlist, 0, index_count (playlist->entries));

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

static void sort_selected (struct playlist * playlist, gint (* compare)
 (const void * * a, const void * * b))
{
    gint entries, count, count2;
    struct index * selected;

    entries = index_count (playlist->entries);
    selected = index_new ();

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->selected)
            index_append (selected, entry);
    }

    index_sort (selected, compare);

    count2 = 0;

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->selected)
            index_set (playlist->entries, count, index_get (selected, count2 ++));
    }

    index_free (selected);
    number_entries (playlist, 0, entries);

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

void playlist_sort_by_filename (gint playlist_num, gint (* compare) (const gchar
 * a, const gchar * b))
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    current_filename_compare = compare;
    sort (playlist, filename_compare);
}

void playlist_sort_by_tuple (gint playlist_num, gint (* compare) (const Tuple *
 a, const Tuple * b))
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->tuple == NULL && ! entry->failed)
            scan_entry (playlist, entry);
    }

    current_tuple_compare = compare;
    sort (playlist, tuple_compare);
}

void playlist_sort_selected_by_filename (gint playlist_num, gint (* compare)
 (const gchar * a, const gchar * b))
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    current_filename_compare = compare;
    sort_selected (playlist, filename_compare);
}

void playlist_sort_selected_by_tuple (gint playlist_num, gint (* compare) (const
 Tuple * a, const Tuple * b))
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (entry->selected && entry->tuple == NULL && ! entry->failed)
            scan_entry (playlist, entry);
    }

    current_tuple_compare = compare;
    sort_selected (playlist, tuple_compare);
}

void playlist_reformat_titles ()
{
    gint playlist_num;

    for (playlist_num = 0; playlist_num < index_count (playlists); playlist_num ++)
    {
        struct playlist * playlist = index_get (playlists, playlist_num);
        gint entries = index_count (playlist->entries);
        gint count;

        for (count = 0; count < entries; count ++)
        {
            struct entry * entry = index_get (playlist->entries, count);

            g_free (entry->title);
            entry->title = (entry->tuple == NULL) ? NULL : title_from_tuple
             (entry->tuple);
        }
    }

    queue_update ();
}

void playlist_rescan (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        entry_set_tuple (entry, NULL);
        entry->failed = FALSE;
    }

    if (playlist == active_playlist)
    {
        queue_update ();
        queue_scan ();
    }
}

glong playlist_get_total_length (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    return (playlist == NULL) ? 0 : playlist->total_length;
}

glong playlist_get_selected_length (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    return (playlist == NULL) ? 0 : playlist->selected_length;
}

void playlist_set_shuffle (gboolean shuffle)
{
    gint playlist_num;

    for (playlist_num = 0; playlist_num < index_count (playlists); playlist_num ++)
    {
        struct playlist * playlist = index_get (playlists, playlist_num);

        if (shuffle)
            create_shuffle (playlist);
        else
            clear_shuffle (playlist);
    }
}

gint playlist_queue_count (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    return (playlist == NULL) ? 0 : g_list_length (playlist->queued);
}

void playlist_queue_insert (gint playlist_num, gint at, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL || entry->queued)
        return;

    if (at < 0)
        playlist->queued = g_list_append (playlist->queued, entry);
    else
        playlist->queued = g_list_insert (playlist->queued, entry, at);

    entry->queued = TRUE;

    if (playlist == active_playlist)
        queue_update ();
}

void playlist_queue_insert_selected (gint playlist_num, gint at)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries, count;

    if (playlist == NULL)
        return;

    entries = index_count (playlist->entries);

    for (count = 0; count < entries; count ++)
    {
        struct entry * entry = index_get (playlist->entries, count);

        if (! entry->selected || entry->queued)
            continue;

        if (at < 0)
            playlist->queued = g_list_append (playlist->queued, entry);
        else
            playlist->queued = g_list_insert (playlist->queued, entry, at ++);

        entry->queued = TRUE;
    }

    if (playlist == active_playlist)
        queue_update ();
}

gint playlist_queue_get_entry (gint playlist_num, gint at)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    GList * node;
    struct entry * entry;

    if (playlist == NULL)
        return -1;

    node = g_list_nth (playlist->queued, at);

    if (node == NULL)
        return -1;

    entry = node->data;
    return entry->number;
}

gint playlist_queue_find_entry (gint playlist_num, gint entry_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    struct entry * entry;

    if (playlist == NULL)
        return -1;

    entry = lookup_entry (playlist, entry_num);

    if (entry == NULL || ! entry->queued)
        return -1;

    return g_list_index (playlist->queued, entry);
}

void playlist_queue_delete (gint playlist_num, gint at, gint number)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL)
        return;

    if (at == 0)
    {
        while (playlist->queued != NULL && number --)
        {
            struct entry * entry = playlist->queued->data;

            playlist->queued = g_list_delete_link (playlist->queued,
             playlist->queued);

            entry->queued = FALSE;
        }
    }
    else
    {
        GList * anchor = g_list_nth (playlist->queued, at - 1);

        if (anchor == NULL)
            return;

        while (anchor->next != NULL && number --)
        {
            struct entry * entry = anchor->next->data;

            playlist->queued = g_list_delete_link (playlist->queued,
             anchor->next);

            entry->queued = FALSE;
        }
    }

    if (playlist == active_playlist)
        queue_update ();
}

gboolean playlist_prev_song (gint playlist_num)
{
    struct playlist * playlist = lookup_playlist (playlist_num);

    if (playlist == NULL || playlist->position == NULL)
        return FALSE;

    if (playlist->queued != NULL)
    {
        if (playlist->position == playlist->queued->data)
            return FALSE;

        playlist->position = playlist->queued->data;
    }
    else if (playlist->shuffled != NULL)
    {
        if (playlist->shuffle_position == 0)
            return FALSE;

        playlist->shuffle_position --;
        playlist->position = index_get (playlist->shuffled,
         playlist->shuffle_position);
    }
    else
    {
        if (playlist->position->number == 0)
            return FALSE;

        playlist->position = index_get (playlist->entries,
         playlist->position->number - 1);
    }

    if (playlist == playing_playlist)
        playback_stop ();

    if (playlist == active_playlist)
    {
        hook_call ("playlist position", NULL);
        queue_update ();
    }

    return TRUE;
}

gboolean playlist_next_song (gint playlist_num, gboolean repeat)
{
    struct playlist * playlist = lookup_playlist (playlist_num);
    gint entries;

    if (playlist == NULL)
        return FALSE;

    entries = index_count (playlist->entries);

    if (entries == 0)
        return FALSE;

    if (playlist->position != NULL && playlist->position->queued)
    {
        playlist->queued = g_list_remove (playlist->queued, playlist->position);
        playlist->position->queued = FALSE;
    }

    if (playlist->queued != NULL)
        playlist->position = playlist->queued->data;
    else if (playlist->shuffled != NULL)
    {
        if (playlist->shuffle_position == entries - 1)
        {
            if (! repeat)
                return FALSE;

            playlist->position = NULL; /* start from random position */
            create_shuffle (playlist);
        }
        else
            playlist->shuffle_position ++;

        playlist->position = index_get (playlist->shuffled,
         playlist->shuffle_position);
    }
    else
    {
        if (playlist->position == NULL)
            playlist->position = index_get (playlist->entries, 0);
        else if (playlist->position->number == entries - 1)
        {
            if (! repeat)
                return FALSE;

            playlist->position = index_get (playlist->entries, 0);
        }
        else
            playlist->position = index_get (playlist->entries,
             playlist->position->number + 1);
    }

    if (playlist == playing_playlist)
        playback_stop ();

    if (playlist == active_playlist)
    {
        hook_call ("playlist position", NULL);
        queue_update ();
    }

    return TRUE;
}
