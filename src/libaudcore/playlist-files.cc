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

EXPORT bool Playlist::filename_is_playlist (const char * filename)
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
    bool plugin_found = false;

    if (ext)
    {
        for (PluginHandle * plugin : aud_plugin_list (PluginType::Playlist))
        {
            if (! aud_plugin_get_enabled (plugin) || ! playlist_plugin_has_ext (plugin, ext))
                continue;

            AUDINFO ("Trying playlist plugin %s.\n", aud_plugin_get_name (plugin));
            plugin_found = true;

            auto pp = (PlaylistPlugin *) aud_plugin_get_header (plugin);
            if (! pp)
                continue;

            VFSFile file (filename, "r");
            if (! file)
            {
                aud_ui_show_error (str_printf (_("Error opening %s:\n%s"),
                 filename, file.error ()));
                return false;
            }

            if (pp->load (filename, file, title, items))
                return true;

            title = String ();
            items.clear ();
        }
    }

    if (plugin_found)
        aud_ui_show_error (str_printf (_("Error loading %s."), filename));
    else
        aud_ui_show_error (str_printf (_("Cannot load %s: unsupported file "
         "name extension."), filename));

    return false;
}

// This procedure is only used when loading playlists from ~/.config/audacious;
// hence, it is drastically simpler than the full-featured routines in adder.cc.
// All support for adding folders, cuesheets, subtunes, etc. is omitted here.
// Additionally, in order to avoid heavy I/O at startup, failed entries are not
// rescanned; they can be rescanned later by refreshing the playlist. */
bool PlaylistEx::insert_flat_playlist (const char * filename) const
{
    String title;
    Index<PlaylistAddItem> items;

    if (! playlist_load (filename, title, items))
        return false;

    if (title)
        set_title (title);

    insert_flat_items (0, std::move (items));

    return true;
}

EXPORT bool Playlist::save_to_file (const char * filename, GetMode mode) const
{
    String title = get_title ();

    Index<PlaylistAddItem> items;
    items.insert (0, n_entries ());

    int i = 0;
    for (PlaylistAddItem & item : items)
    {
        item.filename = entry_filename (i);
        item.tuple = entry_tuple (i, mode);
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

EXPORT Index<Playlist::SaveFormat> Playlist::save_formats ()
{
    Index<Playlist::SaveFormat> formats;

    for (auto plugin : aud_plugin_list (PluginType::Playlist))
    {
        if (! aud_plugin_get_enabled (plugin) || ! playlist_plugin_can_save (plugin))
            continue;

        auto & format = formats.append ();
        format.name = String (aud_plugin_get_name (plugin));

        for (auto & ext : playlist_plugin_get_exts (plugin))
            format.exts.append (ext);
    }

    return formats;
}
