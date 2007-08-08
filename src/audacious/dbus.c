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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "dbus.h"
#include "dbus-service.h"
#include "dbus-server-bindings.h"

#include "main.h"
#include "ui_equalizer.h"
#include "ui_main.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "memorypool.h"
#include "titlestring.h"
#include "ui_jumptotrack.h"
#include "strings.h"

static DBusGConnection *dbus_conn = NULL;
static guint signals[LAST_SIG] = { 0 };

G_DEFINE_TYPE(RemoteObject, audacious_rc, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisRoot, mpris_root, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisPlayer, mpris_player, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisTrackList, mpris_tracklist, G_TYPE_OBJECT);

void audacious_rc_class_init(RemoteObjectClass *klass) {}
void mpris_root_class_init(MprisRootClass *klass) {}

void mpris_player_class_init(MprisPlayerClass *klass) {
    signals[CAPS_CHANGE_SIG] =
        g_signal_new("caps_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__STRING,
                G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[TRACK_CHANGE_SIG] =
        g_signal_new("track_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__STRING,
                G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[STATUS_CHANGE_SIG] =
        g_signal_new("status_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__STRING,
                G_TYPE_NONE, 1, G_TYPE_STRING);
}

void mpris_tracklist_class_init(MprisTrackListClass *klass) {}

void audacious_rc_init(RemoteObject *object) {
    GError *error = NULL;
    DBusGProxy *driver_proxy;
    unsigned int request_ret;

    
    dbus_g_object_type_install_info(audacious_rc_get_type(),
                                    &dbus_glib_audacious_rc_object_info);
    
    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn,
                                        AUDACIOUS_DBUS_PATH, G_OBJECT(object));

    // Register the service name, the constants here are defined in
    // dbus-glib-bindings.h
    driver_proxy = dbus_g_proxy_new_for_name(dbus_conn,
                                             DBUS_SERVICE_DBUS, DBUS_PATH_DBUS,
                                             DBUS_INTERFACE_DBUS);

    if (!org_freedesktop_DBus_request_name(driver_proxy,
        AUDACIOUS_DBUS_SERVICE, 0, &request_ret, &error)) {
        g_warning("Unable to register service: %s", error->message);
        g_error_free(error);
    }

    if (!org_freedesktop_DBus_request_name(driver_proxy,
        AUDACIOUS_DBUS_SERVICE_MPRIS, 0, &request_ret, &error)) {
        g_warning("Unable to register service: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(driver_proxy);
}

void mpris_root_init(MprisRoot *object) {
    dbus_g_object_type_install_info(mpris_root_get_type(),
                                    &dbus_glib_mpris_root_object_info);
    
    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn,
                                        AUDACIOUS_DBUS_PATH_MPRIS_ROOT,
                                        G_OBJECT(object));
}

void mpris_player_init(MprisPlayer *object) {
    dbus_g_object_type_install_info(mpris_player_get_type(),
                                    &dbus_glib_mpris_player_object_info);
    
    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn,
                                        AUDACIOUS_DBUS_PATH_MPRIS_PLAYER,
                                        G_OBJECT(object));
}

void mpris_tracklist_init(MprisTrackList *object) {
    dbus_g_object_type_install_info(mpris_tracklist_get_type(),
                                    &dbus_glib_mpris_tracklist_object_info);
    
    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn,
                                        AUDACIOUS_DBUS_PATH_MPRIS_TRACKLIST,
                                        G_OBJECT(object));
}

void init_dbus() {
    GError *error = NULL;
    // Initialize the DBus connection
    dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_conn == NULL) {
        g_warning("Unable to connect to dbus: %s", error->message);
        g_error_free(error);
        return;
    }

    g_type_init();
    g_object_new(audacious_rc_get_type(), NULL);
    g_object_new(mpris_root_get_type(), NULL);
    g_object_new(mpris_player_get_type(), NULL);
    g_object_new(mpris_tracklist_get_type(), NULL);
    g_message("D-Bus support has been activated");
}

///////////////////////////
// MPRIS defined methods //
///////////////////////////
// MPRIS /
gboolean mpris_root_identity(MprisRoot *obj, gchar **identity,
                             GError **error) {
    *identity = g_strdup_printf("Audacious %s", VERSION);
    return TRUE;
}

