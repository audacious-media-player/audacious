/*
 * libaudgui/urilist.c
 * Copyright 2010 John Lindgren
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

#include <string.h>
#include <glib.h>

#include <audacious/drct.h>
#include <audacious/playlist.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/vfs.h>

#include "libaudgui.h"

typedef void (* ForEachFunc) (char *, void *);

static char * check_uri (char * name)
{
    char * new;

    if (strstr (name, "://") || ! (new = filename_to_uri (name)))
        return name;

    g_free (name);
    return new;
}

static void urilist_for_each (const char * list, ForEachFunc func, void * user)
{
    const char * end, * next;

    while (list[0])
    {
        if ((end = strstr (list, "\r\n")))
            next = end + 2;
        else if ((end = strchr (list, '\n')))
            next = end + 1;
        else
            next = end = strchr (list, 0);

        func (check_uri (g_strndup (list, end - list)), user);
        list = next;
    }
}

static void add_to_index (char * name, Index * index)
{
    index_append (index, str_get (name));
    g_free (name);
}

void audgui_urilist_open (const char * list)
{
    Index * filenames = index_new ();
    urilist_for_each (list, (ForEachFunc) add_to_index, filenames);
    aud_drct_pl_open_list (filenames);
}

void audgui_urilist_insert (int playlist, int at, const char * list)
{
    Index * filenames = index_new ();
    urilist_for_each (list, (ForEachFunc) add_to_index, filenames);
    aud_playlist_entry_insert_batch (playlist, at, filenames, NULL, FALSE);
}

char * audgui_urilist_create_from_selected (int playlist)
{
    int entries = aud_playlist_entry_count (playlist);
    int space = 0;
    int count, length;
    char * name;
    char * buffer, * set;

    for (count = 0; count < entries; count ++)
    {
        if (! aud_playlist_entry_get_selected (playlist, count))
            continue;

        name = aud_playlist_entry_get_filename (playlist, count);
        g_return_val_if_fail (name != NULL, NULL);
        space += strlen (name) + 1;
        str_unref (name);
    }

    if (! space)
        return NULL;

    buffer = g_malloc (space);
    set = buffer;

    for (count = 0; count < entries; count ++)
    {
        if (! aud_playlist_entry_get_selected (playlist, count))
            continue;

        name = aud_playlist_entry_get_filename (playlist, count);
        g_return_val_if_fail (name != NULL, NULL);
        length = strlen (name);
        g_return_val_if_fail (length + 1 <= space, NULL);
        memcpy (set, name, length);
        set += length;
        * set ++ = '\n';
        space -= length + 1;
        str_unref (name);
    }

    * -- set = 0; /* last newline replaced with null */
    return buffer;
}
