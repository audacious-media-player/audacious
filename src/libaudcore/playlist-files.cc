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

#include "playlist-internal.h"

#include "audstrings.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "plugins-internal.h"
#include "runtime.h"

struct PlaylistData {
    const char * filename;
    String title;
    Index<PlaylistAddItem> items;
    bool plugin_found;
    bool success;
};

static void plugin_for_filename (const char * filename, PluginForEachFunc func, void * data)
{
    StringBuf ext = uri_get_extension (filename);
    if (ext)
        playlist_plugin_for_ext (ext, func, data);
}

static bool plugin_found_cb (PluginHandle * plugin, void * data)
{
    * (PluginHandle * *) data = plugin;
    return false; /* stop when first plugin is found */
}

EXPORT bool aud_filename_is_playlist (const char * filename)
{
    PluginHandle * plugin = nullptr;
    plugin_for_filename (filename, plugin_found_cb, & plugin);
    return (plugin != nullptr);
}

static bool playlist_load_cb (PluginHandle * plugin, void * data_)
{
    PlaylistData * data = (PlaylistData *) data_;

    AUDINFO ("Trying playlist plugin %s.\n", aud_plugin_get_name (plugin));

    PlaylistPlugin * pp = (PlaylistPlugin *) aud_plugin_get_header (plugin);
    if (! pp || ! PLUGIN_HAS_FUNC (pp, load))
        return true; /* try another plugin */

    data->plugin_found = true;

    VFSFile * file = vfs_fopen (data->filename, "r");
    if (! file)
        return false; /* stop if we can't open file */

    data->success = pp->load (data->filename, file, data->title, data->items);

    vfs_fclose (file);
    return ! data->success; /* stop when playlist is loaded */
}

bool playlist_load (const char * filename, String & title, Index<PlaylistAddItem> & items)
{
    PlaylistData data = {
        filename
    };

    AUDINFO ("Loading playlist %s.\n", filename);
    plugin_for_filename (filename, playlist_load_cb, & data);

    if (! data.plugin_found)
        aud_ui_show_error (str_printf (_("Cannot load %s: unsupported file extension."), filename));

    if (! data.success)
        return false;

    title = std::move (data.title);
    items = std::move (data.items);
    return true;
}

bool playlist_insert_playlist_raw (int list, int at, const char * filename)
{
    String title;
    Index<PlaylistAddItem> items;

    if (! playlist_load (filename, title, items))
        return false;

    if (title && ! aud_playlist_entry_count (list))
        aud_playlist_set_title (list, title);

    playlist_entry_insert_batch_raw (list, at, std::move (items));

    return true;
}

static bool playlist_save_cb (PluginHandle * plugin, void * data_)
{
    PlaylistData * data = (PlaylistData *) data_;

    PlaylistPlugin * pp = (PlaylistPlugin *) aud_plugin_get_header (plugin);
    if (! pp || ! PLUGIN_HAS_FUNC (pp, save))
        return true; /* try another plugin */

    data->plugin_found = true;

    VFSFile * file = vfs_fopen (data->filename, "w");
    if (! file)
        return false; /* stop if we can't open file */

    data->success = pp->save (data->filename, file, data->title, data->items);

    vfs_fclose (file);
    return false; /* stop after first attempt (successful or not) */
}

EXPORT bool aud_playlist_save (int list, const char * filename)
{
    PlaylistData data = {
        filename,
        aud_playlist_get_title (list)
    };

    int entries = aud_playlist_entry_count (list);
    bool fast = aud_get_bool (nullptr, "metadata_on_play");

    data.items.insert (0, entries);

    for (int i = 0; i < entries; i ++)
    {
        data.items[i].filename = aud_playlist_entry_get_filename (list, i);
        data.items[i].tuple = aud_playlist_entry_get_tuple (list, i, fast);
    }

    AUDINFO ("Saving playlist %s.\n", filename);
    plugin_for_filename (filename, playlist_save_cb, & data);

    if (! data.plugin_found)
        aud_ui_show_error (str_printf (_("Cannot save %s: unsupported file extension."), filename));

    return data.success;
}