// MPRIS /Player
gboolean mpris_player_next(MprisPlayer *obj, GError **error) {
	playlist_next(playlist_get_active());
    return TRUE;
}
gboolean mpris_player_prev(MprisPlayer *obj, GError **error) {
	playlist_prev(playlist_get_active());
    return TRUE;
}
gboolean mpris_player_pause(MprisPlayer *obj, GError **error) {
	playback_pause();
    return TRUE;
}
gboolean mpris_player_stop(MprisPlayer *obj, GError **error) {
	ip_data.stop = TRUE;
    playback_stop();
    ip_data.stop = FALSE;
    mainwin_clear_song_info();
    return TRUE;
}
gboolean mpris_player_play(MprisPlayer *obj, GError **error) {
	if (playback_get_paused())
        playback_pause();
    else if (playlist_get_length(playlist_get_active()))
        playback_initiate();
    else
        mainwin_eject_pushed();
    return TRUE;
}
gboolean mpris_player_repeat(MprisPlayer *obj, gboolean rpt, GError **error) {
    mainwin_repeat_pushed(rpt);
    mainwin_set_noplaylistadvance(rpt);
    return TRUE;
}
gboolean mpris_player_quit(MprisPlayer *obj, GError **error) {
	// TODO: emit disconnected signal
	mainwin_quit_cb();
    return TRUE;
}
gboolean mpris_player_disconnect(MprisPlayer *obj, GError **error) {
	return FALSE;
}
gboolean mpris_player_get_status(MprisPlayer *obj, gint *status,
                                 GError **error) {
    return FALSE;
}
gboolean mpris_player_get_metadata(MprisTrackList *obj, gint pos,
                                   GHashTable *metadata, GError **error) {
	return FALSE;
}
gboolean mpris_player_get_caps(MprisPlayer *obj, gint *capabilities,
                                 GError **error) {
    return FALSE;
}
gboolean mpris_player_volume_set(MprisPlayer *obj, gint vol, GError **error) {
    return FALSE;
}
gboolean mpris_player_volume_get(MprisPlayer *obj, gint *vol,
                                 GError **error) {
    return FALSE;
}
gboolean mpris_player_position_set(MprisPlayer *obj, gint pos,
                                   GError **error) {
    return FALSE;
}
gboolean mpris_player_position_get(MprisPlayer *obj, gint *pos,
                                   GError **error) {
    return FALSE;
}
// MPRIS /Player signals
gboolean mpris_player_emit_caps_change(MprisPlayer *obj, GError **error) {
    g_signal_emit(obj, signals[CAPS_CHANGE_SIG], 0, "capabilities changed");
    return TRUE;
}

gboolean mpris_player_emit_track_change(MprisPlayer *obj, GError **error) {
    g_signal_emit(obj, signals[TRACK_CHANGE_SIG], 0, "track changed");
    return TRUE;
}

gboolean mpris_player_emit_status_change(MprisPlayer *obj, GError **error) {
    g_signal_emit(obj, signals[STATUS_CHANGE_SIG], 0, "status changed");
    return TRUE;
}

gboolean mpris_player_emit_disconnected(MprisPlayer *obj, GError **error) {
	g_signal_emit(obj, signals[DISCONNECTED], 0, NULL);
	return TRUE;
}

// MPRIS /TrackList
gboolean mpris_tracklist_get_metadata(MprisTrackList *obj, gint pos,
                                      GHashTable *metadata, GError **error) {
    return FALSE;
}
gboolean mpris_tracklist_get_current_track(MprisTrackList *obj, gint *pos,
                                           GError **error) {
	*pos = playlist_get_position(playlist_get_active());
    return TRUE;
}
gboolean mpris_tracklist_get_length(MprisTrackList *obj, gint *length,
                                    GError **error) {
	*length = playlist_get_length(playlist_get_active());
    return TRUE;
}
gboolean mpris_tracklist_add_track(MprisTrackList *obj, gchar *uri,
                                   gboolean play, GError **error) {
    playlist_add_url(playlist_get_active(), uri);
    if (play) {
        int pos = playlist_get_length(playlist_get_active()) - 1;
        playlist_set_position(playlist_get_active(), pos);
        playback_initiate();
    }
    return TRUE;
}
gboolean mpris_tracklist_del_track(MprisTrackList *obj, gint pos,
                                   GError **error) {
	playlist_delete_index(playlist_get_active(), pos);
    return TRUE;
}
gboolean mpris_tracklist_loop(MprisTrackList *obj, gboolean loop,
                              GError **error) {
    return FALSE;
}
gboolean mpris_tracklist_random(MprisTrackList *obj, gboolean random,
                                GError **error) {
	mainwin_shuffle_pushed(!cfg.shuffle);
    return TRUE;
}

// Audacious General Information
gboolean audacious_rc_version(RemoteObject *obj, gchar **version,
                              GError **error) {
    *version = g_strdup(VERSION);
    return TRUE;
}

gboolean audacious_rc_quit(RemoteObject *obj, GError **error) {
    mainwin_quit_cb();
    return TRUE;
}

gboolean audacious_rc_eject(RemoteObject *obj, GError **error) {
    if (has_x11_connection)
        mainwin_eject_pushed();
    return TRUE;
}

