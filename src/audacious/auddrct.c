/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Giacomo Lozito
 * Copyright 2009 John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

/* audacious_drct_* provides a handy interface for player
   plugins, originally intended for migration from xmms_remote_* calls */


#include "i18n.h"
#include "input.h"
#include "playback.h"
#include "auddrct.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "interface.h"

/* player */

void
drct_quit ( void )
{
    aud_quit();
}

void drct_eject (void)
{
    hook_call ("filebrowser show", GINT_TO_POINTER (TRUE));
}

void
drct_jtf_show ( void )
{
    interface_show_jump_to_track();
}

gboolean
drct_main_win_is_visible ( void )
{
    return cfg.player_visible;
}

void
drct_main_win_toggle ( gboolean show )
{
    hook_call("mainwin show", GINT_TO_POINTER(show));
}

gboolean
drct_eq_win_is_visible ( void )
{
    return cfg.equalizer_visible;
}

void
drct_eq_win_toggle ( gboolean show )
{
    hook_call("equalizerwin show", &show);
}

gboolean
drct_pl_win_is_visible ( void )
{
    return cfg.playlist_visible;
}

void
drct_pl_win_toggle ( gboolean show )
{
    hook_call("playlistwin show", &show);
}

void drct_activate(void)
{
    g_message("implement me");
}

/* playback */

void
drct_initiate ( void )
{
    playback_initiate();
}

void drct_play (void)
{
    if (playback_get_playing () && playback_get_paused ())
        playback_pause ();
    else
    {
        playlist_set_playing (playlist_get_active ());
        playback_initiate ();
    }

    return;
}

void
drct_pause ( void )
{
    playback_pause();
    return;
}

gboolean
drct_get_playing ( void )
{
    return playback_get_playing();
}

gboolean
drct_get_paused ( void )
{
    return playback_get_paused();
}

gboolean
drct_get_stopped ( void )
{
    return !playback_get_playing();
}

void
drct_get_volume ( gint *vl, gint *vr )
{
    input_get_volume(vl, vr);
    *vl = CLAMP(*vl, 0, 100);
    *vr = CLAMP(*vr, 0, 100);
    return;
}

void
drct_set_volume ( gint vl, gint vr )
{
    vl = CLAMP(vl, 0, 100);
    vr = CLAMP(vr, 0, 100);
    input_set_volume(vl, vr);
    return;
}

void
drct_get_volume_main( gint *v )
{
    gint vl, vr;
    drct_get_volume(&vl, &vr);
    *v = (vl > vr) ? vl : vr;
    return;
}

void
drct_set_volume_main ( gint v )
{
    gint b, vl, vr;
    drct_get_volume_balance(&b);
    if (b < 0) {
        vl = v;
        vr = (v * (100 - abs(b))) / 100;
    }
    else if (b > 0) {
        vl = (v * (100 - b)) / 100;
        vr = v;
    }
    else
        vl = vr = v;
    drct_set_volume(vl, vr);
}

void
drct_get_volume_balance ( gint *b )
{
    gint vl, vr;
    input_get_volume(&vl, &vr);
    if (vl < 0 || vr < 0)
        *b = 0;
    else if (vl > vr)
        *b = -100 + ((vr * 100) / vl);
    else if (vr > vl)
        *b = 100 - ((vl * 100) / vr);
    else
        *b = 0;
    return;
}

void
drct_set_volume_balance ( gint b )
{
    gint v, vl, vr;
    if (b < -100)
        b = -100;
    if (b > 100)
        b = 100;
    drct_get_volume_main(&v);
    if (b < 0) {
        vl = v;
        vr = (v * (100 - abs(b))) / 100;
    }
    else if (b > 0) {
        vl = (v * (100 - b)) / 100;
        vr = v;
    }
    else
    {
        vl = v;
        vr = v;
    }
    drct_set_volume(vl, vr);
    return;
}


/* playlist */

void drct_pl_next (void)
{
    gboolean play = playback_get_playing ();

    if (playlist_next_song (playlist_get_playing (), cfg.repeat) && play)
        playback_initiate ();
}

void drct_pl_prev (void)
{
    gboolean play = playback_get_playing ();

    if (playlist_prev_song (playlist_get_playing ()) && play)
        playback_initiate ();
}

gboolean
drct_pl_repeat_is_enabled( void )
{
    return cfg.repeat;
}

void
drct_pl_repeat_toggle( void )
{
    g_message("implement me");
    return;
}

gboolean
drct_pl_repeat_is_shuffled( void )
{
    return cfg.shuffle;
}

void
drct_pl_shuffle_toggle( void )
{
    g_message("implement me");
    return;
}

