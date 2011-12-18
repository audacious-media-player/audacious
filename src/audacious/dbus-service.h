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
#include <libaudcore/core.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

typedef struct {
    GObject parent;
    DBusGProxy *proxy;
} RemoteObject, MprisRoot, MprisPlayer, MprisTrackList;

typedef struct {
    GObjectClass parent_class;
} RemoteObjectClass, MprisRootClass, MprisPlayerClass, MprisTrackListClass;

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
    MPRIS_STATUS_INVALID = -1,
    MPRIS_STATUS_PLAY = 0,
    MPRIS_STATUS_PAUSE = 1,
    MPRIS_STATUS_STOP = 2,
} PlaybackStatus;

extern MprisPlayer * mpris;

// MPRIS /
bool_t mpris_root_identity(MprisRoot *obj, char **identity,
                             GError **error);
bool_t mpris_root_quit(MprisPlayer *obj, GError **error);

// MPRIS /Player
bool_t mpris_player_next(MprisPlayer *obj, GError **error);
bool_t mpris_player_prev(MprisPlayer *obj, GError **error);
bool_t mpris_player_pause(MprisPlayer *obj, GError **error);
bool_t mpris_player_stop(MprisPlayer *obj, GError **error);
bool_t mpris_player_play(MprisPlayer *obj, GError **error);
bool_t mpris_player_repeat(MprisPlayer *obj, bool_t rpt, GError **error);
bool_t mpris_player_get_status(MprisPlayer *obj, GValueArray **status,
                                 GError **error);
bool_t mpris_player_get_metadata(MprisPlayer *obj, GHashTable **metadata,
                                   GError **error);
bool_t mpris_player_get_caps(MprisPlayer *obj, int *capabilities,
                                 GError **error);
bool_t mpris_player_volume_set(MprisPlayer *obj, int vol, GError **error);
bool_t mpris_player_volume_get(MprisPlayer *obj, int *vol,
                                 GError **error);
