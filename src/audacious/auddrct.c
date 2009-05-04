/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Giacomo Lozito
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


#include "main.h"
#include "input.h"
#include "playback.h"
#include "auddrct.h"
#include "playlist.h"
#include "ui_fileopener.h"

/* player */

void
drct_quit ( void )
{
    aud_quit();
}

void
drct_eject ( void )
{
    gboolean play_button = TRUE;
    hook_call("filebrowser show", &play_button);
}

void
drct_jtf_show ( void )
{
    gboolean show = TRUE;
    hook_call("ui jump to track show", &show);
}

gboolean
drct_main_win_is_visible ( void )
{
    return cfg.player_visible;
}

void
drct_main_win_toggle ( gboolean show )
{
    hook_call("mainwin show", &show);
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

void
drct_play ( void )
{
    if (playback_get_paused())
        playback_pause();
    else if (playlist_get_length(playlist_get_active()))
        playback_initiate ();
    else
        run_filebrowser(TRUE);
    return;
}

void
drct_pause ( void )
{
    playback_pause();
    return;
}

void
drct_stop ( void )
{
    ip_data.stop = TRUE;
    playback_stop();
    ip_data.stop = FALSE;
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
drct_get_info( gint *rate, gint *freq, gint *nch)
{
    playback_get_sample_params(rate, freq, nch);
}

gint
drct_get_time ( void )
{
    gint time;
    if (playback_get_playing())
        time = playback_get_time();
    else
        time = 0;
    return time;
}

gint
drct_get_length ( void )
{
    if (playback_get_playing())
        return playback_get_length();
    else
        return -1;
}

void
drct_seek ( guint pos )
{
    if (playlist_get_current_length(playlist_get_active()) > 0 &&
        pos < (guint)playlist_get_current_length(playlist_get_active()))
        playback_seek(pos / 1000);
    return;
}

void
drct_get_volume ( gint *vl, gint *vr )
{
    input_get_volume(vl, vr);
    return;
}

void
drct_set_volume ( gint vl, gint vr )
{
    if (vl > 100)
        vl = 100;
    if (vr > 100)
        vr = 100;
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

void
drct_pl_next ( void )
{
    playlist_next(playlist_get_active());
    return;
}

void
drct_pl_prev ( void )
{
    playlist_prev(playlist_get_active());
    return;
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

gchar *
drct_pl_get_title( gint pos )
{
    return playlist_get_songtitle(playlist_get_active(), pos);
}

gint
drct_pl_get_time( gint pos )
{
    return playlist_get_songtime(playlist_get_active(), pos);
}

gint
drct_pl_get_pos( void )
{
    return playlist_get_position_nolock(playlist_get_active());
}

gchar *
drct_pl_get_file( gint pos )
{
    return playlist_get_filename(playlist_get_active(), pos);
}

void
drct_pl_add ( GList * list )
{
    GList *node = list;
    while ( node != NULL )
    {
        playlist_add_url(playlist_get_active(), (gchar*)node->data);
        node = g_list_next(node);
    }
    return;
}

void
drct_pl_clear ( void )
{
    playlist_clear(playlist_get_active());
    return;
}


/* following functions are not tested yet. be careful. --yaz */
void
drct_pl_delete ( gint pos )
{
    playlist_delete_index(playlist_get_active(), pos);
}

void
drct_pl_set_pos( gint pos )
{
    Playlist *playlist = playlist_get_active();
    if (pos < (guint)playlist_get_length(playlist))
        playlist_set_position(playlist, pos);
}

gint
drct_pl_get_length( void )
{
    return playlist_get_length(playlist_get_active());
}

void
drct_pl_ins_url_string( gchar * string, gint pos )
{
    playlist_ins_url(playlist_get_active(), string, pos);
}

void
drct_pl_add_url_string( gchar * string )
{
    playlist_add_url(playlist_get_active(), string);
}

void
drct_pl_enqueue_to_temp( gchar * string )
{
    Playlist *new_pl = playlist_new();

    GDK_THREADS_ENTER();
    playlist_select_playlist(new_pl);
    playlist_add_url(new_pl, string);
    GDK_THREADS_LEAVE();
}


/* playqueue */
gint
drct_pq_get_length( void )
{
    return playlist_queue_get_length(playlist_get_active());
}

void
drct_pq_add( gint pos )
{
    Playlist *playlist = playlist_get_active();
    if (pos < (guint)playlist_get_length(playlist))
        playlist_queue_position(playlist, pos);
}

void
drct_pq_remove( gint pos )
{
    Playlist *playlist = playlist_get_active();
    if (pos < (guint)playlist_get_length(playlist))
        playlist_queue_remove(playlist_get_active(), pos);
}

void
drct_pq_clear( void )
{
    playlist_clear_queue(playlist_get_active());
}

gboolean
drct_pq_is_queued( gint pos )
{
    return playlist_is_position_queued(playlist_get_active(), pos);
}

gint
drct_pq_get_position( gint pos )
{
    return playlist_get_queue_position_number(playlist_get_active(), pos);
}

gint
drct_pq_get_queue_position( gint pos )
{
    return playlist_get_queue_position_number(playlist_get_active(), pos);
}

