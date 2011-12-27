/*
 * art.c
 * Copyright 2011 John Lindgren
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

#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "main.h"
#include "misc.h"
#include "playlist.h"
#include "util.h"

typedef struct {
    char * song_file; /* pooled */
    int refcount;

    /* album art as JPEG or PNG data */
    void * data;
    int64_t len;

    /* album art as (possibly a temporary) file */
    char * art_file;
    bool_t is_temp;
} ArtItem;

static GHashTable * art_items;
static char * current_file; /* pooled */

static void art_item_free (ArtItem * item)
{
    /* delete temporary file */
    if (item->art_file && item->is_temp)
    {
        char * unixname = uri_to_filename (item->art_file);
        if (unixname)
        {
            unlink (unixname);
            g_free (unixname);
        }
    }

    str_unref (item->song_file);
    g_free (item->data);
    g_free (item->art_file);
    g_slice_free (ArtItem, item);
}

static ArtItem * art_item_new (const char * file)
{
    /* local files only */
    if (strncmp (file, "file://", 7))
        return NULL;

    ArtItem * item = g_slice_new0 (ArtItem);
    item->song_file = str_get (file);

    /* try to load embedded album art */
    PluginHandle * decoder = file_find_decoder (file, FALSE);
    if (decoder)
        file_read_image (file, decoder, & item->data, & item->len);

    if (item->data)
        return item;

    /* try to find external image file */
    char * unixname = get_associated_image_file (file);
    if (unixname)
    {
        item->art_file = filename_to_uri (unixname);
        g_free (unixname);
    }

    if (item->art_file)
        return item;

    /* failed */
    art_item_free (item);
    return NULL;
}

static ArtItem * art_item_get (const char * file)
{
    if (! art_items)
        art_items = g_hash_table_new_full (g_str_hash, g_str_equal,
         NULL, (GDestroyNotify) art_item_free);

    ArtItem * item = g_hash_table_lookup (art_items, file);
    if (item)
    {
        item->refcount ++;
        return item;
    }

    item = art_item_new (file);
    if (! item)
        return NULL;

    g_hash_table_insert (art_items, item->song_file, item);
    item->refcount = 1;
    return item;
}

static void art_item_unref (ArtItem * item)
{
    if (! -- item->refcount)
    {
        /* keep album art for current entry */
        if (current_file && ! strcmp (current_file, item->song_file))
            return;

        g_hash_table_remove (art_items, item->song_file);
    }
}

static void release_current (void)
{
    if (! art_items || ! current_file)
        return;

    /* free album art for previous entry */
    ArtItem * item = g_hash_table_lookup (art_items, current_file);
    if (item && ! item->refcount)
        g_hash_table_remove (art_items, current_file);
}

static void position_hook (void * data, void * user)
{
    release_current ();
    str_unref (current_file);

    int list = playlist_get_playing ();
    int entry = (list >= 0) ? playlist_get_position (list) : -1;
    current_file = (entry >= 0) ? playlist_entry_get_filename (list, entry) : NULL;
}

void art_init (void)
{
    hook_associate ("playlist position", position_hook, NULL);
    hook_associate ("playlist set playing", position_hook, NULL);
}

void art_cleanup (void)
{
    hook_dissociate ("playlist position", position_hook);
    hook_dissociate ("playlist set playing", position_hook);

    release_current ();
    str_unref (current_file);
    current_file = NULL;

    if (art_items && g_hash_table_size (art_items))
    {
        fprintf (stderr, "Album art not freed\n");
        abort ();
    }

    if (art_items)
    {
        g_hash_table_destroy (art_items);
        art_items = NULL;
    }
}

void art_get_data (const char * file, const void * * data, int64_t * len)
{
    * data = NULL;
    * len = 0;

    ArtItem * item = art_item_get (file);
    if (! item)
        return;

    /* load data from external image file */
    if (! item->data && item->art_file)
        vfs_file_get_contents (item->art_file, & item->data, & item->len);

    if (! item->data)
    {
        art_item_unref (item);
        return;
    }

    * data = item->data;
    * len = item->len;
}

const char * art_get_file (const char * file)
{
    ArtItem * item = art_item_get (file);
    if (! item)
        return NULL;

    /* save data to temporary file */
    if (item->data && ! item->art_file)
    {
        char * unixname = write_temp_file (item->data, item->len);
        if (unixname)
        {
            item->art_file = filename_to_uri (unixname);
            item->is_temp = TRUE;
            g_free (unixname);
        }
    }

    if (! item->art_file)
    {
        art_item_unref (item);
        return NULL;
    }

    return item->art_file;
}

void art_unref (const char * file)
{
    ArtItem * item = art_items ? g_hash_table_lookup (art_items, file) : NULL;
    if (item)
        art_item_unref (item);
}
