/*
 * drct.c
 * Copyright 2009-2013 John Lindgren
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

#include "drct.h"

#include "hook.h"
#include "i18n.h"
#include "internal.h"
#include "playlist-internal.h"
#include "plugins-internal.h"
#include "runtime.h"
#include "tuple.h"

/* --- PLAYBACK CONTROL --- */

EXPORT void aud_drct_play ()
{
    if (aud_drct_get_playing ())
    {
        if (aud_drct_get_paused ())
            aud_drct_pause ();
        else
        {
            int a, b;
            aud_drct_get_ab_repeat (a, b);
            aud_drct_seek (aud::max (a, 0));
        }
    }
    else
    {
        int playlist = aud_playlist_get_active ();
        aud_playlist_set_position (playlist, aud_playlist_get_position (playlist));
        aud_playlist_play (playlist);
    }
}

EXPORT void aud_drct_play_pause ()
{
    if (aud_drct_get_playing ())
        aud_drct_pause ();
    else
        aud_drct_play ();
}

EXPORT void aud_drct_stop ()
{
    aud_playlist_play (-1);
}

EXPORT int aud_drct_get_position ()
{
    int playlist = aud_playlist_get_playing ();
    return aud_playlist_get_position (playlist);
}

EXPORT String aud_drct_get_filename ()
{
    int playlist = aud_playlist_get_playing ();
    int position = aud_playlist_get_position (playlist);
    return aud_playlist_entry_get_filename (playlist, position);
}

/* --- RECORDING CONTROL --- */

/* The recording plugin is currently hard-coded to FileWriter.  Someday
 * aud_drct_set_record_plugin() may be added. */

static PluginHandle * record_plugin;

static bool record_plugin_watcher (PluginHandle *, void *)
{
    hook_call ("enable record", nullptr);
    return true;
}

void record_init ()
{
    auto plugin = aud_plugin_lookup_basename ("filewriter");
    if (plugin && aud_plugin_get_type (plugin) == PluginType::Output)
    {
        record_plugin = plugin;
        aud_plugin_add_watch (plugin, record_plugin_watcher, nullptr);
    }
}

void record_cleanup ()
{
    if (record_plugin)
    {
        aud_plugin_remove_watch (record_plugin, record_plugin_watcher, nullptr);
        record_plugin = nullptr;
    }
}

EXPORT PluginHandle * aud_drct_get_record_plugin ()
{
    /* recording is disabled when FileWriter is the primary output plugin */
    if (! record_plugin || plugin_get_enabled (record_plugin) == PluginEnabled::Primary)
        return nullptr;

    return record_plugin;
}

EXPORT bool aud_drct_get_record_enabled ()
{
    return (record_plugin && plugin_get_enabled (record_plugin) == PluginEnabled::Secondary);
}

EXPORT bool aud_drct_enable_record (bool enable)
{
    if (! record_plugin || plugin_get_enabled (record_plugin) == PluginEnabled::Primary)
        return false;

    return plugin_enable_secondary (record_plugin, enable);
}

/* --- VOLUME CONTROL --- */

EXPORT int aud_drct_get_volume_main ()
{
    StereoVolume volume = aud_drct_get_volume ();
    return aud::max (volume.left, volume.right);
}

EXPORT void aud_drct_set_volume_main (int volume)
{
    StereoVolume old = aud_drct_get_volume ();
    int main = aud::max (old.left, old.right);

    if (main > 0)
        aud_drct_set_volume ({
            aud::rescale (old.left, main, volume),
            aud::rescale (old.right, main, volume)
        });
    else
        aud_drct_set_volume ({volume, volume});
}

EXPORT int aud_drct_get_volume_balance ()
{
    StereoVolume volume = aud_drct_get_volume ();

    if (volume.left == volume.right)
        return 0;
    else if (volume.left > volume.right)
        return -100 + aud::rescale (volume.right, volume.left, 100);
    else
        return 100 - aud::rescale (volume.left, volume.right, 100);
}

EXPORT void aud_drct_set_volume_balance (int balance)
{
    int main = aud_drct_get_volume_main ();

    if (balance < 0)
        aud_drct_set_volume ({main, aud::rescale (main, 100, 100 + balance)});
    else
        aud_drct_set_volume ({aud::rescale (main, 100, 100 - balance), main});
}

/* --- PLAYLIST CONTROL --- */

EXPORT void aud_drct_pl_next ()
{
    int playlist = aud_playlist_get_playing ();
    if (playlist < 0)
        playlist = aud_playlist_get_active ();

    playlist_next_song (playlist, aud_get_bool (nullptr, "repeat"));
}

EXPORT void aud_drct_pl_prev ()
{
    int playlist = aud_playlist_get_playing ();
    if (playlist < 0)
        playlist = aud_playlist_get_active ();

    playlist_prev_song (playlist);
}

static void add_list (Index<PlaylistAddItem> && items, int at, bool to_temp, bool play)
{
    if (to_temp)
        aud_playlist_set_active (aud_playlist_get_temporary ());

    aud_playlist_entry_insert_batch (aud_playlist_get_active (), at, std::move (items), play);
}

EXPORT void aud_drct_pl_add (const char * filename, int at)
{
    Index<PlaylistAddItem> items;
    items.append (String (filename));
    add_list (std::move (items), at, false, false);
}

EXPORT void aud_drct_pl_add_list (Index<PlaylistAddItem> && items, int at)
{
    add_list (std::move (items), at, false, false);
}

EXPORT void aud_drct_pl_open (const char * filename)
{
    Index<PlaylistAddItem> items;
    items.append (String (filename));
    add_list (std::move (items), -1, aud_get_bool (nullptr, "open_to_temporary"), true);
}

EXPORT void aud_drct_pl_open_list (Index<PlaylistAddItem> && items)
{
    add_list (std::move (items), -1, aud_get_bool (nullptr, "open_to_temporary"), true);
}

EXPORT void aud_drct_pl_open_temp (const char * filename)
{
    Index<PlaylistAddItem> items;
    items.append (String (filename));
    add_list (std::move (items), -1, true, true);
}

EXPORT void aud_drct_pl_open_temp_list (Index<PlaylistAddItem> && items)
{
    add_list (std::move (items), -1, true, true);
}
