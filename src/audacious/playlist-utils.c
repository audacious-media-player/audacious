/*
 * playlist-utils.c
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

#include <dirent.h>
#include <glib.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "misc.h"
#include "playlist.h"

static const gchar * get_basename (const gchar * filename)
{
    const gchar * slash = strrchr (filename, '/');

    return (slash == NULL) ? filename : slash + 1;
}

static gint filename_compare_basename (const gchar * a, const gchar * b)
{
    return string_compare_encoded (get_basename (a), get_basename (b));
}

static gint tuple_compare_string (const Tuple * a, const Tuple * b, gint field)
{
    const gchar * string_a = tuple_get_string (a, field, NULL);
    const gchar * string_b = tuple_get_string (b, field, NULL);

    if (string_a == NULL)
        return (string_b == NULL) ? 0 : -1;
    if (string_b == NULL)
        return 1;

    return string_compare (string_a, string_b);
}

static gint tuple_compare_int (const Tuple * a, const Tuple * b, gint field)
{
    if (tuple_get_value_type (a, field, NULL) != TUPLE_INT)
        return (tuple_get_value_type (b, field, NULL) != TUPLE_INT) ? 0 : -1;
    if (tuple_get_value_type (b, field, NULL) != TUPLE_INT)
        return 1;

    gint int_a = tuple_get_int (a, field, NULL);
    gint int_b = tuple_get_int (b, field, NULL);

    return (int_a < int_b) ? -1 : (int_a > int_b);
}

static gint tuple_compare_title (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_TITLE);
}

static gint tuple_compare_album (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ALBUM);
}

static gint tuple_compare_artist (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ARTIST);
}

static gint tuple_compare_date (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_YEAR);
}

static gint tuple_compare_track (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_TRACK_NUMBER);
}

static const PlaylistStringCompareFunc filename_comparisons[] = {
 [PLAYLIST_SORT_PATH] = string_compare_encoded,
 [PLAYLIST_SORT_FILENAME] = filename_compare_basename,
 [PLAYLIST_SORT_TITLE] = NULL,
 [PLAYLIST_SORT_ALBUM] = NULL,
 [PLAYLIST_SORT_ARTIST] = NULL,
 [PLAYLIST_SORT_DATE] = NULL,
 [PLAYLIST_SORT_TRACK] = NULL,
 [PLAYLIST_SORT_FORMATTED_TITLE] = NULL};

static const PlaylistTupleCompareFunc tuple_comparisons[] = {
 [PLAYLIST_SORT_PATH] = NULL,
 [PLAYLIST_SORT_FILENAME] = NULL,
 [PLAYLIST_SORT_TITLE] = tuple_compare_title,
 [PLAYLIST_SORT_ALBUM] = tuple_compare_album,
 [PLAYLIST_SORT_ARTIST] = tuple_compare_artist,
 [PLAYLIST_SORT_DATE] = tuple_compare_date,
 [PLAYLIST_SORT_TRACK] = tuple_compare_track,
 [PLAYLIST_SORT_FORMATTED_TITLE] = NULL};

static const PlaylistStringCompareFunc title_comparisons[] = {
 [PLAYLIST_SORT_PATH] = NULL,
 [PLAYLIST_SORT_FILENAME] = NULL,
 [PLAYLIST_SORT_TITLE] = NULL,
 [PLAYLIST_SORT_ALBUM] = NULL,
 [PLAYLIST_SORT_ARTIST] = NULL,
 [PLAYLIST_SORT_DATE] = NULL,
 [PLAYLIST_SORT_TRACK] = NULL,
 [PLAYLIST_SORT_FORMATTED_TITLE] = string_compare};

void playlist_sort_by_scheme (gint playlist, gint scheme)
{
    if (filename_comparisons[scheme] != NULL)
        playlist_sort_by_filename (playlist, filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        playlist_sort_by_tuple (playlist, tuple_comparisons[scheme]);
    else if (title_comparisons[scheme] != NULL)
        playlist_sort_by_title (playlist, title_comparisons[scheme]);
}

void playlist_sort_selected_by_scheme (gint playlist, gint scheme)
{
    if (filename_comparisons[scheme] != NULL)
        playlist_sort_selected_by_filename (playlist,
         filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        playlist_sort_selected_by_tuple (playlist, tuple_comparisons[scheme]);
    else if (title_comparisons[scheme] != NULL)
        playlist_sort_selected_by_title (playlist, title_comparisons[scheme]);
}

/* Fix me:  This considers empty fields as duplicates. */
void playlist_remove_duplicates_by_scheme (gint playlist, gint scheme)
{
    gint entries = playlist_entry_count (playlist);
    gint count;

    if (entries < 1)
        return;

    playlist_select_all (playlist, FALSE);

    if (filename_comparisons[scheme] != NULL)
    {
        gint (* compare) (const gchar * a, const gchar * b) =
         filename_comparisons[scheme];

        playlist_sort_by_filename (playlist, compare);
        gchar * last = playlist_entry_get_filename (playlist, 0);

        for (count = 1; count < entries; count ++)
        {
            gchar * current = playlist_entry_get_filename (playlist, count);

            if (compare (last, current) == 0)
                playlist_entry_set_selected (playlist, count, TRUE);

            g_free (last);
            last = current;
        }

        g_free (last);
    }
    else if (tuple_comparisons[scheme] != NULL)
    {
        gint (* compare) (const Tuple * a, const Tuple * b) =
         tuple_comparisons[scheme];

        playlist_sort_by_tuple (playlist, compare);
        Tuple * last = playlist_entry_get_tuple (playlist, 0, FALSE);

        for (count = 1; count < entries; count ++)
        {
            Tuple * current = playlist_entry_get_tuple (playlist, count, FALSE);

            if (last != NULL && current != NULL && compare (last, current) == 0)
                playlist_entry_set_selected (playlist, count, TRUE);

            if (last)
                tuple_free (last);
            last = current;
        }

        if (last)
            tuple_free (last);
    }

    playlist_delete_selected (playlist);
}

