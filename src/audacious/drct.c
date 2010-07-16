/*
 * drct.c
 * Copyright 2009-2010 John Lindgren
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
#include <libaudcore/hook.h>
#include <libaudcore/vfs.h>

#include "audconfig.h"
#include "config.h"
#include "drct.h"
#include "i18n.h"
#include "main.h"
#include "playback.h"
#include "playlist.h"

/* --- PROGRAM CONTROL --- */

void drct_quit (void)
{
    aud_quit ();
}

/* --- PLAYBACK CONTROL --- */

void drct_play (void)
{
    playback_play (0, FALSE);
}

void drct_pause (void)
{
    playback_pause ();
}

void drct_stop (void)
{
    playback_stop ();
}

gboolean drct_get_playing (void)
{
    return playback_get_playing ();
}

gboolean drct_get_paused (void)
{
    return playback_get_paused ();
}

gchar * drct_get_title (void)
{
    return playback_get_title ();
}

void drct_get_info (gint * bitrate, gint * samplerate, gint * channels)
{
    playback_get_info (bitrate, samplerate, channels);
}

gint drct_get_time (void)
{
    return playback_get_time ();
}

gint drct_get_length (void)
{
    return playback_get_length ();
}

void drct_seek (gint time)
{
    playback_seek (time);
}

/* --- VOLUME CONTROL --- */

void drct_get_volume (gint * left, gint * right)
{
    input_get_volume (left, right);
    * left = CLAMP (* left, 0, 100);
    * right = CLAMP (* right, 0, 100);
}

void drct_set_volume (gint left, gint right)
{
    input_set_volume (CLAMP (left, 0, 100), CLAMP (right, 0, 100));
}

void drct_get_volume_main (gint * volume)
{
    gint left, right;
    drct_get_volume (& left, & right);
    * volume = MAX (left, right);
}

void drct_set_volume_main (gint volume)
{
    gint left, right, current;
    drct_get_volume (& left, & right);
    current = MAX (left, right);

    if (current > 0)
        drct_set_volume (volume * left / current, volume * right / current);
    else
        drct_set_volume (volume, volume);
}

void drct_get_volume_balance (gint * balance)
{
    gint left, right;
    drct_get_volume (& left, & right);

    if (left == right)
        * balance = 0;
    else if (left > right)
        * balance = -100 + right * 100 / left;
    else
        * balance = 100 - left * 100 / right;
}

