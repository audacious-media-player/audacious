/*
 * playlist-files.c
 * Copyright 2010-2011 John Lindgren
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
#include <string.h>

#include "debug.h"
#include "misc.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"

static const char * get_extension (const char * filename, bool_t quiet)
{
    const char * s = strrchr (filename, '/');
    if (! s)
        goto FAIL;

    const char * p = strrchr (s + 1, '.');
    if (! p)
        goto FAIL;

    return p + 1;

FAIL:
    if (! quiet)
        fprintf (stderr, "Failed to parse playlist filename %s.\n", filename);
    return NULL;
}

bool_t filename_is_playlist (const char * filename)
{
    const char * ext = get_extension (filename, TRUE);
    if (! ext)
        return FALSE;

    return playlist_plugin_for_extension (ext) ? TRUE : FALSE;
}

static PlaylistPlugin * get_plugin (const char * filename)
{
    const char * ext = get_extension (filename, FALSE);
    if (! ext)
        return NULL;

    PluginHandle * plugin = playlist_plugin_for_extension (ext);
    if (! plugin)
    {
        fprintf (stderr, "Unrecognized playlist file type \"%s\".\n", ext);
        return NULL;
    }

    return plugin_get_header (plugin);
}

bool_t playlist_load (const char * filename, char * * title,
 Index * * filenames_p, Index * * tuples_p)
{
    AUDDBG ("Loading playlist %s.\n", filename);
    PlaylistPlugin * pp = get_plugin (filename);
    g_return_val_if_fail (pp && PLUGIN_HAS_FUNC (pp, load), FALSE);

    VFSFile * file = vfs_fopen (filename, "r");
    if (! file)
        return FALSE;

    Index * filenames = index_new ();
    Index * tuples = index_new ();
    bool_t success = pp->load (filename, file, title, filenames, tuples);

    vfs_fclose (file);

    if (! success)
    {
        index_free (filenames);
        index_free (tuples);
        return FALSE;
    }

    if (index_count (tuples))
        g_return_val_if_fail (index_count (tuples) == index_count (filenames),
         FALSE);
    else
    {
        index_free (tuples);
        tuples = NULL;
    }

    * filenames_p = filenames;
    * tuples_p = tuples;
    return TRUE;
}

bool_t playlist_insert_playlist_raw (int list, int at,
 const char * filename)
{
    char * title = NULL;
    Index * filenames, * tuples;

    if (! playlist_load (filename, & title, & filenames, & tuples))
        return FALSE;

    if (title && ! playlist_entry_count (list))
        playlist_set_title (list, title);

    playlist_entry_insert_batch_raw (list, at, filenames, tuples, NULL);

    str_unref (title);
    return TRUE;
}

bool_t playlist_save (int list, const char * filename)
{
    AUDDBG ("Saving playlist %s.\n", filename);
    PlaylistPlugin * pp = get_plugin (filename);
    g_return_val_if_fail (pp && PLUGIN_HAS_FUNC (pp, save), FALSE);

    bool_t fast = get_bool (NULL, "metadata_on_play");

    VFSFile * file = vfs_fopen (filename, "w");
    if (! file)
        return FALSE;

    char * title = playlist_get_title (list);

    int entries = playlist_entry_count (list);
    Index * filenames = index_new ();
    index_allocate (filenames, entries);
    Index * tuples = index_new ();
    index_allocate (tuples, entries);

    for (int i = 0; i < entries; i ++)
    {
        index_append (filenames, playlist_entry_get_filename (list, i));
        index_append (tuples, playlist_entry_get_tuple (list, i, fast));
    }

    bool_t success = pp->save (filename, file, title, filenames, tuples);

    vfs_fclose (file);
    str_unref (title);

    for (int i = 0; i < entries; i ++)
    {
        str_unref (index_get (filenames, i));
        Tuple * tuple = index_get (tuples, i);
        if (tuple)
            tuple_unref (tuple);
    }

    index_free (filenames);
    index_free (tuples);

    return success;
}
