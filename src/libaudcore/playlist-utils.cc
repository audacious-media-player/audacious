/*
 * playlist-utils.c
 * Copyright 2009-2011 John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"
#include "hook.h"
#include "runtime.h"
#include "tuple.h"
#include "vfs.h"

static const char * get_basename (const char * filename)
{
    const char * slash = strrchr (filename, '/');

    return (slash == NULL) ? filename : slash + 1;
}

static int filename_compare_basename (const char * a, const char * b)
{
    return str_compare_encoded (get_basename (a), get_basename (b));
}

static int tuple_compare_string (const Tuple * a, const Tuple * b, int field)
{
    String string_a = tuple_get_str (a, field);
    String string_b = tuple_get_str (b, field);

    if (! string_a)
        return (! string_b) ? 0 : -1;

    return (! string_b) ? 1 : str_compare (string_a, string_b);
}

static int tuple_compare_int (const Tuple * a, const Tuple * b, int field)
{
    if (tuple_get_value_type (a, field) != TUPLE_INT)
        return (tuple_get_value_type (b, field) != TUPLE_INT) ? 0 : -1;
    if (tuple_get_value_type (b, field) != TUPLE_INT)
        return 1;

    int int_a = tuple_get_int (a, field);
    int int_b = tuple_get_int (b, field);

    return (int_a < int_b) ? -1 : (int_a > int_b);
}

static int tuple_compare_title (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_TITLE);
}

static int tuple_compare_album (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ALBUM);
}

static int tuple_compare_artist (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ARTIST);
}

static int tuple_compare_date (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_YEAR);
}

static int tuple_compare_track (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_TRACK_NUMBER);
}

static int tuple_compare_length (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_LENGTH);
}

static const PlaylistStringCompareFunc filename_comparisons[] = {
 [PLAYLIST_SORT_PATH] = str_compare_encoded,
 [PLAYLIST_SORT_FILENAME] = filename_compare_basename,
 [PLAYLIST_SORT_TITLE] = NULL,
 [PLAYLIST_SORT_ALBUM] = NULL,
 [PLAYLIST_SORT_ARTIST] = NULL,
 [PLAYLIST_SORT_DATE] = NULL,
 [PLAYLIST_SORT_TRACK] = NULL,
 [PLAYLIST_SORT_FORMATTED_TITLE] = NULL,
 [PLAYLIST_SORT_LENGTH] = NULL};

static const PlaylistTupleCompareFunc tuple_comparisons[] = {
 [PLAYLIST_SORT_PATH] = NULL,
 [PLAYLIST_SORT_FILENAME] = NULL,
 [PLAYLIST_SORT_TITLE] = tuple_compare_title,
 [PLAYLIST_SORT_ALBUM] = tuple_compare_album,
 [PLAYLIST_SORT_ARTIST] = tuple_compare_artist,
 [PLAYLIST_SORT_DATE] = tuple_compare_date,
 [PLAYLIST_SORT_TRACK] = tuple_compare_track,
 [PLAYLIST_SORT_FORMATTED_TITLE] = NULL,
 [PLAYLIST_SORT_LENGTH] = tuple_compare_length};

static const PlaylistStringCompareFunc title_comparisons[] = {
 [PLAYLIST_SORT_PATH] = NULL,
 [PLAYLIST_SORT_FILENAME] = NULL,
 [PLAYLIST_SORT_TITLE] = NULL,
 [PLAYLIST_SORT_ALBUM] = NULL,
 [PLAYLIST_SORT_ARTIST] = NULL,
 [PLAYLIST_SORT_DATE] = NULL,
 [PLAYLIST_SORT_TRACK] = NULL,
 [PLAYLIST_SORT_FORMATTED_TITLE] = str_compare,
 [PLAYLIST_SORT_LENGTH] = NULL};

EXPORT void aud_playlist_sort_by_scheme (int playlist, int scheme)
{
    if (filename_comparisons[scheme] != NULL)
        aud_playlist_sort_by_filename (playlist, filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        aud_playlist_sort_by_tuple (playlist, tuple_comparisons[scheme]);
    else if (title_comparisons[scheme] != NULL)
        aud_playlist_sort_by_title (playlist, title_comparisons[scheme]);
}

EXPORT void aud_playlist_sort_selected_by_scheme (int playlist, int scheme)
{
    if (filename_comparisons[scheme] != NULL)
        aud_playlist_sort_selected_by_filename (playlist, filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        aud_playlist_sort_selected_by_tuple (playlist, tuple_comparisons[scheme]);
    else if (title_comparisons[scheme] != NULL)
        aud_playlist_sort_selected_by_title (playlist, title_comparisons[scheme]);
}

/* FIXME: this considers empty fields as duplicates */
EXPORT void aud_playlist_remove_duplicates_by_scheme (int playlist, int scheme)
{
    int entries = aud_playlist_entry_count (playlist);
    if (entries < 1)
        return;

    aud_playlist_select_all (playlist, FALSE);

    if (filename_comparisons[scheme] != NULL)
    {
        int (* compare) (const char * a, const char * b) =
         filename_comparisons[scheme];

        aud_playlist_sort_by_filename (playlist, compare);
        String last = aud_playlist_entry_get_filename (playlist, 0);

        for (int count = 1; count < entries; count ++)
        {
            String current = aud_playlist_entry_get_filename (playlist, count);

            if (compare (last, current) == 0)
                aud_playlist_entry_set_selected (playlist, count, TRUE);

            last = current;
        }
    }
    else if (tuple_comparisons[scheme] != NULL)
    {
        int (* compare) (const Tuple * a, const Tuple * b) =
         tuple_comparisons[scheme];

        aud_playlist_sort_by_tuple (playlist, compare);
        Tuple * last = aud_playlist_entry_get_tuple (playlist, 0, FALSE);

        for (int count = 1; count < entries; count ++)
        {
            Tuple * current = aud_playlist_entry_get_tuple (playlist, count, FALSE);

            if (last != NULL && current != NULL && compare (last, current) == 0)
                aud_playlist_entry_set_selected (playlist, count, TRUE);

            tuple_unref (last);
            last = current;
        }

        tuple_unref (last);
    }

    aud_playlist_delete_selected (playlist);
}

