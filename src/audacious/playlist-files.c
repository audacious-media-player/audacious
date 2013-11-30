/*
 * playlist-files.c
 * Copyright 2010-2013 John Lindgren
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

#include <glib.h>
#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"

typedef struct
{
    const char * filename;
    char * title;
    Index * filenames;
    Index * tuples;
    bool_t plugin_found;
    bool_t success;
}
PlaylistData;

static void plugin_for_filename (const char * filename, PluginForEachFunc func, void * data)
{
    char ext[32];
    if (uri_get_extension (filename, ext, sizeof ext))
        playlist_plugin_for_ext (ext, func, data);
}

static bool_t plugin_found_cb (PluginHandle * plugin, void * data)
{
    * (PluginHandle * *) data = plugin;
    return FALSE; /* stop when first plugin is found */
}

bool_t filename_is_playlist (const char * filename)
{
    PluginHandle * plugin = NULL;
    plugin_for_filename (filename, plugin_found_cb, & plugin);
    return (plugin != NULL);
}

static bool_t playlist_load_cb (PluginHandle * plugin, void * data_)
{
    PlaylistData * data = (PlaylistData *) data_;

    PlaylistPlugin * pp = plugin_get_header (plugin);
    if (! pp || ! PLUGIN_HAS_FUNC (pp, load))
        return TRUE; /* try another plugin */

    data->plugin_found = TRUE;

    VFSFile * file = vfs_fopen (data->filename, "r");
    if (! file)
        return FALSE; /* stop if we can't open file */

    data->success = pp->load (data->filename, file, & data->title, data->filenames, data->tuples);

    vfs_fclose (file);
    return ! data->success; /* stop when playlist is loaded */
}

bool_t playlist_load (const char * filename, char * * title, Index * * filenames, Index * * tuples)
{
    PlaylistData data =
    {
        .filename = filename,
        .filenames = index_new (),
        .tuples = index_new ()
    };

    AUDDBG ("Loading playlist %s.\n", filename);
    plugin_for_filename (filename, playlist_load_cb, & data);

    if (! data.plugin_found)
    {
        SPRINTF (error, _("Cannot load %s: unsupported file extension."), filename);
        interface_show_error (error);
    }

    if (! data.success)
    {
        str_unref (data.title);
        index_free_full (data.filenames, (IndexFreeFunc) str_unref);
        index_free_full (data.tuples, (IndexFreeFunc) tuple_unref);
        return FALSE;
    }

    if (index_count (data.tuples))
        g_return_val_if_fail (index_count (data.tuples) == index_count (data.filenames), FALSE);
    else
    {
        index_free (data.tuples);
        data.tuples = NULL;
    }

    * title = data.title;
    * filenames = data.filenames;
    * tuples = data.tuples;
    return TRUE;
}

bool_t playlist_insert_playlist_raw (int list, int at, const char * filename)
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

static bool_t playlist_save_cb (PluginHandle * plugin, void * data_)
{
    PlaylistData * data = data_;

    PlaylistPlugin * pp = plugin_get_header (plugin);
    if (! pp || ! PLUGIN_HAS_FUNC (pp, save))
        return TRUE; /* try another plugin */

    data->plugin_found = TRUE;

    VFSFile * file = vfs_fopen (data->filename, "w");
    if (! file)
        return FALSE; /* stop if we can't open file */

    data->success = pp->save (data->filename, file, data->title, data->filenames, data->tuples);

    vfs_fclose (file);
    return FALSE; /* stop after first attempt (successful or not) */
}

bool_t playlist_save (int list, const char * filename)
{
    PlaylistData data =
    {
        .filename = filename,
        .title = playlist_get_title (list),
        .filenames = index_new (),
        .tuples = index_new ()
    };

    int entries = playlist_entry_count (list);
    bool_t fast = get_bool (NULL, "metadata_on_play");

    index_allocate (data.filenames, entries);
    index_allocate (data.tuples, entries);

    for (int i = 0; i < entries; i ++)
    {
        index_insert (data.filenames, -1, playlist_entry_get_filename (list, i));
        index_insert (data.tuples, -1, playlist_entry_get_tuple (list, i, fast));
    }

    AUDDBG ("Saving playlist %s.\n", filename);
    plugin_for_filename (filename, playlist_save_cb, & data);

    if (! data.plugin_found)
    {
        SPRINTF (error, _("Cannot save %s: unsupported file extension."), filename);
        interface_show_error (error);
    }

    str_unref (data.title);
    index_free_full (data.filenames, (IndexFreeFunc) str_unref);
    index_free_full (data.tuples, (IndexFreeFunc) tuple_unref);

    return data.success;
}
