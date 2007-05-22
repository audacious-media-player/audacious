/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Giacomo Lozito
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* audacious_drct_* provides a handy interface for player
   plugins, originally intended for migration from xmms_remote_* calls */

#ifndef AUDDRCT_H
#define AUDDRCT_H

#include <glib.h>

/* player */
void audacious_drct_quit ( void );
void audacious_drct_eject ( void );
void audacious_drct_jtf_show ( void );
gboolean audacious_drct_main_win_is_visible ( void );
void audacious_drct_main_win_toggle ( gboolean );
gboolean audacious_drct_eq_win_is_visible ( void );
void audacious_drct_eq_win_toggle ( gboolean );
gboolean audacious_drct_pl_win_is_visible ( void );
void audacious_drct_pl_win_toggle ( gboolean );
void audacious_drct_set_skin(gchar *skinfile);
void audacious_drct_show_jtf_box(void);
void audacious_drct_activate(void);

/* playback */
void audacious_drct_play ( void );
void audacious_drct_pause ( void );
void audacious_drct_stop ( void );
gboolean audacious_drct_get_playing ( void );
gboolean audacious_drct_get_paused ( void );
gboolean audacious_drct_get_stopped ( void );
void audacious_drct_get_info( gint *rate, gint *freq, gint *nch);
gint audacious_drct_get_time ( void );
void audacious_drct_seek ( guint pos );
void audacious_drct_get_volume( gint *vl, gint *vr );
void audacious_drct_set_volume( gint vl, gint vr );
void audacious_drct_get_volume_main( gint *v );
void audacious_drct_set_volume_main( gint v );
void audacious_drct_get_volume_balance( gint *b );
void audacious_drct_set_volume_balance( gint b );

/* playlist */
void audacious_drct_pl_next( void );
void audacious_drct_pl_prev( void );
gboolean audacious_drct_pl_repeat_is_enabled ( void );
void audacious_drct_pl_repeat_toggle ( void );
gboolean audacious_drct_pl_repeat_is_shuffled ( void );
void audacious_drct_pl_shuffle_toggle ( void );
gchar *audacious_drct_pl_get_title( gint pos );
gint audacious_drct_pl_get_time( gint pos );
gint audacious_drct_pl_get_pos( void );
gchar *audacious_drct_pl_get_file( gint pos );
void audacious_drct_pl_add ( GList * list );
void audacious_drct_pl_clear ( void );
gint audacious_drct_pl_get_length( void );
void audacious_drct_pl_delete ( gint pos );
void audacious_drct_pl_set_pos( gint pos );
void audacious_drct_pl_ins_url_string( gchar * string, gint pos );
void audacious_drct_pl_add_url_string( gchar * string );
void audacious_drct_pl_enqueue_to_temp( gchar * string );

/* playqueue */
gint audacious_drct_pq_get_length( void );
void audacious_drct_pq_add( gint pos );
void audacious_drct_pq_remove( gint pos );
void audacious_drct_pq_clear( void );
gboolean audacious_drct_pq_is_queued( gint pos );
gint audacious_drct_pq_get_position( gint pos );
gint audaciuos_drct_pq_get_queue_position( gint pos );

#endif
