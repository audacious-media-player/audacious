/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Ben Tucker
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

#ifndef _DBUS_SERVICE_H
#define _DBUS_SERVICE_H

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

typedef struct {
    GObject parent;
    DBusGConnection *connection;
} RemoteObject;

typedef struct {
    GObjectClass parent_class;
} RemoteObjectClass;

void init_dbus();
void free_dbus();
DBusGProxy *audacious_get_dbus_proxy();

// Audacious General Information
gboolean audacious_rc_version(RemoteObject *obj, gchar **version,
                              GError **error);
gboolean audacious_rc_quit(RemoteObject *obj, GError **error);
gboolean audacious_rc_eject(RemoteObject *obj, GError **error);
gboolean audacious_rc_main_win_visible(RemoteObject *obj,
                                       gboolean *is_main_win, GError **error);
gboolean audacious_rc_show_main_win(RemoteObject *obj, gboolean show,
                                    GError **error);
gboolean audacious_rc_equalizer_visible(RemoteObject *obj, gboolean *is_eq_win,
                                        GError **error);
gboolean audacious_rc_show_equalizer(RemoteObject *obj, gboolean show,
                                     GError **error);
gboolean audacious_rc_playlist_visible(RemoteObject *obj,
                                       gboolean *is_pl_win,
                                       GError **error);
gboolean audacious_rc_show_playlist(RemoteObject *obj, gboolean show,
                                    GError **error);

// Playback Information/Manipulation
gboolean audacious_rc_play(RemoteObject *obj, GError **error);
gboolean audacious_rc_pause(RemoteObject *obj, GError **error);
gboolean audacious_rc_stop(RemoteObject *obj, GError **error);
gboolean audacious_rc_playing(RemoteObject *obj, gboolean *is_playing,
                              GError **error);
gboolean audacious_rc_paused(RemoteObject *obj, gboolean *is_paused,
                             GError **error);
gboolean audacious_rc_stopped(RemoteObject *obj, gboolean *is_stopped,
                              GError **error);
gboolean audacious_rc_status(RemoteObject *obj, gchar **status,
                             GError **error);
gboolean audacious_rc_info(RemoteObject *obj, gint *rate, gint *freq,
                           gint *nch, GError **error);
gboolean audacious_rc_time(RemoteObject *obj, gint *time, GError **error);
gboolean audacious_rc_seek(RemoteObject *obj, guint pos, GError **error);
gboolean audacious_rc_volume(RemoteObject *obj, gint *vl, gint *vr,
                             GError **error);
gboolean audacious_rc_set_volume(RemoteObject *obj, gint vl, gint vr,
                                 GError **error);
gboolean audacious_rc_balance(RemoteObject *obj, gint *balance,
                              GError **error);

// Playlist Information/Manipulation
gboolean audacious_rc_position(RemoteObject *obj, int *pos, GError **error);
gboolean audacious_rc_advance(RemoteObject *obj, GError **error);
gboolean audacious_rc_reverse(RemoteObject *obj, GError **error);
gboolean audacious_rc_length(RemoteObject *obj, int *length,
                             GError **error);
gboolean audacious_rc_song_title(RemoteObject *obj, guint pos,
                                 gchar **title, GError **error);
gboolean audacious_rc_song_filename(RemoteObject *obj, guint pos,
                                    gchar **filename, GError **error);
gboolean audacious_rc_song_length(RemoteObject *obj, guint pos, int *length,
                                  GError **error);
gboolean audacious_rc_song_frames(RemoteObject *obj, guint pos, int *length,
                                  GError **error);
gboolean audacious_rc_song_tuple(RemoteObject *obj, guint pos, gchar *tuple,
                                 GValue *value, GError **error);
gboolean audacious_rc_jump(RemoteObject *obj, guint pos, GError **error);
gboolean audacious_rc_add(RemoteObject *obj, gchar *file, GError **error);
gboolean audacious_rc_add_url(RemoteObject *obj, gchar *url,
                              GError **error);
gboolean audacious_rc_delete(RemoteObject *obj, guint pos, GError **error);
gboolean audacious_rc_clear(RemoteObject *obj, GError **error);
gboolean audacious_rc_repeating(RemoteObject *obj, gboolean *is_repeating,
                                GError **error);
gboolean audacious_rc_repeat(RemoteObject *obj, GError **error);
gboolean audacious_rc_shuffling(RemoteObject *obj, gboolean *is_shuffling,
                                GError **error);
gboolean audacious_rc_shuffle(RemoteObject *obj, GError **error);
#endif // !_DBUS_SERVICE_H
