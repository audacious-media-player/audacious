/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef XMMS_XMMSCTRL_H
#define XMMS_XMMSCTRL_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum
    {
        AUDACIOUS_TYPE_UNIX,
	AUDACIOUS_TYPE_TCP,
    };

    /* Do NOT use this! This is only for control socket initialization now. */
    gint xmms_connect_to_session(gint session);

    void xmms_remote_playlist(gint session, gchar ** list, gint num,
                              gboolean enqueue);
    gint xmms_remote_get_version(gint session);
    void xmms_remote_playlist_add(gint session, GList * list);
    void xmms_remote_playlist_delete(gint session, gint pos);
    void xmms_remote_play(gint session);
    void xmms_remote_pause(gint session);
    void xmms_remote_stop(gint session);
    gboolean xmms_remote_is_playing(gint session);
    gboolean xmms_remote_is_paused(gint session);
    gint xmms_remote_get_playlist_pos(gint session);
    void xmms_remote_set_playlist_pos(gint session, gint pos);
    gint xmms_remote_get_playlist_length(gint session);
    void xmms_remote_playlist_clear(gint session);
    gint xmms_remote_get_output_time(gint session);
    void xmms_remote_jump_to_time(gint session, gint pos);
    void xmms_remote_get_volume(gint session, gint * vl, gint * vr);
    gint xmms_remote_get_main_volume(gint session);
    gint xmms_remote_get_balance(gint session);
    void xmms_remote_set_volume(gint session, gint vl, gint vr);
    void xmms_remote_set_main_volume(gint session, gint v);
    void xmms_remote_set_balance(gint session, gint b);
    gchar *xmms_remote_get_skin(gint session);
    void xmms_remote_set_skin(gint session, gchar * skinfile);
    gchar *xmms_remote_get_playlist_file(gint session, gint pos);
    gchar *xmms_remote_get_playlist_title(gint session, gint pos);
    gint xmms_remote_get_playlist_time(gint session, gint pos);
    void xmms_remote_get_info(gint session, gint * rate, gint * freq,
                              gint * nch);
    void xmms_remote_main_win_toggle(gint session, gboolean show);
    void xmms_remote_pl_win_toggle(gint session, gboolean show);
    void xmms_remote_eq_win_toggle(gint session, gboolean show);
    gboolean xmms_remote_is_main_win(gint session);
    gboolean xmms_remote_is_pl_win(gint session);
    gboolean xmms_remote_is_eq_win(gint session);
    void xmms_remote_show_prefs_box(gint session);
    void xmms_remote_toggle_aot(gint session, gboolean ontop);
    void xmms_remote_eject(gint session);
    void xmms_remote_playlist_prev(gint session);
    void xmms_remote_playlist_next(gint session);
    void xmms_remote_playlist_add_url_string(gint session, gchar * string);
    gboolean xmms_remote_is_running(gint session);
    void xmms_remote_toggle_repeat(gint session);
    void xmms_remote_toggle_shuffle(gint session);
    gboolean xmms_remote_is_repeat(gint session);
    gboolean xmms_remote_is_shuffle(gint session);
    void xmms_remote_get_eq(gint session, gfloat * preamp,
                            gfloat ** bands);
    gfloat xmms_remote_get_eq_preamp(gint session);
    gfloat xmms_remote_get_eq_band(gint session, gint band);
    void xmms_remote_set_eq(gint session, gfloat preamp, gfloat * bands);
    void xmms_remote_set_eq_preamp(gint session, gfloat preamp);
    void xmms_remote_set_eq_band(gint session, gint band, gfloat value);

/* Added in XMMS 1.2.1 */
    void xmms_remote_quit(gint session);

/* Added in XMMS 1.2.6 */
    void xmms_remote_play_pause(gint session);
    void xmms_remote_playlist_ins_url_string(gint session, gchar * string,
                                             gint pos);

/* Added in XMMS 1.2.11 */
    void xmms_remote_playqueue_add(gint session, gint pos);
    void xmms_remote_playqueue_remove(gint session, gint pos);
    gint xmms_remote_get_playqueue_length(gint session);
    void xmms_remote_toggle_advance(gint session);
    gboolean xmms_remote_is_advance(gint session);

/* Added in BMP 0.9.7 */
    void xmms_remote_activate(gint session);

/* Added in Audacious 1.1 */
    void xmms_remote_show_jtf_box(gint session);
    void xmms_remote_playqueue_clear(gint session);
    gboolean xmms_remote_playqueue_is_queued(gint session, gint pos);
    gint xmms_remote_get_playqueue_position(gint session, gint pos);
    gint xmms_remote_get_playqueue_queue_position(gint session, gint pos);

/* Added in Audacious 1.2 */
    void audacious_set_session_uri(gchar *uri);
    gchar *audacious_get_session_uri(gint session);

#ifdef __cplusplus
};
#endif

/* Deprecated APIs */
void xmms_remote_play_files(gint session, GList * list);

#define xmms_remote_add_files(session,list) \
        xmms_remote_playlist_add(session,list)


#endif