EXPORT void aud_playlist_remove_failed (int playlist)
{
    int entries = aud_playlist_entry_count (playlist);
    int count;

    aud_playlist_select_all (playlist, FALSE);

    for (count = 0; count < entries; count ++)
    {
        String filename = aud_playlist_entry_get_filename (playlist, count);

        /* vfs_file_test() only works for file:// URIs currently */
        if (! strncmp (filename, "file://", 7) && ! vfs_file_test (filename, G_FILE_TEST_EXISTS))
            aud_playlist_entry_set_selected (playlist, count, TRUE);
    }

    aud_playlist_delete_selected (playlist);
}

EXPORT void aud_playlist_select_by_patterns (int playlist, const Tuple * patterns)
{
    const int fields[] = {FIELD_TITLE, FIELD_ALBUM, FIELD_ARTIST,
     FIELD_FILE_NAME};

    int entries = aud_playlist_entry_count (playlist);

    aud_playlist_select_all (playlist, TRUE);

    for (unsigned field = 0; field < ARRAY_LEN (fields); field ++)
    {
        String pattern = tuple_get_str (patterns, fields[field]);
        GRegex * regex;

        if (! pattern || ! pattern[0] || ! (regex = g_regex_new (pattern,
         G_REGEX_CASELESS, (GRegexMatchFlags) 0, NULL)))
            continue;

        for (int entry = 0; entry < entries; entry ++)
        {
            if (! aud_playlist_entry_get_selected (playlist, entry))
                continue;

            Tuple * tuple = aud_playlist_entry_get_tuple (playlist, entry, FALSE);
            String string = tuple ? tuple_get_str (tuple, fields[field]) : String ();

            if (! string || ! g_regex_match (regex, string, (GRegexMatchFlags) 0, NULL))
                aud_playlist_entry_set_selected (playlist, entry, FALSE);

            tuple_unref (tuple);
        }

        g_regex_unref (regex);
    }
}

static String make_playlist_path (int playlist)
{
    if (! playlist)
        return filename_build (aud_get_path (AUD_PATH_USER_DIR), "playlist.xspf");

    SPRINTF (name, "playlist_%02d.xspf", 1 + playlist);
    return filename_build (aud_get_path (AUD_PATH_PLAYLISTS_DIR), name);
}