void playlist_remove_failed (gint playlist)
{
    gint entries = playlist_entry_count (playlist);
    gint count;

    playlist_select_all (playlist, FALSE);

    for (count = 0; count < entries; count ++)
    {
        gchar * filename = playlist_entry_get_filename (playlist, count);

        /* vfs_file_test() only works for file:// URIs currently */
        if (! strncmp (filename, "file://", 7) && ! vfs_file_test (filename,
         G_FILE_TEST_EXISTS))
            playlist_entry_set_selected (playlist, count, TRUE);

        g_free (filename);
    }

    playlist_delete_selected (playlist);
}

void playlist_select_by_patterns (gint playlist, const Tuple * patterns)
{
    const gint fields[] = {FIELD_TITLE, FIELD_ALBUM, FIELD_ARTIST,
     FIELD_FILE_NAME};

    gint entries = playlist_entry_count (playlist);
    gint field, entry;

    playlist_select_all (playlist, TRUE);

    for (field = 0; field < G_N_ELEMENTS (fields); field ++)
    {
        const gchar * pattern = tuple_get_string ((Tuple *) patterns,
         fields[field], NULL);
        regex_t regex;

        if (pattern == NULL || pattern[0] == 0)
            continue;

        if (regcomp (& regex, pattern, REG_ICASE) != 0)
            continue;

        for (entry = 0; entry < entries; entry ++)
        {
            const gchar * string;

            if (! playlist_entry_get_selected (playlist, entry))
                continue;

            Tuple * tuple = playlist_entry_get_tuple (playlist, entry, FALSE);
            if (! tuple)
                goto NO_MATCH;

            string = tuple_get_string (tuple, fields[field], NULL);
            if (! string)
                goto NO_MATCH;

            if (regexec (& regex, string, 0, NULL, 0) == 0)
            {
                tuple_free (tuple);
                continue;
            }

        NO_MATCH:
            playlist_entry_set_selected (playlist, entry, FALSE);
            if (tuple)
                tuple_free (tuple);
        }

        regfree (& regex);
    }
}

static gchar * make_playlist_path (gint playlist)
{
    if (! playlist)
        return g_strdup_printf ("%s/playlist.xspf", get_path (AUD_PATH_USER_DIR));

    return g_strdup_printf ("%s/playlist_%02d.xspf",
     get_path (AUD_PATH_PLAYLISTS_DIR), 1 + playlist);
}

