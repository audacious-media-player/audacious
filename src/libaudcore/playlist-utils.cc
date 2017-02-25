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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "hook.h"
#include "multihash.h"
#include "runtime.h"
#include "tuple.h"
#include "vfs.h"

static const char * get_basename (const char * filename)
{
    const char * slash = strrchr (filename, '/');
    return slash ? slash + 1 : filename;
}

static int filename_compare_basename (const char * a, const char * b)
{
    return str_compare_encoded (get_basename (a), get_basename (b));
}

static int tuple_compare_string (const Tuple & a, const Tuple & b, Tuple::Field field)
{
    String string_a = a.get_str (field);
    String string_b = b.get_str (field);

    if (! string_a)
        return (! string_b) ? 0 : -1;

    return (! string_b) ? 1 : str_compare (string_a, string_b);
}

static int tuple_compare_int (const Tuple & a, const Tuple & b, Tuple::Field field)
{
    if (a.get_value_type (field) != Tuple::Int)
        return (b.get_value_type (field) != Tuple::Int) ? 0 : -1;
    if (b.get_value_type (field) != Tuple::Int)
        return 1;

    int int_a = a.get_int (field);
    int int_b = b.get_int (field);

    return (int_a < int_b) ? -1 : (int_a > int_b);
}

static int tuple_compare_title (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::Title); }
static int tuple_compare_album (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::Album); }
static int tuple_compare_artist (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::Artist); }
static int tuple_compare_album_artist (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::AlbumArtist); }
static int tuple_compare_date (const Tuple & a, const Tuple & b)
    { return tuple_compare_int (a, b, Tuple::Year); }
static int tuple_compare_genre (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::Genre); }
static int tuple_compare_track (const Tuple & a, const Tuple & b)
    { return tuple_compare_int (a, b, Tuple::Track); }
static int tuple_compare_formatted_title (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::FormattedTitle); }
static int tuple_compare_length (const Tuple & a, const Tuple & b)
    { return tuple_compare_int (a, b, Tuple::Length); }
static int tuple_compare_comment (const Tuple & a, const Tuple & b)
    { return tuple_compare_string (a, b, Tuple::Comment); }

static const Playlist::StringCompareFunc filename_comparisons[] = {
    str_compare_encoded,  // path
    filename_compare_basename,  // filename
    nullptr,  // title
    nullptr,  // album
    nullptr,  // artist
    nullptr,  // album artist
    nullptr,  // date
    nullptr,  // genre
    nullptr,  // track
    nullptr,  // formatted title
    nullptr,  // length
    nullptr   // comment
};

static const Playlist::TupleCompareFunc tuple_comparisons[] = {
    nullptr,  // path
    nullptr,  // filename
    tuple_compare_title,
    tuple_compare_album,
    tuple_compare_artist,
    tuple_compare_album_artist,
    tuple_compare_date,
    tuple_compare_genre,
    tuple_compare_track,
    tuple_compare_formatted_title,
    tuple_compare_length,
    tuple_compare_comment
};

static_assert (aud::n_elems (filename_comparisons) == Playlist::n_sort_types &&
 aud::n_elems (tuple_comparisons) == Playlist::n_sort_types,
 "Update playlist comparison functions");

EXPORT void Playlist::sort_entries (SortType scheme) const
{
    if (filename_comparisons[scheme])
        sort_by_filename (filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme])
        sort_by_tuple (tuple_comparisons[scheme]);
}

EXPORT void Playlist::sort_selected (SortType scheme) const
{
    if (filename_comparisons[scheme])
        sort_selected_by_filename (filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme])
        sort_selected_by_tuple (tuple_comparisons[scheme]);
}

/* FIXME: this considers empty fields as duplicates */
EXPORT void Playlist::remove_duplicates (SortType scheme) const
{
    int entries = n_entries ();
    if (entries < 1)
        return;

    select_all (false);

    if (filename_comparisons[scheme])
    {
        StringCompareFunc compare = filename_comparisons[scheme];

        sort_by_filename (compare);
        String last = entry_filename (0);

        for (int i = 1; i < entries; i ++)
        {
            String current = entry_filename (i);

            if (compare (last, current) == 0)
                select_entry (i, true);

            last = current;
        }
    }
    else if (tuple_comparisons[scheme])
    {
        TupleCompareFunc compare = tuple_comparisons[scheme];

        sort_by_tuple (compare);
        Tuple last = entry_tuple (0);

        for (int i = 1; i < entries; i ++)
        {
            Tuple current = entry_tuple (i);

            if (last.valid () && current.valid () && compare (last, current) == 0)
                select_entry (i, true);

            last = std::move (current);
        }
    }

    remove_selected ();
}

EXPORT void Playlist::remove_unavailable () const
{
    int entries = n_entries ();

    select_all (false);

    for (int i = 0; i < entries; i ++)
    {
        String filename = entry_filename (i);

        /* use VFS_NO_ACCESS since VFS_EXISTS doesn't distinguish between
         * inaccessible files and URI schemes that don't support file_test() */
        if (VFSFile::test_file (filename, VFS_NO_ACCESS))
            select_entry (i, true);
    }

    remove_selected ();
}

