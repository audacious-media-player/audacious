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

EXPORT bool aud_filename_is_playlist (const char * filename)
{
    StringBuf ext = uri_get_extension (filename);

    if (ext)
    {
        for (PluginHandle * plugin : aud_plugin_list (PluginType::Playlist))
        {
            if (aud_plugin_get_enabled (plugin) && playlist_plugin_has_ext (plugin, ext))
                return true;
        }
    }

    return false;
}

bool playlist_load (const char * filename, String & title, Index<PlaylistAddItem> & items)
{
    AUDINFO ("Loading playlist %s.\n", filename);

    StringBuf ext = uri_get_extension (filename);

    if (ext)
    {
        for (PluginHandle * plugin : aud_plugin_list (PluginType::Playlist))
        {
            if (! aud_plugin_get_enabled (plugin) || ! playlist_plugin_has_ext (plugin, ext))
                continue;

            AUDINFO ("Trying playlist plugin %s.\n", aud_plugin_get_name (plugin));

            PlaylistPlugin * pp = (PlaylistPlugin *) aud_plugin_get_header (plugin);
            if (! pp)
                continue;

            VFSFile file (filename, "r");
            if (! file)
                return false;

            if (pp->load (filename, file, title, items))
                return true;

            title = String ();
            items.clear ();
        }
    }

    aud_ui_show_error (str_printf (_("Cannot load %s: unsupported file name extension."), filename));

    return false;
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

EXPORT bool aud_playlist_save (int list, const char * filename, Playlist::GetMode mode)
{
    String title = aud_playlist_get_title (list);

    Index<PlaylistAddItem> items;
    items.insert (0, aud_playlist_entry_count (list));

    int i = 0;
    for (PlaylistAddItem & item : items)
    {
        item.filename = aud_playlist_entry_get_filename (list, i);
        item.tuple = aud_playlist_entry_get_tuple (list, i, mode);
        item.tuple.delete_fallbacks ();
        i ++;
    }

    AUDINFO ("Saving playlist %s.\n", filename);

    StringBuf ext = uri_get_extension (filename);

    if (ext)
    {
        for (PluginHandle * plugin : aud_plugin_list (PluginType::Playlist))
        {
            if (! aud_plugin_get_enabled (plugin) || ! playlist_plugin_has_ext (plugin, ext))
                continue;

            PlaylistPlugin * pp = (PlaylistPlugin *) aud_plugin_get_header (plugin);
            if (! pp || ! pp->can_save)
                continue;

            VFSFile file (filename, "w");
            if (! file)
                return false;

            return pp->save (filename, file, title, items) && file.fflush () == 0;
        }
    }

    aud_ui_show_error (str_printf (_("Cannot save %s: unsupported file name extension."), filename));

    return false;
}
