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

#ifndef AUDACIOUS_AUDDRCT_H
#define AUDACIOUS_AUDDRCT_H

#include <glib.h>

/* player */
void drct_quit ( void );
void drct_eject ( void );
void drct_jtf_show ( void );
gboolean drct_main_win_is_visible ( void );
void drct_main_win_toggle ( gboolean );
gboolean drct_eq_win_is_visible ( void );
void drct_eq_win_toggle ( gboolean );
gboolean drct_pl_win_is_visible ( void );
void drct_pl_win_toggle ( gboolean );
void drct_activate(void);

/* playback */
void drct_initiate ( void );
void drct_play ( void );
void drct_pause ( void );
void drct_stop ( void );
gboolean drct_get_playing ( void );
gboolean drct_get_paused ( void );
gboolean drct_get_stopped ( void );
void drct_seek (gint pos);
void drct_get_volume( gint *vl, gint *vr );
void drct_set_volume( gint vl, gint vr );
void drct_get_volume_main( gint *v );
void drct_set_volume_main( gint v );
void drct_get_volume_balance( gint *b );
void drct_set_volume_balance( gint b );

/* playlist */
void drct_pl_next( void );
void drct_pl_prev( void );
gboolean drct_pl_repeat_is_enabled ( void );
void drct_pl_repeat_toggle ( void );
gboolean drct_pl_repeat_is_shuffled ( void );
void drct_pl_shuffle_toggle ( void );
gchar *drct_pl_get_title( gint pos );
gint drct_pl_get_time( gint pos );
gint drct_pl_get_pos( void );
gchar *drct_pl_get_file( gint pos );
void drct_pl_open (const gchar * filename);
void drct_pl_open_list (GList * list);
void drct_pl_add ( GList * list );
void drct_pl_clear ( void );
gint drct_pl_get_length( void );
void drct_pl_delete ( gint pos );
void drct_pl_set_pos( gint pos );
void drct_pl_ins_url_string (const gchar * string, gint pos);
void drct_pl_add_url_string (const gchar * string);
void drct_pl_enqueue_to_temp (const gchar * filename);
void drct_pl_temp_open_list (GList * list);

/* playqueue */
gint drct_pq_get_length( void );
void drct_pq_add( gint pos );
void drct_pq_remove( gint pos );
void drct_pq_clear( void );
gboolean drct_pq_is_queued( gint pos );
gint drct_pq_get_position( gint pos );
gint drct_pq_get_queue_position( gint pos );

/* adjust naming scheme to audacious_remote_* functions */
#define audacious_drct_show_jtf_box audacious_drct_jtf_show
#define audacious_drct_is_eq_win audacious_drct_eq_win_is_visible
#define audacious_drct_is_pl_win audacious_drct_pl_win_is_visible
#define audacious_drct_is_playing audacious_drct_get_playing
#define audacious_drct_is_paused audacious_drct_get_paused
#define audacious_drct_get_output_time audacious_drct_get_time
#define audacious_drct_jump_to_time audacious_drct_seek
#define audacious_drct_get_main_volume audacious_drct_get_volume_main
#define audacious_drct_set_main_volume audacious_drct_set_volume_main
#define audacious_drct_get_balance audacious_drct_get_volume_balance
#define audacious_drct_set_balance audacious_drct_set_volume_balance
#define audacious_drct_playlist_next audacious_drct_pl_next
#define audacious_drct_playlist_prev audacious_drct_pl_prev
#define audacious_drct_is_repeat audacious_drct_pl_repeat_is_enabled

#define audacious_drct_toggle_repeat audacious_drct_pl_repeat_toggle
#define audacious_drct_is_shuffle audacious_drct_pl_repeat_is_shuffled
#define audacious_drct_toggle_shuffle audacious_drct_pl_shuffle_toggle
#define audacious_drct_get_playlist_title audacious_drct_pl_get_title
#define audacious_drct_get_playlist_time audacious_drct_pl_get_time
#define audacious_drct_get_playlist_pos audacious_drct_pl_get_pos
#define audacious_drct_get_playlist_file audacious_drct_pl_get_file
#define audacious_drct_playlist_add audacious_drct_pl_add
#define audacious_drct_playlist_clear audacious_drct_pl_clear
#define audacious_drct_get_playlist_length audacious_drct_pl_get_length
#define audacious_drct_playlist_delete audacious_drct_pl_delete
#define audacious_drct_set_playlist_pos audacious_drct_pl_set_pos
#define audacious_drct_playlist_ins_url_string audacious_drct_pl_ins_url_string
#define audacious_drct_playlist_add_url_string audacious_drct_pl_add_url_string
#define audacious_drct_playlist_enqueue_to_temp audacious_drct_pl_enqueue_to_temp

#define audacious_drct_get_playqueue_length audacious_drct_pq_get_length
#define audacious_drct_playqueue_add audacious_drct_pq_add
#define audacious_drct_playqueue_remove audacious_drct_pq_remove
#define audacious_drct_playqueue_clear audacious_drct_pq_clear
#define audacious_drct_playqueue_is_queued audacious_drct_pq_is_queued
#define audacious_drct_get_playqueue_position audacious_drct_pq_get_position
#define audacious_drct_get_playqueue_queue_position audaciuos_drct_pq_get_queue_position

#endif /* AUDACIOUS_AUDDRCT_H */