EXPORT void Playlist::select_by_patterns (const Tuple & patterns) const
{
    int entries = n_entries ();

    select_all (true);

    for (Tuple::Field field : {Tuple::Title, Tuple::Album, Tuple::Artist, Tuple::Basename})
    {
        String pattern = patterns.get_str (field);
        GRegex * regex;

        if (! pattern || ! pattern[0] || ! (regex = g_regex_new (pattern,
         G_REGEX_CASELESS, (GRegexMatchFlags) 0, nullptr)))
            continue;

        for (int i = 0; i < entries; i ++)
        {
            if (! entry_selected (i))
                continue;

            Tuple tuple = entry_tuple (i);
            String string = tuple.get_str (field);

            if (! string || ! g_regex_match (regex, string, (GRegexMatchFlags) 0, nullptr))
                select_entry (i, false);
        }

        g_regex_unref (regex);
    }
}

static StringBuf make_playlist_path (int playlist)
{
    if (! playlist)
        return filename_build ({aud_get_path (AudPath::UserDir), "playlist.xspf"});

    StringBuf name = str_printf ("playlist_%02d.xspf", 1 + playlist);
    name.steal (filename_build ({aud_get_path (AudPath::UserDir), name}));
    return name;
}

static void load_playlists_real ()
{
    const char * folder = aud_get_path (AudPath::PlaylistDir);

    /* old (v3.1 and earlier) naming scheme */

    int count;
    for (count = 0; ; count ++)
    {
        StringBuf path = make_playlist_path (count);
        if (! g_file_test (path, G_FILE_TEST_EXISTS))
            break;

        PlaylistEx playlist = Playlist::insert_playlist (count);
        playlist.insert_flat_playlist (filename_to_uri (path));
        playlist.set_modified (true);
    }

    /* unique ID-based naming scheme */

    StringBuf order_path = filename_build ({folder, "order"});
    char * order_string;
    Index<String> order;

    g_file_get_contents (order_path, & order_string, nullptr, nullptr);
    if (! order_string)
        goto DONE;

    order = str_list_to_index (order_string, " ");
    g_free (order_string);

    for (int i = 0; i < order.len (); i ++)
    {
        const String & number = order[i];

        StringBuf name1 = str_concat ({number, ".audpl"});
        StringBuf name2 = str_concat ({number, ".xspf"});

        StringBuf path = filename_build ({folder, name1});
        if (! g_file_test (path, G_FILE_TEST_EXISTS))
            path.steal (filename_build ({folder, name2}));

        PlaylistEx playlist = PlaylistEx::insert_with_stamp (count + i, atoi (number));
        playlist.insert_flat_playlist (filename_to_uri (path));
        playlist.set_modified (g_str_has_suffix (path, ".xspf"));
    }

DONE:
    if (! Playlist::n_playlists ())
        Playlist::insert_playlist (0);
}

static void save_playlists_real ()
{
    int lists = Playlist::n_playlists ();
    const char * folder = aud_get_path (AudPath::PlaylistDir);

    /* save playlists */

    Index<String> order;
    SimpleHash<String, bool> saved;

    for (int i = 0; i < lists; i ++)
    {
        PlaylistEx playlist = Playlist::by_index (i);
        StringBuf number = int_to_str (playlist.stamp ());
        StringBuf name = str_concat ({number, ".audpl"});

        if (playlist.get_modified ())
        {
            StringBuf path = filename_build ({folder, name});
            playlist.save_to_file (filename_to_uri (path), Playlist::NoWait);
            playlist.set_modified (false);
        }

        order.append (String (number));
        saved.add (String (name), true);
    }

    StringBuf order_string = index_to_str_list (order, " ");
    StringBuf order_path = filename_build ({folder, "order"});

    char * old_order_string;
    g_file_get_contents (order_path, & old_order_string, nullptr, nullptr);

    if (! old_order_string || strcmp (old_order_string, order_string))
    {
        GError * error = nullptr;
        if (! g_file_set_contents (order_path, order_string, -1, & error))
        {
            AUDERR ("Cannot write to %s: %s\n", (const char *) order_path, error->message);
            g_error_free (error);
        }
    }

    g_free (old_order_string);

    /* clean up deleted playlists and files from old naming scheme */

    g_unlink (make_playlist_path (0));

    GDir * dir = g_dir_open (folder, 0, nullptr);
    if (! dir)
        return;

    const char * name;
    while ((name = g_dir_read_name (dir)))
    {
        if (! g_str_has_suffix (name, ".audpl") && ! g_str_has_suffix (name, ".xspf"))
            continue;

        if (! saved.lookup (String (name)))
            g_unlink (filename_build ({folder, name}));
    }

    g_dir_close (dir);
}

static bool hooks_added, state_changed;

static void update_cb (void * data, void *)
{
    auto level = aud::from_ptr<Playlist::UpdateLevel> (data);
    if (level >= Playlist::Metadata)
        state_changed = true;
}

static void state_cb (void * data, void * user)
{
    state_changed = true;
}

void load_playlists ()
{
    load_playlists_real ();
    playlist_load_state ();

    state_changed = false;

    if (! hooks_added)
    {
        hook_associate ("playlist update", update_cb, nullptr);
        hook_associate ("playlist activate", state_cb, nullptr);
        hook_associate ("playlist position", state_cb, nullptr);

        hooks_added = true;
    }
}

void save_playlists (bool exiting)
{
    save_playlists_real ();

    /* on exit, save resume states */
    if (state_changed || exiting)
    {
        playlist_save_state ();
        state_changed = false;
    }

    if (exiting && hooks_added)
    {
        hook_dissociate ("playlist update", update_cb);
        hook_dissociate ("playlist activate", state_cb);
        hook_dissociate ("playlist position", state_cb);

        hooks_added = false;
    }
}
