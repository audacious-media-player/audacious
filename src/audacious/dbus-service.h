/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Ben Tucker
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

#ifndef AUDACIOUS_DBUS_SERVICE_H
#define AUDACIOUS_DBUS_SERVICE_H

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

typedef struct {
    GObject parent;
    DBusGProxy *proxy;
} RemoteObject, MprisRoot, MprisPlayer, MprisTrackList;

typedef struct {
    GObjectClass parent_class;
} RemoteObjectClass, MprisRootClass, MprisPlayerClass, MprisTrackListClass;

void init_dbus();
void free_dbus();
DBusGProxy *audacious_get_dbus_proxy();

/* MPRIS API */
// Capabilities
enum {
    MPRIS_CAPS_NONE                     = 0,
    MPRIS_CAPS_CAN_GO_NEXT              = 1 << 0,
    MPRIS_CAPS_CAN_GO_PREV              = 1 << 1,
    MPRIS_CAPS_CAN_PAUSE                = 1 << 2,
    MPRIS_CAPS_CAN_PLAY                 = 1 << 3,
    MPRIS_CAPS_CAN_SEEK                 = 1 << 4,
    MPRIS_CAPS_CAN_PROVIDE_METADATA     = 1 << 5,
    MPRIS_CAPS_PROVIDES_TIMING          = 1 << 6,
};

// Status
typedef enum {
	MPRIS_STATUS_PLAY = 0,
	MPRIS_STATUS_PAUSE,
	MPRIS_STATUS_STOP
} PlaybackStatus;

// MPRIS /
gboolean mpris_root_identity(MprisRoot *obj, gchar **identity,
                             GError **error);
gboolean mpris_root_quit(MprisPlayer *obj, GError **error);

// MPRIS /Player
gboolean mpris_player_next(MprisPlayer *obj, GError **error);
gboolean mpris_player_prev(MprisPlayer *obj, GError **error);
gboolean mpris_player_pause(MprisPlayer *obj, GError **error);
gboolean mpris_player_stop(MprisPlayer *obj, GError **error);
gboolean mpris_player_play(MprisPlayer *obj, GError **error);
gboolean mpris_player_repeat(MprisPlayer *obj, gboolean rpt, GError **error);
gboolean mpris_player_get_status(MprisPlayer *obj, GValueArray **status,
                                 GError **error);
gboolean mpris_player_get_metadata(MprisPlayer *obj, GHashTable **metadata,
                                   GError **error);
gboolean mpris_player_get_caps(MprisPlayer *obj, gint *capabilities,
                                 GError **error);
gboolean mpris_player_volume_set(MprisPlayer *obj, gint vol, GError **error);
gboolean mpris_player_volume_get(MprisPlayer *obj, gint *vol,
                                 GError **error);
gboolean mpris_player_position_set(MprisPlayer *obj, gint pos, GError **error);
gboolean mpris_player_position_get(MprisPlayer *obj, gint *pos,
                                   GError **error);
enum {
    TRACK_CHANGE_SIG,
    STATUS_CHANGE_SIG,
    CAPS_CHANGE_SIG,
    LAST_SIG
};

enum {
    TRACKLIST_CHANGE_SIG,
    LAST_TRACKLIST_SIG
};

gboolean mpris_emit_track_change(MprisPlayer *obj);
gboolean mpris_emit_status_change(MprisPlayer *obj, PlaybackStatus status);
gboolean mpris_emit_caps_change(MprisPlayer *obj);
gboolean mpris_emit_tracklist_change(MprisTrackList *obj, gint playlist);

// MPRIS /TrackList
gboolean mpris_tracklist_get_metadata(MprisTrackList *obj, gint pos,
                                      GHashTable **metadata, GError **error);
gboolean mpris_tracklist_get_current_track(MprisTrackList *obj, gint *pos,
                                           GError **error);
gboolean mpris_tracklist_get_length(MprisTrackList *obj, gint *length,
                                    GError **error);
gboolean mpris_tracklist_add_track(MprisTrackList *obj, gchar *uri,
                                   gboolean play, GError **error);
gboolean mpris_tracklist_del_track(MprisTrackList *obj, gint pos,
                                   GError **error);
gboolean mpris_tracklist_loop(MprisTrackList *obj, gboolean loop,
                              GError **error);
gboolean mpris_tracklist_random(MprisTrackList *obj, gboolean random,
                                GError **error);

