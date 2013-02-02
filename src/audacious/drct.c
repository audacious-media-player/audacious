/*
 * drct.c
 * Copyright 2009-2011 John Lindgren
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
#include <libaudcore/hook.h>
#include <libaudcore/vfs.h>

#include "config.h"
#include "drct.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"

/* --- PROGRAM CONTROL --- */

void drct_quit (void)
{
    hook_call ("quit", NULL);
}

/* --- PLAYBACK CONTROL --- */

void drct_play (void)
{
    int playlist = playlist_get_playing ();
    if (playlist < 0)
        playlist = playlist_get_active ();

    drct_play_playlist (playlist);
}

void drct_play_playlist (int playlist)
{
    bool_t same_playlist = (playlist_get_playing () == playlist);

    if (! same_playlist)
        playlist_set_playing (playlist);

    if (drct_get_playing ())
    {
        if (drct_get_paused ())
            drct_pause ();
        else if (same_playlist)
        {
            int a, b;
            drct_get_ab_repeat (& a, & b);
            drct_seek (MAX (a, 0));
        }
    }
    else
    {
        if (playlist_get_position (playlist) < 0)
            playlist_next_song (playlist, TRUE);

        playback_play (0, FALSE);
    }
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
    bool_t play = drct_get_playing ();

    int playlist = playlist_get_playing ();
    if (playlist < 0)
        playlist = playlist_get_active ();

    if (playlist_next_song (playlist, get_bool (NULL, "repeat")) && play)
    {
        playlist_set_playing (playlist);
        playback_play (0, FALSE);
    }
}

void drct_pl_prev (void)
{
    bool_t play = drct_get_playing ();

    int playlist = playlist_get_playing ();
    if (playlist < 0)
        playlist = playlist_get_active ();

    if (playlist_prev_song (playlist) && play)
    {
        playlist_set_playing (playlist);
        playback_play (0, FALSE);
    }
}

static void add_list (Index * filenames, int at, bool_t to_temp, bool_t play)
{
    if (to_temp)
        playlist_set_active (playlist_get_temporary ());

    int playlist = playlist_get_active ();

    if (play)
    {
        if (get_bool (NULL, "clear_playlist"))
            playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
        else
            playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
    }

    playlist_entry_insert_batch (playlist, at, filenames, NULL, play);
}

void drct_pl_add (const char * filename, int at)
{
    Index * filenames = index_new ();
    index_append (filenames, str_get (filename));
    add_list (filenames, at, FALSE, FALSE);
}

void drct_pl_add_list (Index * filenames, int at)
{
    add_list (filenames, at, FALSE, FALSE);
}

void drct_pl_open (const char * filename)
{
    Index * filenames = index_new ();
    index_append (filenames, str_get (filename));
    add_list (filenames, -1, get_bool (NULL, "open_to_temporary"), TRUE);
}

void drct_pl_open_list (Index * filenames)
{
    add_list (filenames, -1, get_bool (NULL, "open_to_temporary"), TRUE);
}

void drct_pl_open_temp (const char * filename)
{
    Index * filenames = index_new ();
    index_append (filenames, str_get (filename));
    add_list (filenames, -1, TRUE, TRUE);
}

void drct_pl_open_temp_list (Index * filenames)
{
    add_list (filenames, -1, TRUE, TRUE);
}

/* Advancing to the next song when the current one is deleted is tricky.  First,
 * we delete all the selected songs except the current one.  We can then advance
 * to a new song without worrying about picking one that is also selected.
 * Finally, we can delete the former current song without stopping playback. */

void drct_pl_delete_selected (int list)
{
    int pos = playlist_get_position (list);

    if (get_bool (NULL, "advance_on_delete")
     && ! get_bool (NULL, "no_playlist_advance")
     && drct_get_playing () && list == playlist_get_playing ()
     && pos >= 0 && playlist_entry_get_selected (list, pos))
    {
        playlist_entry_set_selected (list, pos, FALSE);
        playlist_delete_selected (list);
        pos = playlist_get_position (list); /* it may have moved */

        if (playlist_next_song (list, get_bool (NULL, "repeat"))
         && playlist_get_position (list) != pos)
            playback_play (0, FALSE);

        playlist_entry_delete (list, pos, 1);
    }
    else
        playlist_delete_selected (list);
}