gchar * drct_pl_get_title (gint pos)
{
    const gchar * title = playlist_entry_get_title (playlist_get_active (), pos);

    return (title == NULL) ? NULL : g_strdup (title);
}

gint drct_pl_get_time (gint pos)
{
    return playlist_entry_get_length (playlist_get_active (), pos);
}

gint drct_pl_get_pos (void)
{
    return playlist_get_position (playlist_get_active ());
}

gchar * drct_pl_get_file (gint pos)
{
    const gchar * filename = playlist_entry_get_filename
     (playlist_get_active (), pos);

    return (filename == NULL) ? NULL : g_strdup (filename);
}

void drct_pl_open (const gchar * filename)
{
    GList * list = g_list_prepend (NULL, (gchar *) filename);

    drct_pl_open_list (list);
    g_list_free (list);
}

static void add_list (GList * list, gboolean opening)
{
    gint playlist = playlist_get_active ();
    gint entries;
    struct index * filenames = index_new ();

    if (opening)
    {
        /* insert_folder will not play if already playing */
        if (playback_get_playing ())
            playback_stop ();

        if (cfg.clear_playlist)
            playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
        else
            playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
    }

    entries = playlist_entry_count (playlist);

    for (; list != NULL; list = list->next)
    {
        if (filename_is_playlist (list->data))
            playlist_insert_playlist (playlist, -1, list->data);
        else if (vfs_file_test (list->data, G_FILE_TEST_IS_DIR))
            playlist_insert_folder_v2 (playlist, -1, list->data, opening);
        else
            index_append (filenames, g_strdup (list->data));
    }

    playlist_entry_insert_batch (playlist, -1, filenames, NULL);

    if (opening && playlist_entry_count (playlist) > entries)
    {
        if (entries > 0)
            playlist_set_position (playlist, entries);

        playlist_set_playing (playlist);
        playback_initiate ();
    }
}

void drct_pl_open_list (GList * list)
{
    add_list (list, TRUE);
}

void drct_pl_add (GList * list)
{
    add_list (list, FALSE);
}

void drct_pl_clear (void)
{
    gint playlist = playlist_get_active ();

    playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
}

void drct_pl_delete (gint pos)
{
    playlist_entry_delete (playlist_get_active (), pos, 1);
}

void drct_pl_set_pos (gint pos)
{
    gboolean play = playback_get_playing ();
    gint playlist = playlist_get_active ();

    playlist_set_position (playlist, pos);

    if (play)
    {
        playlist_set_playing (playlist);
        playback_initiate ();
    }
}

gint drct_pl_get_length (void)
{
    return playlist_entry_count (playlist_get_active ());
}

void drct_pl_ins_url_string (const gchar * string, gint pos)
{
    if (filename_is_playlist (string))
        playlist_insert_playlist (playlist_get_active (), pos, string);
    else if (vfs_file_test (string, G_FILE_TEST_IS_DIR))
        playlist_insert_folder (playlist_get_active (), pos, string);
    else
        playlist_entry_insert (playlist_get_active (), pos, g_strdup (string),
         NULL);
}

void drct_pl_add_url_string (const gchar * string)
{
    drct_pl_ins_url_string (string, -1);
}

void drct_pl_enqueue_to_temp (const gchar * filename)
{
    GList * list = g_list_prepend (NULL, (gchar *) filename);

    drct_pl_temp_open_list (list);
    g_list_free (list);
}

void drct_pl_temp_open_list (GList * list)
{
    gint playlist, playlists = playlist_count ();
    const gchar * title = _("Temporary Playlist");

    for (playlist = 0; playlist < playlists; playlist ++)
    {
        if (! strcmp (playlist_get_title (playlist), title))
            goto FOUND;
    }

    playlist_insert (playlist);
    playlist_set_title (playlist, title);

FOUND:
    playlist_set_active (playlist);
    drct_pl_open_list (list);
}

/* playqueue */

gint drct_pq_get_length (void)
{
    return playlist_queue_count (playlist_get_active ());
}

void drct_pq_add (gint pos)
{
    playlist_queue_insert (playlist_get_active (), -1, pos);
}

void drct_pq_remove (gint pos)
{
    gint playlist = playlist_get_active ();

    playlist_queue_delete (playlist, playlist_queue_find_entry (playlist, pos),
     1);
}

void drct_pq_clear (void)
{
    gint playlist = playlist_get_active ();

    playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
}

gboolean drct_pq_is_queued (gint pos)
{
    return (playlist_queue_find_entry (playlist_get_active (), pos) != -1);
}

gint drct_pq_get_position (gint pos)
{
    return playlist_queue_get_entry (playlist_get_active (), pos);
}

gint drct_pq_get_queue_position (gint pos)
{
    return playlist_queue_find_entry (playlist_get_active (), pos);
}
