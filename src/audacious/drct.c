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

#include <libaudcore/hook.h>
#include <libaudcore/vfs.h>

#include "drct.h"
#include "i18n.h"
#include "misc.h"
#include "playlist.h"

/* --- PLAYBACK CONTROL --- */

void drct_play (void)
{
    if (drct_get_playing ())
    {
        if (drct_get_paused ())
            drct_pause ();
        else
        {
            int a, b;
            drct_get_ab_repeat (& a, & b);
            drct_seek (MAX (a, 0));
        }
    }
    else
    {
        int playlist = playlist_get_active ();
        playlist_set_position (playlist, playlist_get_position (playlist));
        drct_play_playlist (playlist);
    }
}

void drct_play_pause (void)
{
    if (drct_get_playing ())
        drct_pause ();
    else
        drct_play ();
}

void drct_play_playlist (int playlist)
{
    playlist_set_playing (playlist);
    if (drct_get_paused ())
        drct_pause ();
}

void drct_stop (void)
{
    playlist_set_playing (-1);
}

/* --- VOLUME CONTROL --- */

void drct_get_volume_main (int * volume)
{
    int left, right;
    drct_get_volume (& left, & right);
    * volume = MAX (left, right);
}

void drct_set_volume_main (int volume)
{
    int left, right, current;
    drct_get_volume (& left, & right);
    current = MAX (left, right);

    if (current > 0)
        drct_set_volume (volume * left / current, volume * right / current);
    else
        drct_set_volume (volume, volume);
}

void drct_get_volume_balance (int * balance)
{
    int left, right;
    drct_get_volume (& left, & right);

    if (left == right)
        * balance = 0;
    else if (left > right)
        * balance = -100 + right * 100 / left;
    else
        * balance = 100 - left * 100 / right;
}

void drct_set_volume_balance (int balance)
{
    int left, right;
    drct_get_volume_main (& left);

    if (balance < 0)
        right = left * (100 + balance) / 100;
    else
    {
        right = left;
        left = right * (100 - balance) / 100;
    }

    drct_set_volume (left, right);
}

/* --- PLAYLIST CONTROL --- */

void drct_pl_next (void)
{
    int playlist = playlist_get_playing ();
    if (playlist < 0)
        playlist = playlist_get_active ();

    playlist_next_song (playlist, get_bool (NULL, "repeat"));
}

void drct_pl_prev (void)
{
    int playlist = playlist_get_playing ();
    if (playlist < 0)
        playlist = playlist_get_active ();

    playlist_prev_song (playlist);
}

static void add_list (Index * filenames, int at, bool_t to_temp, bool_t play)
{
    if (to_temp)
        playlist_set_active (playlist_get_temporary ());

    int playlist = playlist_get_active ();

    /* queue the new entries before deleting the old ones */
    /* this is to avoid triggering the --quit-after-play condition */
    playlist_entry_insert_batch (playlist, at, filenames, NULL, play);

    if (play)
    {
        if (get_bool (NULL, "clear_playlist"))
            playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
        else
            playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
    }
}

void drct_pl_add (const char * filename, int at)
{
    Index * filenames = index_new ();
    index_insert (filenames, -1, str_get (filename));
    add_list (filenames, at, FALSE, FALSE);
}

void drct_pl_add_list (Index * filenames, int at)
{
    add_list (filenames, at, FALSE, FALSE);
}

void drct_pl_open (const char * filename)
{
    Index * filenames = index_new ();
    index_insert (filenames, -1, str_get (filename));
    add_list (filenames, -1, get_bool (NULL, "open_to_temporary"), TRUE);
}

void drct_pl_open_list (Index * filenames)
{
    add_list (filenames, -1, get_bool (NULL, "open_to_temporary"), TRUE);
}

void drct_pl_open_temp (const char * filename)
{
    Index * filenames = index_new ();
    index_insert (filenames, -1, str_get (filename));
    add_list (filenames, -1, TRUE, TRUE);
}

void drct_pl_open_temp_list (Index * filenames)
{
    add_list (filenames, -1, TRUE, TRUE);
}
