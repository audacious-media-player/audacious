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
#include <libaudcore/playlist.h>

#include "libaudgui.h"

static String check_uri (const char * name)
{
    return strstr (name, "://") ? String (name) : String (filename_to_uri (name));
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

        if (end > list)
            index.append (check_uri (str_copy (list, end - list)));

        list = next;
    }

    return index;
}

EXPORT void audgui_urilist_open (const char * list)
{
    aud_drct_pl_open_list (urilist_to_index (list));
}

EXPORT void audgui_urilist_insert (Playlist playlist, int at, const char * list)
{
    playlist.insert_items (at, urilist_to_index (list), false);
}

EXPORT Index<char> audgui_urilist_create_from_selected (Playlist playlist)
{
    playlist.cache_selected ();

    Index<char> buf;
    int entries = playlist.n_entries ();

    for (int i = 0; i < entries; i ++)
    {
        if (playlist.entry_selected (i))
        {
            if (buf.len ())
                buf.append ('\n');

            String filename = playlist.entry_filename (i);
            buf.insert (filename, -1, strlen (filename));
        }
    }

    return buf;
}