bool_t mpris_player_position_set(MprisPlayer *obj, int pos, GError **error);
bool_t mpris_player_position_get(MprisPlayer *obj, int *pos,
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

bool_t mpris_emit_track_change(MprisPlayer *obj);
bool_t mpris_emit_status_change(MprisPlayer *obj, PlaybackStatus status);
bool_t mpris_emit_caps_change(MprisPlayer *obj);
bool_t mpris_emit_tracklist_change(MprisTrackList *obj, int playlist);

// MPRIS /TrackList
bool_t mpris_tracklist_get_metadata(MprisTrackList *obj, int pos,
                                      GHashTable **metadata, GError **error);
bool_t mpris_tracklist_get_current_track(MprisTrackList *obj, int *pos,
                                           GError **error);
bool_t mpris_tracklist_get_length(MprisTrackList *obj, int *length,
                                    GError **error);
bool_t mpris_tracklist_add_track(MprisTrackList *obj, char *uri,
                                   bool_t play, GError **error);
bool_t mpris_tracklist_del_track(MprisTrackList *obj, int pos,
                                   GError **error);
bool_t mpris_tracklist_loop(MprisTrackList *obj, bool_t loop,
                              GError **error);
bool_t mpris_tracklist_random(MprisTrackList *obj, bool_t random,
                                GError **error);

/* Legacy API */
// Audacious General Information
bool_t audacious_rc_version(RemoteObject *obj, char **version, GError **error);
bool_t audacious_rc_quit(RemoteObject *obj, GError **error);
bool_t audacious_rc_eject(RemoteObject *obj, GError **error);
bool_t audacious_rc_main_win_visible(RemoteObject *obj,
                                       bool_t *is_main_win, GError **error);
bool_t audacious_rc_show_main_win(RemoteObject *obj, bool_t show,
                                    GError **error);
bool_t audacious_rc_equalizer_visible(RemoteObject *obj, bool_t *is_eq_win,
                                        GError **error);
bool_t audacious_rc_show_equalizer(RemoteObject *obj, bool_t show,
                                     GError **error);
bool_t audacious_rc_playlist_visible(RemoteObject *obj,
                                       bool_t *is_pl_win,
                                       GError **error);
bool_t audacious_rc_show_playlist(RemoteObject *obj, bool_t show,
                                    GError **error);
bool_t audacious_rc_get_tuple_fields(RemoteObject *obj, char ***fields,
                                    GError **error);

// Playback Information/Manipulation
bool_t audacious_rc_play(RemoteObject *obj, GError **error);
bool_t audacious_rc_pause(RemoteObject *obj, GError **error);
bool_t audacious_rc_stop(RemoteObject *obj, GError **error);
bool_t audacious_rc_playing(RemoteObject *obj, bool_t *is_playing,
                              GError **error);
bool_t audacious_rc_paused(RemoteObject *obj, bool_t *is_paused,
                             GError **error);
bool_t audacious_rc_stopped(RemoteObject *obj, bool_t *is_stopped,
                              GError **error);
bool_t audacious_rc_status(RemoteObject *obj, char **status,
                             GError **error);
bool_t audacious_rc_info(RemoteObject *obj, int *rate, int *freq,
                           int *nch, GError **error);
bool_t audacious_rc_time(RemoteObject *obj, int *time, GError **error);
bool_t audacious_rc_seek(RemoteObject *obj, unsigned int pos, GError **error);
bool_t audacious_rc_volume(RemoteObject *obj, int *vl, int *vr,
                             GError **error);
bool_t audacious_rc_set_volume(RemoteObject *obj, int vl, int vr,
                                 GError **error);
bool_t audacious_rc_balance(RemoteObject *obj, int *balance,
                              GError **error);

// Playlist Information/Manipulation
bool_t audacious_rc_position(RemoteObject *obj, int *pos, GError **error);
bool_t audacious_rc_advance(RemoteObject *obj, GError **error);
bool_t audacious_rc_reverse(RemoteObject *obj, GError **error);
bool_t audacious_rc_length(RemoteObject *obj, int *length,
                             GError **error);
bool_t audacious_rc_song_title(RemoteObject *obj, unsigned int pos,
                                 char **title, GError **error);
bool_t audacious_rc_song_filename(RemoteObject *obj, unsigned int pos,
                                    char **filename, GError **error);
bool_t audacious_rc_song_length(RemoteObject *obj, unsigned int pos, int *length,
                                  GError **error);
bool_t audacious_rc_song_frames(RemoteObject *obj, unsigned int pos, int *length,
                                  GError **error);
bool_t audacious_rc_song_tuple(RemoteObject *obj, unsigned int pos, char *tuple,
                                 GValue *value, GError **error);
bool_t audacious_rc_jump(RemoteObject *obj, unsigned int pos, GError **error);
bool_t audacious_rc_add(RemoteObject *obj, char *file, GError **error);
bool_t audacious_rc_add_url(RemoteObject *obj, char *url,
                              GError **error);
bool_t audacious_rc_add_list (RemoteObject * obj, char * * filenames,
 GError * * error);
bool_t audacious_rc_open_list (RemoteObject * obj, char * * filenames,
 GError * * error);
bool_t audacious_rc_open_list_to_temp (RemoteObject * obj, char * *
 filenames, GError * * error);
bool_t audacious_rc_delete(RemoteObject *obj, unsigned int pos, GError **error);
bool_t audacious_rc_clear(RemoteObject *obj, GError **error);
bool_t audacious_rc_auto_advance(RemoteObject *obj, bool_t *is_advance,
                                   GError **error);
bool_t audacious_rc_toggle_auto_advance(RemoteObject *obj, GError **error);
bool_t audacious_rc_repeat(RemoteObject *obj, bool_t *is_repeat,
                             GError **error);
bool_t audacious_rc_toggle_repeat(RemoteObject *obj, GError **error);
bool_t audacious_rc_shuffle(RemoteObject *obj, bool_t *is_shuffle,
                              GError **error);
bool_t audacious_rc_toggle_shuffle(RemoteObject *obj, GError **error);

/* new */
bool_t audacious_rc_show_prefs_box(RemoteObject *obj, bool_t show, GError **error);
bool_t audacious_rc_show_about_box(RemoteObject *obj, bool_t show, GError **error);
bool_t audacious_rc_show_jtf_box(RemoteObject *obj, bool_t show, GError **error);
bool_t audacious_rc_show_filebrowser(RemoteObject *obj, bool_t show, GError **error); //new Nov 8
bool_t audacious_rc_play_pause(RemoteObject *obj, GError **error);
bool_t audacious_rc_activate(RemoteObject *obj, GError **error);
bool_t audacious_rc_queue_get_list_pos(RemoteObject *obj, int qpos, int *pos, GError **error);
bool_t audacious_rc_queue_get_queue_pos(RemoteObject *obj, int pos, int *qpos, GError **error);
bool_t audacious_rc_get_info(RemoteObject *obj, int *rate, int *freq, int *nch, GError **error);
bool_t audacious_rc_toggle_aot(RemoteObject *obj, bool_t ontop, GError **error);
bool_t audacious_rc_get_playqueue_length(RemoteObject *obj, int *length, GError **error);
bool_t audacious_rc_playqueue_add(RemoteObject *obj, int pos, GError **error);
bool_t audacious_rc_playqueue_remove(RemoteObject *obj, int pos, GError **error);
bool_t audacious_rc_playqueue_clear(RemoteObject *obj, GError **error);
bool_t audacious_rc_playqueue_is_queued(RemoteObject *obj, int pos, bool_t *is_queued, GError **error);
bool_t audacious_rc_playlist_ins_url_string(RemoteObject *obj, char *url, int pos, GError **error);
bool_t audacious_rc_playlist_enqueue_to_temp(RemoteObject *obj, char *url, GError **error);
bool_t audacious_rc_playlist_add(RemoteObject *obj, gpointer list, GError **error);

/* new on nov 7 */
bool_t audacious_rc_get_eq(RemoteObject *obj, double *preamp, GArray **bands, GError **error);
bool_t audacious_rc_get_eq_preamp(RemoteObject *obj, double *preamp, GError **error);
bool_t audacious_rc_get_eq_band(RemoteObject *obj, int band, double *value, GError **error);
bool_t audacious_rc_set_eq(RemoteObject *obj, double preamp, GArray *bands, GError **error);
bool_t audacious_rc_set_eq_preamp(RemoteObject *obj, double preamp, GError **error);
bool_t audacious_rc_set_eq_band(RemoteObject *obj, int band, double value, GError **error);
bool_t audacious_rc_equalizer_activate(RemoteObject *obj, bool_t active, GError **error);

/* new in 2.4 */
bool_t audacious_rc_get_active_playlist_name(RemoteObject *obj, char **title, GError **error);

/* new in 3.1 */
bool_t audacious_rc_stop_after (RemoteObject * obj, bool_t * is_stopping, GError * * error);
bool_t audacious_rc_toggle_stop_after (RemoteObject * obj, GError * * error);

#endif /* AUDACIOUS_DBUS_SERVICE_H */