void drct_set_volume_balance (gint balance)
{
    gint left, right;
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

gint drct_pl_get_length (void)
{
    return playlist_entry_count (playlist_get_active ());
}

void drct_pl_next (void)
{
    gboolean play = playback_get_playing ();
    if (playlist_next_song (playlist_get_playing (), cfg.repeat) && play)
        playback_play (0, FALSE);
}

void drct_pl_prev (void)
{
    gboolean play = playback_get_playing ();
    if (playlist_prev_song (playlist_get_playing ()) && play)
        playback_play (0, FALSE);
}

gint drct_pl_get_pos (void)
{
    return playlist_get_position (playlist_get_active ());
}

void drct_pl_set_pos (gint pos)
{
    gint playlist = playlist_get_active ();
    gboolean play = playback_get_playing ();

    playlist_set_position (playlist, pos);

    if (play)
    {
        playlist_set_playing (playlist);
        playback_play (0, FALSE);
    }
}

gboolean drct_pl_repeat_is_enabled (void)
{
    return cfg.repeat;
}

void drct_pl_repeat_toggle (void)
{
    cfg.repeat = ! cfg.repeat;
    hook_call ("toggle repeat", NULL);
}

gboolean drct_pl_shuffle_is_enabled (void)
{
    return cfg.shuffle;
}

void drct_pl_shuffle_toggle (void)
{
    cfg.shuffle = ! cfg.shuffle;
    hook_call ("toggle shuffle", NULL);
}

gchar * drct_pl_get_file (gint entry)
{
    const gchar * filename = playlist_entry_get_filename
     (playlist_get_active (), entry);
    return (filename == NULL) ? NULL : g_strdup (filename);
}

gchar * drct_pl_get_title (gint entry)
{
    const gchar * title = playlist_entry_get_title (playlist_get_active (),
     entry, FALSE);
    return (title == NULL) ? NULL : g_strdup (title);
}

gint drct_pl_get_time (gint pos)
{
    return playlist_entry_get_length (playlist_get_active (), pos, FALSE);
}

static void add_list (GList * list, gint at, gboolean play)
{
    gint playlist = playlist_get_active ();

    if (play)
    {
        if (cfg.clear_playlist)
            playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
        else
            playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
    }

    gint entries = playlist_entry_count (playlist);
    if (at < 0)
        at = entries;

    gint added = 0;
    GQueue folders = G_QUEUE_INIT;
    struct index * filenames = index_new ();

    for (; list != NULL; list = list->next)
    {
        if (filename_is_playlist (list->data))
        {
            playlist_insert_playlist (playlist, at + added, list->data);
            added += playlist_entry_count (playlist) - entries;
            entries = playlist_entry_count (playlist);
        }
        else if (vfs_file_test (list->data, G_FILE_TEST_IS_DIR))
            g_queue_push_tail (& folders, list->data);
        else
            index_append (filenames, g_strdup (list->data));
    }

    playlist_entry_insert_batch (playlist, at + added, filenames, NULL);
    added += playlist_entry_count (playlist) - entries;

    if (added && play)
    {
        playlist_set_playing (playlist);
        if (! cfg.shuffle)
            playlist_set_position (playlist, at);
        playback_play (0, FALSE);
        play = FALSE;
    }

    const gchar * folder;
    while ((folder = g_queue_pop_head (& folders)) != NULL)
    {
        playlist_insert_folder (playlist, at + added, folder, play);
        play = FALSE;
    }
}

void drct_pl_add (const gchar * filename, gint at)
{
    GList * list = g_list_prepend (NULL, (void *) filename);
    add_list (list, at, FALSE);
    g_list_free (list);
}

void drct_pl_add_list (GList * list, gint at)
{
    add_list (list, at, FALSE);
}

void drct_pl_open (const gchar * filename)
{
    GList * list = g_list_prepend (NULL, (void *) filename);
    add_list (list, -1, TRUE);
    g_list_free (list);
}

void drct_pl_open_list (GList * list)
{
    add_list (list, -1, TRUE);
}

static void activate_temp (void)
{
    gint playlists = playlist_count ();
    const gchar * title = _("Temporary Playlist");

    for (gint playlist = 0; playlist < playlists; playlist ++)
    {
        if (! strcmp (playlist_get_title (playlist), title))
        {
            playlist_set_active (playlist);
            return;
        }
    }

    playlist_insert (playlists);
    playlist_set_title (playlists, title);
}

void drct_pl_open_temp (const gchar * filename)
{
    activate_temp ();
    drct_pl_open (filename);
}

void drct_pl_open_temp_list (GList * list)
{
    activate_temp ();
    drct_pl_open_list (list);
}

void drct_pl_delete (gint entry)
{
    playlist_entry_delete (playlist_get_active (), entry, 1);
}

void drct_pl_clear (void)
{
    gint playlist = playlist_get_active ();
    playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
}

/* --- PLAYLIST QUEUE CONTROL --- */

gint drct_pq_get_length (void)
{
    return playlist_queue_count (playlist_get_active ());
}

gint drct_pq_get_entry (gint queue_position)
{
    return playlist_queue_get_entry (playlist_get_active (), queue_position);
}

gboolean drct_pq_is_queued (gint entry)
{
    return (drct_pq_get_queue_position (entry) >= 0);
}

gint drct_pq_get_queue_position (gint entry)
{
    return playlist_queue_find_entry (playlist_get_active (), entry);
}

void drct_pq_add (gint entry)
{
    playlist_queue_insert (playlist_get_active (), -1, entry);
}

void drct_pq_remove (gint entry)
{
    gint playlist = playlist_get_active ();
    playlist_queue_delete (playlist, playlist_queue_find_entry (playlist,
     entry), 1);
}

void drct_pq_clear (void)
{
    gint playlist = playlist_get_active ();
    playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
}