gboolean audacious_rc_main_win_visible(RemoteObject *obj,
                                       gboolean *is_main_win, GError **error) {
    *is_main_win = cfg.player_visible;
    g_message("main win %s\n", (cfg.player_visible? "visible" : "hidden"));
    return TRUE;
}

gboolean audacious_rc_show_main_win(RemoteObject *obj, gboolean show,
                                    GError **error) {
    g_message("%s main win\n", (show? "showing": "hiding"));
    if (has_x11_connection)
        mainwin_show(show);
    return TRUE;
}

gboolean audacious_rc_equalizer_visible(RemoteObject *obj,
                                        gboolean *is_eq_win, GError **error) {
    *is_eq_win = cfg.equalizer_visible;
    return TRUE;
}

gboolean audacious_rc_show_equalizer(RemoteObject *obj, gboolean show,
                                     GError **error) {
    if (has_x11_connection)
        equalizerwin_show(show);
    return TRUE;
}

gboolean audacious_rc_playlist_visible(RemoteObject *obj, gboolean *is_pl_win,
                                       GError **error) {
    *is_pl_win = cfg.playlist_visible;
    return TRUE;
}

gboolean audacious_rc_show_playlist(RemoteObject *obj, gboolean show,
                                    GError **error) {
    if (has_x11_connection) {
        if (show)
            playlistwin_show();
        else
            playlistwin_hide();
    }
    return TRUE;
}

// Playback Information/Manipulation
gboolean audacious_rc_play(RemoteObject *obj, GError **error) {
    if (playback_get_paused())
        playback_pause();
    else if (playlist_get_length(playlist_get_active()))
        playback_initiate();
    else
        mainwin_eject_pushed();
    return TRUE;
}

gboolean audacious_rc_pause(RemoteObject *obj, GError **error) {
    playback_pause();
    return TRUE;
}

gboolean audacious_rc_stop(RemoteObject *obj, GError **error) {
    ip_data.stop = TRUE;
    playback_stop();
    ip_data.stop = FALSE;
    mainwin_clear_song_info();
    return TRUE;
}

gboolean audacious_rc_playing(RemoteObject *obj, gboolean *is_playing,
                              GError **error) {
    *is_playing = playback_get_playing();
    return TRUE;
}

gboolean audacious_rc_paused(RemoteObject *obj, gboolean *is_paused,
                             GError **error) {
    *is_paused = playback_get_paused();
    return TRUE;
}

gboolean audacious_rc_stopped(RemoteObject *obj, gboolean *is_stopped,
                              GError **error) {
    *is_stopped = !playback_get_playing();
    return TRUE;
}

gboolean audacious_rc_status(RemoteObject *obj, gchar **status,
                             GError **error) {
    if (playback_get_paused())
        *status = g_strdup("paused");
    else if (playback_get_playing())
        *status = g_strdup("playing");
    else
        *status = g_strdup("stopped");
    return TRUE;
}

gboolean audacious_rc_info(RemoteObject *obj, gint *rate, gint *freq,
                           gint *nch, GError **error) {
    playback_get_sample_params(rate, freq, nch);
    return TRUE;
}

gboolean audacious_rc_time(RemoteObject *obj, gint *time, GError **error) {
    if (playback_get_playing())
        *time = playback_get_time();
    else
        *time = 0;
    return TRUE;
}

gboolean audacious_rc_seek(RemoteObject *obj, guint pos, GError **error) {
    if (playlist_get_current_length(playlist_get_active()) > 0 &&
            pos < (guint)playlist_get_current_length(playlist_get_active()))
            playback_seek(pos / 1000);

    return TRUE;
}

gboolean audacious_rc_volume(RemoteObject *obj, gint *vl, gint *vr,
                             GError **error) {
    input_get_volume(vl, vr);
    return TRUE;
}

gboolean audacious_rc_set_volume(RemoteObject *obj, gint vl, gint vr,
                                 GError **error) {
    if (vl > 100)
        vl = 100;
    if (vr > 100)
        vr = 100;
    input_set_volume(vl, vr);
    return TRUE;
}

gboolean audacious_rc_balance(RemoteObject *obj, gint *balance,
                              GError **error) {
    gint vl, vr;
    input_get_volume(&vl, &vr);
    if (vl < 0 || vr < 0)
        *balance = 0;
    else if (vl > vr)
        *balance = -100 + ((vr * 100) / vl);
    else if (vr > vl)
        *balance = 100 - ((vl * 100) / vr);
    else
        *balance = 0;
    return TRUE;
}

