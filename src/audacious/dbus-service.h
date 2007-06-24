/*
 * audacious: Cross-platform multimedia player.
 * dbus-service.h: Audacious DBus/MPRIS access API.
 *
 * Copyright (c) 2007 Ben Tucker    
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DBUS_SERVICE_H
#define _DBUS_SERVICE_H

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

typedef struct {
    GObject parent;
} RemoteObject, MprisRoot, MprisPlayer, MprisTrackList;

typedef struct {
    GObjectClass parent_class;
} RemoteObjectClass, MprisRootClass, MprisPlayerClass, MprisTrackListClass;

void init_dbus();
void free_dbus();
DBusGProxy *audacious_get_dbus_proxy();

///////////////////////////
// MPRIS defined methods //
///////////////////////////
// MPRIS /
gboolean mpris_root_identity(MprisRoot *obj, gchar **identity,
                             GError **error);
// MPRIS /Player
gboolean mpris_player_next(MprisPlayer *obj, GError **error);
gboolean mpris_player_prev(MprisPlayer *obj, GError **error);
gboolean mpris_player_pause(MprisPlayer *obj, GError **error);
gboolean mpris_player_stop(MprisPlayer *obj, GError **error);
gboolean mpris_player_play(MprisPlayer *obj, GError **error);
gboolean mpris_player_quit(MprisPlayer *obj, GError **error);
gboolean mpris_player_repeat(MprisPlayer *obj, gboolean rpt, GError **error);
gboolean mpris_player_get_status(MprisPlayer *obj, gint *status,
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
    CAPS_CHANGE_SIG,
    TRACK_CHANGE_SIG,
    STATUS_CHANGE_SIG,
    LAST_SIG
};
gboolean mpris_player_emit_caps_change(MprisPlayer *obj, GError **error);
gboolean mpris_player_emit_track_change(MprisPlayer *obj, GError **error);
gboolean mpris_player_emit_status_change(MprisPlayer *obj, GError **error);

// MPRIS /TrackList
gboolean mpris_tracklist_get_metadata(MprisTrackList *obj, gint pos,
                                      GHashTable *metadata, GError **error);
gboolean mpris_tracklist_get_current_track(MprisTrackList *obj, gint *pos,
                                           GError **error);
gboolean mpris_tracklist_get_length(MprisTrackList *obj, gint *pos,
                                    GError **error);
gboolean mpris_tracklist_add_track(MprisTrackList *obj, gchar *uri,
                                   gboolean play, GError **error);
gboolean mpris_tracklist_del_track(MprisTrackList *obj, gint pos,
                                   GError **error);
gboolean mpris_tracklist_loop(MprisTrackList *obj, gboolean loop,
                              GError **error);
gboolean mpris_tracklist_random(MprisTrackList *obj, gboolean random,
                                GError **error);


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
gboolean audacious_rc_auto_advance(RemoteObject *obj, gboolean *is_advance,
                                   GError **error);
gboolean audacious_rc_toggle_auto_advance(RemoteObject *obj, GError **error);
gboolean audacious_rc_repeat(RemoteObject *obj, gboolean *is_repeat,
                             GError **error);
gboolean audacious_rc_toggle_repeat(RemoteObject *obj, GError **error);
gboolean audacious_rc_shuffle(RemoteObject *obj, gboolean *is_shuffle,
                              GError **error);
gboolean audacious_rc_toggle_shuffle(RemoteObject *obj, GError **error);
#endif // !_DBUS_SERVICE_H
