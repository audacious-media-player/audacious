/*
 * urilist.c
 * Copyright 2010-2011 John Lindgren
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

#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/multihash.h>
#include <libaudcore/playlist.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "libaudgui.h"

static SimpleHash<String, Tuple> tuple_cache;
static QueuedFunc cleanup_timer;

void urilist_cleanup ()
{
    tuple_cache.clear ();
    cleanup_timer.stop ();
}

static String check_uri (const char * name)
{
    if (! strstr (name, "://"))
    {
        StringBuf uri = filename_to_uri (name);
        if (uri)
            return String (uri);
    }

    return String (name);
}

static Index<PlaylistAddItem> urilist_to_index (const char * list)
{
    Index<PlaylistAddItem> index;
    const char * end, * next;

    while (list[0])
    {
        if ((end = strchr (list, '\n')))
        {
            next = end + 1;
            if (end > list && end[-1] == '\r')
                end --;
        }
        else
            next = end = strchr (list, 0);

        String filename = check_uri (str_copy (list, end - list));
        const Tuple * tuple = tuple_cache.lookup (filename);

        index.append (filename, tuple ? tuple->ref () : Tuple ());

        list = next;
    }

    return index;
}

EXPORT void audgui_urilist_open (const char * list)
{
    aud_drct_pl_open_list (urilist_to_index (list));
}

EXPORT void audgui_urilist_insert (int playlist, int at, const char * list)
{
    aud_playlist_entry_insert_batch (playlist, at, urilist_to_index (list), false);
}

EXPORT Index<char> audgui_urilist_create_from_selected (int playlist)
{
    Index<char> buf;
    int entries = aud_playlist_entry_count (playlist);

    for (int count = 0; count < entries; count ++)
    {
        if (aud_playlist_entry_get_selected (playlist, count))
        {
            if (buf.len ())
                buf.append ('\n');

            String filename = aud_playlist_entry_get_filename (playlist, count);
            Tuple tuple = aud_playlist_entry_get_tuple (playlist, count, Playlist::Nothing);

            buf.insert (filename, -1, strlen (filename));
            if (tuple)
                tuple_cache.add (filename, std::move (tuple));
        }
    }

    cleanup_timer.start (30000, [] (void *) { urilist_cleanup (); }, nullptr);

    return buf;
}