/* Legacy API */
// Audacious General Information
gboolean audacious_rc_version(RemoteObject *obj, gchar **version, GError **error);
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
gboolean audacious_rc_get_tuple_fields(RemoteObject *obj, gchar ***fields,
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
gboolean audacious_rc_add_list (RemoteObject * obj, gchar * * filenames,
 GError * * error);
gboolean audacious_rc_open_list (RemoteObject * obj, gchar * * filenames,
 GError * * error);
gboolean audacious_rc_open_list_to_temp (RemoteObject * obj, gchar * *
 filenames, GError * * error);
gboolean audacious_rc_delete(RemoteObject *obj, guint pos, GError **error);
gboolean audacious_rc_clear(RemoteObject *obj, GError **error);
gboolean audacious_rc_auto_advance(RemoteObject *obj, gboolean *is_advance,
                                   GError **error);
gboolean audacious_rc_toggle_auto_advance(RemoteObject *obj, GError **error);
gboolean audacious_rc_repeat(RemoteObject *obj, gboolean *is_repeat,
                             GError **error);
gboolean audacious_rc_toggle_repeat(RemoteObject *obj, GError **error);
gboolean audacious_rc_shuffle(RemoteObject *obj, gboolean *is_shuffle,
                              GError **error);
gboolean audacious_rc_toggle_shuffle(RemoteObject *obj, GError **error);

/* new */
gboolean audacious_rc_show_prefs_box(RemoteObject *obj, gboolean show, GError **error);
gboolean audacious_rc_show_about_box(RemoteObject *obj, gboolean show, GError **error);
gboolean audacious_rc_show_jtf_box(RemoteObject *obj, gboolean show, GError **error);
gboolean audacious_rc_show_filebrowser(RemoteObject *obj, gboolean show, GError **error); //new Nov 8
gboolean audacious_rc_play_pause(RemoteObject *obj, GError **error);
gboolean audacious_rc_activate(RemoteObject *obj, GError **error);
gboolean audacious_rc_queue_get_list_pos(RemoteObject *obj, gint qpos, gint *pos, GError **error);
gboolean audacious_rc_queue_get_queue_pos(RemoteObject *obj, gint pos, gint *qpos, GError **error);
gboolean audacious_rc_get_info(RemoteObject *obj, gint *rate, gint *freq, gint *nch, GError **error);
gboolean audacious_rc_toggle_aot(RemoteObject *obj, gboolean ontop, GError **error);
gboolean audacious_rc_get_playqueue_length(RemoteObject *obj, gint *length, GError **error);
gboolean audacious_rc_playqueue_add(RemoteObject *obj, gint pos, GError **error);
gboolean audacious_rc_playqueue_remove(RemoteObject *obj, gint pos, GError **error);
gboolean audacious_rc_playqueue_clear(RemoteObject *obj, GError **error);
gboolean audacious_rc_playqueue_is_queued(RemoteObject *obj, gint pos, gboolean *is_queued, GError **error);
gboolean audacious_rc_playlist_ins_url_string(RemoteObject *obj, gchar *url, gint pos, GError **error);
gboolean audacious_rc_playlist_enqueue_to_temp(RemoteObject *obj, gchar *url, GError **error);
gboolean audacious_rc_playlist_add(RemoteObject *obj, gpointer list, GError **error);

/* new on nov 7 */
gboolean audacious_rc_get_eq(RemoteObject *obj, gdouble *preamp, GArray **bands, GError **error);
gboolean audacious_rc_get_eq_preamp(RemoteObject *obj, gdouble *preamp, GError **error);
gboolean audacious_rc_get_eq_band(RemoteObject *obj, gint band, gdouble *value, GError **error);
gboolean audacious_rc_set_eq(RemoteObject *obj, gdouble preamp, GArray *bands, GError **error);
gboolean audacious_rc_set_eq_preamp(RemoteObject *obj, gdouble preamp, GError **error);
gboolean audacious_rc_set_eq_band(RemoteObject *obj, gint band, gdouble value, GError **error);
gboolean audacious_rc_equalizer_activate(RemoteObject *obj, gboolean active, GError **error);

/* new in 2.4 */
gboolean audacious_rc_get_active_playlist_name(RemoteObject *obj, gchar **title, GError **error);

#endif /* AUDACIOUS_DBUS_SERVICE_H */