static void load_playlists_real (void)
{
    const char * folder = aud_get_path (AUD_PATH_PLAYLISTS_DIR);

    /* old (v3.1 and earlier) naming scheme */

    int count;
    for (count = 0; ; count ++)
    {
        String path = make_playlist_path (count);
        if (! g_file_test (path, G_FILE_TEST_EXISTS))
            break;

        aud_playlist_insert (count);
        playlist_insert_playlist_raw (count, 0, filename_to_uri (path));
        playlist_set_modified (count, TRUE);
    }

    /* unique ID-based naming scheme */

    String order_path = filename_build (folder, "order");
    char * order_string;
    Index<String> order;

    g_file_get_contents (order_path, & order_string, NULL, NULL);
    if (! order_string)
        goto DONE;

    order = str_list_to_index (order_string, " ");
    g_free (order_string);

    for (int i = 0; i < order.len (); i ++)
    {
        const String & number = order[i];
        SCONCAT2 (name, number, ".audpl");

        String path = filename_build (folder, name);
        if (! g_file_test (path, G_FILE_TEST_EXISTS))
        {
            SCONCAT2 (name2, number, ".xspf");
            path = filename_build (folder, name2);
        }

        playlist_insert_with_id (count + i, atoi (number));
        playlist_insert_playlist_raw (count + i, 0, filename_to_uri (path));
        playlist_set_modified (count + i, FALSE);

        if (g_str_has_suffix (path, ".xspf"))
            playlist_set_modified (count + i, TRUE);
    }

DONE:
    if (! aud_playlist_count ())
        aud_playlist_insert (0);

    aud_playlist_set_active (0);
}

static void save_playlists_real (void)
{
    int lists = aud_playlist_count ();
    const char * folder = aud_get_path (AUD_PATH_PLAYLISTS_DIR);

    /* save playlists */

    Index<String> order;
    GHashTable * saved = g_hash_table_new_full ((GHashFunc) str_calc_hash,
     g_str_equal, (GDestroyNotify) str_unref, NULL);

    for (int i = 0; i < lists; i ++)
    {
        int id = aud_playlist_get_unique_id (i);
        String number = int_to_str (id);

        SCONCAT2 (name, number, ".audpl");

        if (playlist_get_modified (i))
        {
            String path = filename_build (folder, name);

            aud_playlist_save (i, filename_to_uri (path));
            playlist_set_modified (i, FALSE);
        }

        order.append (std::move (number));
        g_hash_table_insert (saved, str_get (name), NULL);
    }

    String order_string = index_to_str_list (order, " ");
    String order_path = filename_build (folder, "order");

    char * old_order_string;
    g_file_get_contents (order_path, & old_order_string, NULL, NULL);

    if (! old_order_string || strcmp (old_order_string, order_string))
    {
        GError * error = NULL;
        if (! g_file_set_contents (order_path, order_string, -1, & error))
        {
            fprintf (stderr, "Cannot write to %s: %s\n", (const char *) order_path, error->message);
            g_error_free (error);
        }
    }

    g_free (old_order_string);

    /* clean up deleted playlists and files from old naming scheme */

    g_unlink (make_playlist_path (0));

    GDir * dir = g_dir_open (folder, 0, NULL);
    if (! dir)
        goto DONE;

    const char * name;
    while ((name = g_dir_read_name (dir)))
    {
        if (! g_str_has_suffix (name, ".audpl") && ! g_str_has_suffix (name, ".xspf"))
            continue;

        if (! g_hash_table_contains (saved, name))
            g_unlink (filename_build (folder, name));
    }

    g_dir_close (dir);

DONE:
    g_hash_table_destroy (saved);
}

static bool_t hooks_added, state_changed;

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

void save_playlists (bool_t exiting)
{
    save_playlists_real ();

    /* on exit, save resume states */
    if (state_changed || exiting)
    {
        playlist_save_state ();
        state_changed = FALSE;
    }

    if (exiting && hooks_added)
    {
        hook_dissociate ("playlist update", update_cb);
        hook_dissociate ("playlist activate", state_cb);
        hook_dissociate ("playlist position", state_cb);

        hooks_added = FALSE;
    }
}
