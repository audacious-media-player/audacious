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
#include <glib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/playlist.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "libaudgui.h"

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
        if ((end = strstr (list, "\r\n")))
            next = end + 2;
        else if ((end = strchr (list, '\n')))
            next = end + 1;
        else
            next = end = strchr (list, 0);

        index.append ({check_uri (str_copy (list, end - list))});
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
    aud_playlist_entry_insert_batch (playlist, at, urilist_to_index (list), FALSE);
}

EXPORT char * audgui_urilist_create_from_selected (int playlist)
{
    int entries = aud_playlist_entry_count (playlist);
    int space = 0;
    int count, length;
    char * buffer, * set;

    for (count = 0; count < entries; count ++)
    {
        if (! aud_playlist_entry_get_selected (playlist, count))
            continue;

        String name = aud_playlist_entry_get_filename (playlist, count);
        g_return_val_if_fail (name != nullptr, nullptr);
        space += strlen (name) + 1;
    }

    if (! space)
        return nullptr;

    buffer = g_new (char, space);
    set = buffer;

    for (count = 0; count < entries; count ++)
    {
        if (! aud_playlist_entry_get_selected (playlist, count))
            continue;

        String name = aud_playlist_entry_get_filename (playlist, count);
        g_return_val_if_fail (name != nullptr, nullptr);
        length = strlen (name);
        g_return_val_if_fail (length + 1 <= space, nullptr);
        memcpy (set, name, length);
        set += length;
        * set ++ = '\n';
        space -= length + 1;
    }

    * -- set = 0; /* last newline replaced with null */
    return buffer;
}