// Playlist Information/Manipulation
gboolean audacious_rc_position(RemoteObject *obj, int *pos, GError **error) {
    *pos = playlist_get_position(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_advance(RemoteObject *obj, GError **error) {
    playlist_next(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_reverse(RemoteObject *obj, GError **error) {
    playlist_prev(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_length(RemoteObject *obj, int *length,
                             GError **error) {
    *length = playlist_get_length(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_song_title(RemoteObject *obj, guint pos,
                                 gchar **title, GError **error) {
    *title = playlist_get_songtitle(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_song_filename(RemoteObject *obj, guint pos,
                                    gchar **filename, GError **error) {
    gchar *tmp = NULL;
    tmp = playlist_get_filename(playlist_get_active(), pos);

    if(tmp){
        *filename = str_to_utf8(tmp);
    }
    free(tmp);
    tmp = NULL;

    return TRUE;
}

gboolean audacious_rc_song_length(RemoteObject *obj, guint pos, int *length,
                                  GError **error) {
    *length = playlist_get_songtime(playlist_get_active(), pos) / 1000;
    return TRUE;
}

gboolean audacious_rc_song_frames(RemoteObject *obj, guint pos, int *length,
                                  GError **error) {
    *length = playlist_get_songtime(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_song_tuple(RemoteObject *obj, guint pos, gchar *field,
                                 GValue *value, GError **error) {
    TitleInput *tuple;
    tuple = playlist_get_tuple(playlist_get_active(), pos);
    if (!tuple) {
        return FALSE;
    } else {
        if (!strcasecmp(field, "performer")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->performer);
        } else if (!strcasecmp(field, "album_name")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->album_name);
        } else if (!strcasecmp(field, "track_name")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->track_name);
        } else if (!strcasecmp(field, "track_number")) {
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, tuple->track_number);
        } else if (!strcasecmp(field, "year")) {
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, tuple->year);
        } else if (!strcasecmp(field, "date")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->date);
        } else if (!strcasecmp(field, "genre")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->genre);
        } else if (!strcasecmp(field, "comment")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->comment);
        } else if (!strcasecmp(field, "file_name")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->file_name);
        } else if (!strcasecmp(field, "file_ext")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, g_strdup(tuple->file_ext));
        } else if (!strcasecmp(field, "file_path")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->file_path);
        } else if (!strcasecmp(field, "length")) {
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, tuple->length);
        } else if (!strcasecmp(field, "album_name")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->album_name);
        } else if (!strcasecmp(field, "formatter")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->formatter);
        } else if (!strcasecmp(field, "custom")) {
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple->custom);
        } else if (!strcasecmp(field, "mtime")) {
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, tuple->mtime);
        }
    }
    return TRUE;
}

gboolean audacious_rc_jump(RemoteObject *obj, guint pos, GError **error) {
    if (pos < (guint)playlist_get_length(playlist_get_active()))
                playlist_set_position(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_add(RemoteObject *obj, gchar *file, GError **error) {
    playlist_add_url(playlist_get_active(), file);
    return TRUE;
}
gboolean audacious_rc_add_url(RemoteObject *obj, gchar *url, GError **error) {
    playlist_add_url(playlist_get_active(), url);
    return TRUE;
}

gboolean audacious_rc_delete(RemoteObject *obj, guint pos, GError **error) {
    playlist_delete_index(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_clear(RemoteObject *obj, GError **error) {
    playlist_clear(playlist_get_active());
    mainwin_clear_song_info();
    return TRUE;
}

gboolean audacious_rc_auto_advance(RemoteObject *obj, gboolean *is_advance,
                                   GError **error) {
    *is_advance = cfg.no_playlist_advance;
    return TRUE;
}

gboolean audacious_rc_toggle_auto_advance(RemoteObject *obj, GError **error) {
    cfg.no_playlist_advance = !cfg.no_playlist_advance;
    return TRUE;
}

gboolean audacious_rc_repeat(RemoteObject *obj, gboolean *is_repeating,
                                GError **error) {
    *is_repeating = cfg.repeat;
    return TRUE;
}

gboolean audacious_rc_toggle_repeat(RemoteObject *obj, GError **error) {
    mainwin_repeat_pushed(!cfg.repeat);
    return TRUE;
}

gboolean audacious_rc_shuffle(RemoteObject *obj, gboolean *is_shuffling,
                                GError **error) {
    *is_shuffling = cfg.shuffle;
    return TRUE;
}

gboolean audacious_rc_toggle_shuffle(RemoteObject *obj, GError **error) {
    mainwin_shuffle_pushed(!cfg.shuffle);
    return TRUE;
}

DBusGProxy *audacious_get_dbus_proxy(void)
{
    DBusGConnection *connection = NULL;
    GError *error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    g_clear_error(&error);
    return dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                     AUDACIOUS_DBUS_PATH,
                                     AUDACIOUS_DBUS_INTERFACE);
}
