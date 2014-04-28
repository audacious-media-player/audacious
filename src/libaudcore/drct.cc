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

#include "i18n.h"
#include "internal.h"
#include "playlist-internal.h"
#include "runtime.h"

/* --- PLAYBACK CONTROL --- */

EXPORT void aud_drct_play (void)
{
    if (aud_drct_get_playing ())
    {
        if (aud_drct_get_paused ())
            aud_drct_pause ();
        else
        {
            int a, b;
            aud_drct_get_ab_repeat (& a, & b);
            aud_drct_seek (MAX (a, 0));
        }
    }
    else
    {
        int playlist = aud_playlist_get_active ();
        aud_playlist_set_position (playlist, aud_playlist_get_position (playlist));
        aud_drct_play_playlist (playlist);
    }
}

EXPORT void aud_drct_play_pause (void)
{
    if (aud_drct_get_playing ())
        aud_drct_pause ();
    else
        aud_drct_play ();
}

EXPORT void aud_drct_play_playlist (int playlist)
{
    aud_playlist_set_playing (playlist);
    if (aud_drct_get_paused ())
        aud_drct_pause ();
}

EXPORT void aud_drct_stop (void)
{
    aud_playlist_set_playing (-1);
}

/* --- VOLUME CONTROL --- */

EXPORT void aud_drct_get_volume_main (int * volume)
{
    int left, right;
    aud_drct_get_volume (& left, & right);
    * volume = MAX (left, right);
}

EXPORT void aud_drct_set_volume_main (int volume)
{
    int left, right, current;
    aud_drct_get_volume (& left, & right);
    current = MAX (left, right);

    if (current > 0)
        aud_drct_set_volume (volume * left / current, volume * right / current);
    else
        aud_drct_set_volume (volume, volume);
}

EXPORT void aud_drct_get_volume_balance (int * balance)
{
    int left, right;
    aud_drct_get_volume (& left, & right);

    if (left == right)
        * balance = 0;
    else if (left > right)
        * balance = -100 + right * 100 / left;
    else
        * balance = 100 - left * 100 / right;
}

EXPORT void aud_drct_set_volume_balance (int balance)
{
    int left, right;
    aud_drct_get_volume_main (& left);

    if (balance < 0)
        right = left * (100 + balance) / 100;
    else
    {
        right = left;
        left = right * (100 - balance) / 100;
    }

    aud_drct_set_volume (left, right);
}

/* --- PLAYLIST CONTROL --- */

EXPORT void aud_drct_pl_next (void)
{
    int playlist = aud_playlist_get_playing ();
    if (playlist < 0)
        playlist = aud_playlist_get_active ();

    playlist_next_song (playlist, aud_get_bool (NULL, "repeat"));
}

EXPORT void aud_drct_pl_prev (void)
{
    int playlist = aud_playlist_get_playing ();
    if (playlist < 0)
        playlist = aud_playlist_get_active ();

    playlist_prev_song (playlist);
}

static void add_list (Index<PlaylistAddItem> && items, int at, bool_t to_temp, bool_t play)
{
    if (to_temp)
        aud_playlist_set_active (aud_playlist_get_temporary ());

    int playlist = aud_playlist_get_active ();

    /* queue the new entries before deleting the old ones */
    /* this is to avoid triggering the --quit-after-play condition */
    aud_playlist_entry_insert_batch (playlist, at, std::move (items), play);

    if (play)
    {
        if (aud_get_bool (NULL, "clear_playlist"))
            aud_playlist_entry_delete (playlist, 0, aud_playlist_entry_count (playlist));
        else
            aud_playlist_queue_delete (playlist, 0, aud_playlist_queue_count (playlist));
    }
}

EXPORT void aud_drct_pl_add (const char * filename, int at)
{
    Index<PlaylistAddItem> items;
    items.append ({str_get (filename)});
    add_list (std::move (items), at, FALSE, FALSE);
}

EXPORT void aud_drct_pl_add_list (Index<PlaylistAddItem> && items, int at)
{
    add_list (std::move (items), at, FALSE, FALSE);
}

EXPORT void aud_drct_pl_open (const char * filename)
{
    Index<PlaylistAddItem> items;
    items.append ({str_get (filename)});
    add_list (std::move (items), -1, aud_get_bool (NULL, "open_to_temporary"), TRUE);
}

EXPORT void aud_drct_pl_open_list (Index<PlaylistAddItem> && items)
{
    add_list (std::move (items), -1, aud_get_bool (NULL, "open_to_temporary"), TRUE);
}

EXPORT void aud_drct_pl_open_temp (const char * filename)
{
    Index<PlaylistAddItem> items;
    items.append ({str_get (filename)});
    add_list (std::move (items), -1, TRUE, TRUE);
}

EXPORT void aud_drct_pl_open_temp_list (Index<PlaylistAddItem> && items)
{
    add_list (std::move (items), -1, TRUE, TRUE);
}