static void load_playlists_real (void)
{
    /* old (v3.1 and earlier) naming scheme */

    gint count;
    for (count = 0; ; count ++)
    {
        gchar * path = make_playlist_path (count);

        if (! g_file_test (path, G_FILE_TEST_EXISTS))
        {
            g_free (path);
            break;
        }

        gchar * uri = filename_to_uri (path);

        playlist_insert (count);
        playlist_insert_playlist_raw (count, 0, uri);
        playlist_set_modified (count, TRUE);

        g_free (path);
        g_free (uri);
    }

    /* unique ID-based naming scheme */

    gchar * order_path = g_strdup_printf ("%s/order", get_path (AUD_PATH_PLAYLISTS_DIR));
    gchar * order_string;
    g_file_get_contents (order_path, & order_string, NULL, NULL);
    g_free (order_path);

    if (! order_string)
        goto DONE;

    gchar * * order = g_strsplit (order_string, " ", -1);
    g_free (order_string);

    for (gint i = 0; order[i]; i ++)
    {
        gchar * path = g_strdup_printf ("%s/%s.xspf", get_path (AUD_PATH_PLAYLISTS_DIR), order[i]);
        gchar * uri = filename_to_uri (path);

        playlist_insert_with_id (count + i, atoi (order[i]));
        playlist_insert_playlist_raw (count + i, 0, uri);
        playlist_set_modified (count + i, FALSE);

        g_free (path);
        g_free (uri);
    }

    g_strfreev (order);

DONE:
    if (! playlist_count ())
        playlist_insert (0);

    playlist_set_active (0);
}

static void save_playlists_real (void)
{
    gint lists = playlist_count ();
    const gchar * folder = get_path (AUD_PATH_PLAYLISTS_DIR);

    /* save playlists */

    gchar * * order = g_malloc (sizeof (gchar *) * (lists + 1));
    GHashTable * saved = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    for (gint i = 0; i < lists; i ++)
    {
        gint id = playlist_get_unique_id (i);
        order[i] = g_strdup_printf ("%d", id);

        if (playlist_get_modified (i))
        {
            gchar * path = g_strdup_printf ("%s/%d.xspf", folder, id);
            gchar * uri = filename_to_uri (path);

            playlist_save (i, uri);
            playlist_set_modified (i, FALSE);

            g_free (path);
            g_free (uri);
        }

        g_hash_table_insert (saved, g_strdup_printf ("%d.xspf", id), NULL);
    }

    order[lists] = NULL;
    gchar * order_string = g_strjoinv (" ", order);
    g_strfreev (order);

    GError * error = NULL;
    gchar * order_path = g_strdup_printf ("%s/order", get_path (AUD_PATH_PLAYLISTS_DIR));

    gchar * old_order_string;
    g_file_get_contents (order_path, & old_order_string, NULL, NULL);

    if (! old_order_string || strcmp (old_order_string, order_string))
    {
        if (! g_file_set_contents (order_path, order_string, -1, & error))
        {
            fprintf (stderr, "Cannot write to %s: %s\n", order_path, error->message);
            g_error_free (error);
        }
    }

    g_free (order_string);
    g_free (order_path);
    g_free (old_order_string);

    /* clean up deleted playlists and files from old naming scheme */

    gchar * path = make_playlist_path (0);
    remove (path);
    g_free (path);

    DIR * dir = opendir (folder);
    if (! dir)
        goto DONE;

    struct dirent * entry;
    while ((entry = readdir (dir)))
    {
        if (! g_str_has_suffix (entry->d_name, ".xspf"))
            continue;

        if (! g_hash_table_lookup_extended (saved, entry->d_name, NULL, NULL))
        {
            gchar * path = g_strdup_printf ("%s/%s", folder, entry->d_name);
            remove (path);
            g_free (path);
        }
    }

    closedir (dir);

DONE:
    g_hash_table_destroy (saved);
}

static gboolean hooks_added, state_changed;

static void update_cb (void * data, void * user)
{
    if (GPOINTER_TO_INT (data) < PLAYLIST_UPDATE_METADATA)
        return;

    state_changed = TRUE;
}

static void state_cb (void * data, void * user)
{
    state_changed = TRUE;
}

void load_playlists (void)
{
    load_playlists_real ();
    playlist_load_state ();

    state_changed = FALSE;

    if (! hooks_added)
    {
        hook_associate ("playlist update", update_cb, NULL);
        hook_associate ("playlist activate", state_cb, NULL);
        hook_associate ("playlist position", state_cb, NULL);

        hooks_added = TRUE;
    }
}

void save_playlists (gboolean exiting)
{
    save_playlists_real ();

    /* on exit, save resume time if resume feature is enabled */
    if (state_changed || (exiting && get_bool (NULL, "resume_playback_on_startup")))
    {
        playlist_save_state ();
        state_changed = FALSE;
    }
}
