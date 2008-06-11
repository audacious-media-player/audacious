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
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "dbus.h"
#include "dbus-service.h"
#include "dbus-server-bindings.h"

#include <math.h>
#include "main.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "tuple.h"
#include "strings.h"

#include "ui_equalizer.h"
#include "ui_skin.h"

static DBusGConnection *dbus_conn = NULL;
static guint signals[LAST_SIG] = { 0 };

G_DEFINE_TYPE(RemoteObject, audacious_rc, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisRoot, mpris_root, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisPlayer, mpris_player, G_TYPE_OBJECT);
G_DEFINE_TYPE(MprisTrackList, mpris_tracklist, G_TYPE_OBJECT);

#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

void audacious_rc_class_init(RemoteObjectClass *klass) {}
void mpris_root_class_init(MprisRootClass *klass) {}

void mpris_player_class_init(MprisPlayerClass *klass) {
    signals[CAPS_CHANGE_SIG] =
        g_signal_new("caps_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__INT,
                G_TYPE_NONE, 1, G_TYPE_INT);
    signals[TRACK_CHANGE_SIG] =
        g_signal_new("track_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__BOXED,
                G_TYPE_NONE, 1, DBUS_TYPE_G_STRING_VALUE_HASHTABLE);
    signals[STATUS_CHANGE_SIG] =
        g_signal_new("status_change",
            G_OBJECT_CLASS_TYPE(klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__INT,
                G_TYPE_NONE, 1, G_TYPE_INT);
}

void mpris_tracklist_class_init(MprisTrackListClass *klass) {}

void audacious_rc_init(RemoteObject *object) {
    GError *error = NULL;
    DBusGProxy *driver_proxy;
    guint request_ret;

    g_message("Registering remote D-Bus interfaces");
    
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

    // Add signals
    DBusGProxy *proxy = object->proxy;
    if (proxy != NULL) {
        dbus_g_proxy_add_signal(proxy, "StatusChange",
            G_TYPE_INT, G_TYPE_INVALID);
        dbus_g_proxy_add_signal(proxy, "CapsChange",
            G_TYPE_INT, G_TYPE_INVALID);
        dbus_g_proxy_add_signal(proxy, "TrackChange",
            DBUS_TYPE_G_STRING_VALUE_HASHTABLE, G_TYPE_INVALID);
    } else {
        /* XXX / FIXME: Why does this happen? -- ccr */
        g_warning("in mpris_player_init object->proxy == NULL, not adding some signals.");
    }
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
    DBusConnection *local_conn;
    // Initialize the DBus connection
    g_message("Trying to initialize D-Bus");
    dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_conn == NULL) {
        g_warning("Unable to connect to dbus: %s", error->message);
        g_error_free(error);
        return;
    }

    g_type_init();
    g_message("D-Bus RC");
    g_object_new(audacious_rc_get_type(), NULL);
    g_message("D-Bus MPRIS root");
    g_object_new(mpris_root_get_type(), NULL);
    g_message("D-Bus MPRIS player");
    mpris = g_object_new(mpris_player_get_type(), NULL);
    g_message("result=%p", mpris);
    g_message("D-Bus MPRIS tracklist");
    g_object_new(mpris_tracklist_get_type(), NULL);
    g_message("D-Bus support has been activated");

    local_conn = dbus_g_connection_get_connection(dbus_conn);
    dbus_connection_set_exit_on_disconnect(local_conn, FALSE);
}

GValue *tuple_value_to_gvalue(Tuple *tuple, const gchar *key) {
    GValue *val;
    TupleValueType type;
    type = tuple_get_value_type(tuple, -1, key);
    if (type == TUPLE_STRING) {
        gchar *result = str_to_utf8(tuple_get_string(tuple, -1, key));

        val = g_new0(GValue, 1);
        g_value_init(val, G_TYPE_STRING);
        g_value_take_string(val, result);
        return val;
    } else if (type == TUPLE_INT) {
        val = g_new0(GValue, 1);
        g_value_init(val, G_TYPE_INT);
        g_value_set_int(val, tuple_get_int(tuple, -1, key));
        return val;
    }
    return NULL;
}

static void tuple_insert_to_hash(GHashTable *md, Tuple *tuple, const gchar *key)
{
    GValue *value = tuple_value_to_gvalue(tuple, key);
    if (value != NULL)
        g_hash_table_insert(md, key, value);
}

static void remove_metadata_value(gpointer value)
{
    g_value_unset((GValue*)value);
    g_free((GValue*)value);
}

GHashTable *mpris_metadata_from_tuple(Tuple *tuple) {
    GHashTable *md = NULL;
    GValue *value;

    if (tuple == NULL)
        return NULL;

    md = g_hash_table_new_full(g_str_hash, g_str_equal,
                               NULL, remove_metadata_value);

    tuple_insert_to_hash(md, tuple, "length");
    tuple_insert_to_hash(md, tuple, "title");
    tuple_insert_to_hash(md, tuple, "artist");
    tuple_insert_to_hash(md, tuple, "album");
    tuple_insert_to_hash(md, tuple, "genre");
    tuple_insert_to_hash(md, tuple, "codec");
    tuple_insert_to_hash(md, tuple, "quality");

    return md;
}

/* MPRIS API */
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
    aud_quit();
    return TRUE;
}
gboolean mpris_player_get_status(MprisPlayer *obj, gint *status,
                                 GError **error) {
    // check paused before playing because playback_get_playing() is true when
    // paused as well as when playing
    if (playback_get_paused())
        *status = MPRIS_STATUS_PAUSE;
    else if (playback_get_playing())
        *status = MPRIS_STATUS_PLAY;
    else
        *status = MPRIS_STATUS_STOP;
    return TRUE;
}
gboolean mpris_player_get_metadata(MprisPlayer *obj, GHashTable **metadata,
                                   GError **error) {
    GHashTable *md = NULL;
    Tuple *tuple = NULL;
    GValue *value;
    Playlist *active;

    active = playlist_get_active();
    gint pos = playlist_get_position(active);
    tuple = playlist_get_tuple(active, pos);

    md = mpris_metadata_from_tuple(tuple);

    if (md == NULL) {
        // there's no metadata for this track
        return TRUE;
    }

    // Song URI
    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_take_string(value, playlist_get_filename(active, pos));

    g_hash_table_insert(md, "URI", value);

    *metadata = md;

    return TRUE;
}
gboolean mpris_player_get_caps(MprisPlayer *obj, gint *capabilities,
                                 GError **error) {
    *capabilities = MPRIS_CAPS_CAN_GO_NEXT |
        MPRIS_CAPS_CAN_GO_PREV |
        MPRIS_CAPS_CAN_PAUSE |
        MPRIS_CAPS_CAN_PLAY |
        MPRIS_CAPS_CAN_SEEK |
        MPRIS_CAPS_CAN_PROVIDE_METADATA |
        MPRIS_CAPS_PROVIDES_TIMING;
    return TRUE;
}
gboolean mpris_player_volume_set(MprisPlayer *obj, gint vol, GError **error) {
    gint vl, vr, v;

    // get the current volume so we can maintain the balance
    input_get_volume(&vl, &vr);

    // sanity check
    vl = CLAMP(vl, 0, 100);
    vr = CLAMP(vr, 0, 100);
    v = CLAMP(vol, 0, 100);

    if (vl > vr) {
        input_set_volume(v, (gint) rint(((gdouble) vr / vl) * v));
    } else if (vl < vr) {
        input_set_volume((gint) rint(((gdouble) vl / vr) * v), v);
    } else {
        input_set_volume(v, v);
    }
    return TRUE;
}
gboolean mpris_player_volume_get(MprisPlayer *obj, gint *vol,
                                 GError **error) {
    gint vl, vr;
    input_get_volume(&vl, &vr);
    // vl and vr may be different depending on the balance; the true volume is
    // the maximum of vl or vr.
    *vol = MAX(vl, vr);
    return TRUE;
}
gboolean mpris_player_position_set(MprisPlayer *obj, gint pos,
                                   GError **error) {
    gint time = CLAMP(pos / 1000, 0,
            playlist_get_current_length(playlist_get_active()) / 1000 - 1);
    playback_seek(time);
    return TRUE;
}
gboolean mpris_player_position_get(MprisPlayer *obj, gint *pos,
                                   GError **error) {
    if (playback_get_playing())
        *pos = playback_get_time();
    else
        *pos = 0;
    return TRUE;
}
// MPRIS /Player signals
gboolean mpris_emit_caps_change(MprisPlayer *obj) {
    g_signal_emit(obj, signals[CAPS_CHANGE_SIG], 0, 0);
    return TRUE;
}

gboolean mpris_emit_track_change(MprisPlayer *obj) {
    GHashTable *metadata;
    Tuple *tuple = NULL;
    GValue *value;
    Playlist *active;

    active = playlist_get_active();
    gint pos = playlist_get_position(active);
    tuple = playlist_get_tuple(active, pos);

    metadata = mpris_metadata_from_tuple(tuple);

    if (!metadata)
        metadata = g_hash_table_new_full(g_str_hash, g_str_equal,
                                         NULL, remove_metadata_value);

    // Song URI
    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_take_string(value, playlist_get_filename(active, pos));

    g_hash_table_insert(metadata, "URI", value);

    g_signal_emit(obj, signals[TRACK_CHANGE_SIG], 0, metadata);
    g_hash_table_destroy(metadata);
    return TRUE;
}

gboolean mpris_emit_status_change(MprisPlayer *obj, PlaybackStatus status) {
    g_signal_emit(obj, signals[STATUS_CHANGE_SIG], 0, status);
    return TRUE;
}

// MPRIS /TrackList
gboolean mpris_tracklist_get_metadata(MprisTrackList *obj, gint pos,
                                      GHashTable **metadata, GError **error) {
    GHashTable *md = NULL;
    Tuple *tuple = NULL;
    GValue *value;
    Playlist *active;

    active = playlist_get_active();
    tuple = playlist_get_tuple(active, pos);

    md = mpris_metadata_from_tuple(tuple);

    if (md == NULL) {
        // there's no metadata for this track
        return TRUE;
    }

    // Song URI
    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_take_string(value, playlist_get_filename(active, pos));

    g_hash_table_insert(md, "URI", value);

    *metadata = md;

    return TRUE;
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
    guint num_added;
    num_added = playlist_add_url(playlist_get_active(), uri);
    if (play && num_added > 0) {
        gint pos = playlist_get_length(playlist_get_active()) - 1;
        playlist_set_position(playlist_get_active(), pos);
        playback_initiate();
    }
    // TODO: set an error if num_added == 0
    return TRUE;
}
gboolean mpris_tracklist_del_track(MprisTrackList *obj, gint pos,
                                   GError **error) {
    playlist_delete_index(playlist_get_active(), pos);
    return TRUE;
}
gboolean mpris_tracklist_loop(MprisTrackList *obj, gboolean loop,
                              GError **error) {
    mainwin_repeat_pushed(loop);
    if (loop) {
        mainwin_set_noplaylistadvance(FALSE);
        mainwin_set_stopaftersong(FALSE);
    }
    return TRUE;
}
gboolean mpris_tracklist_random(MprisTrackList *obj, gboolean random,
                                GError **error) {
    mainwin_shuffle_pushed(random);
    return TRUE;
}

// Audacious General Information
gboolean audacious_rc_version(RemoteObject *obj, gchar **version, GError **error) {
    *version = g_strdup(VERSION);
    return TRUE;
}

gboolean audacious_rc_quit(RemoteObject *obj, GError **error) {
    aud_quit();
    return TRUE;
}

gboolean audacious_rc_eject(RemoteObject *obj, GError **error) {
    drct_eject();
    return TRUE;
}

gboolean audacious_rc_main_win_visible(RemoteObject *obj,
                                       gboolean *is_main_win, GError **error) {
    *is_main_win = cfg.player_visible;
    return TRUE;
}

gboolean audacious_rc_show_main_win(RemoteObject *obj, gboolean show,
                                    GError **error) {
    drct_main_win_toggle(show);
    return TRUE;
}

gboolean audacious_rc_equalizer_visible(RemoteObject *obj,
                                        gboolean *is_eq_win, GError **error) {
    *is_eq_win = cfg.equalizer_visible;
    return TRUE;
}

gboolean audacious_rc_show_equalizer(RemoteObject *obj, gboolean show,
                                     GError **error) {
    drct_eq_win_toggle(show);
    return TRUE;
}

gboolean audacious_rc_playlist_visible(RemoteObject *obj, gboolean *is_pl_win,
                                       GError **error) {
    *is_pl_win = cfg.playlist_visible;
    return TRUE;
}

gboolean audacious_rc_show_playlist(RemoteObject *obj, gboolean show,
                                    GError **error) {
    drct_pl_win_toggle(show);
    return TRUE;
}

gboolean audacious_rc_get_tuple_fields(RemoteObject *obj, gchar ***fields,
                                    GError **error) {
    gchar **res = g_new0(gchar *, FIELD_LAST);
    gint i;
    for (i = 0; i < FIELD_LAST; i++) {
        res[i] = g_strdup(tuple_fields[i].name);
    }
    *fields = res;
    
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
gboolean audacious_rc_position(RemoteObject *obj, gint *pos, GError **error) {
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

gboolean audacious_rc_length(RemoteObject *obj, gint *length,
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

gboolean audacious_rc_song_length(RemoteObject *obj, guint pos, gint *length,
                                  GError **error) {
    *length = playlist_get_songtime(playlist_get_active(), pos) / 1000;
    return TRUE;
}

gboolean audacious_rc_song_frames(RemoteObject *obj, guint pos, gint *length,
                                  GError **error) {
    *length = playlist_get_songtime(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_song_tuple(RemoteObject *obj, guint pos, gchar *field,
                                 GValue *value, GError **error) {
    Tuple *tuple;
    tuple = playlist_get_tuple(playlist_get_active(), pos);
    if (!tuple) {
        return FALSE;
    } else {
        TupleValueType type = tuple_get_value_type(tuple, -1, field);

        switch(type)
        {
        case TUPLE_STRING:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, tuple_get_string(tuple, -1, field));
            break;
        case TUPLE_INT:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, tuple_get_int(tuple, -1, field));
            break;
        default:
            return FALSE;
            break;
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

/* New on Oct 5 */
gboolean audacious_rc_show_prefs_box(RemoteObject *obj, gboolean show, GError **error) {
    hook_call("prefswin show", &show);
    return TRUE;
}

gboolean audacious_rc_show_about_box(RemoteObject *obj, gboolean show, GError **error) {
    hook_call("aboutwin show", &show);
    return TRUE;
}

gboolean audacious_rc_show_jtf_box(RemoteObject *obj, gboolean show, GError **error) {
    hook_call("ui jump to track show", &show);
    return TRUE;
}

gboolean audacious_rc_show_filebrowser(RemoteObject *obj, gboolean show, GError **error)
{
    gboolean play_button = FALSE;
    if (show)
        hook_call("filebrowser show", &play_button);
    else
        hook_call("filebrowser hide", NULL);
    return TRUE;
}

gboolean audacious_rc_play_pause(RemoteObject *obj, GError **error) {
    if (playback_get_playing())
        playback_pause();
    else
        playback_initiate();
    return TRUE;
}

gboolean audacious_rc_activate(RemoteObject *obj, GError **error) {
    gtk_window_present(GTK_WINDOW(mainwin));
    return TRUE;
}

/* TODO: these skin functions should be removed when skin functionality
 * disappears --mf0102 */
gboolean audacious_rc_get_skin(RemoteObject *obj, gchar **skin, GError **error) {
    *skin = g_strdup(aud_active_skin->path);
    return TRUE;
}

gboolean audacious_rc_set_skin(RemoteObject *obj, gchar *skin, GError **error) {
    aud_active_skin_load(skin);
    return TRUE;
}

gboolean audacious_rc_get_info(RemoteObject *obj, gint *rate, gint *freq, gint *nch, GError **error) {
    playback_get_sample_params(rate, freq, nch);
    return TRUE;
}

gboolean audacious_rc_toggle_aot(RemoteObject *obj, gboolean ontop, GError **error) {
    hook_call("mainwin set always on top", &ontop);
    return TRUE;
}

/* New on Oct 9: Queue */
gboolean audacious_rc_playqueue_add(RemoteObject *obj, gint pos, GError **error) {
    if (pos < (guint)playlist_get_length(playlist_get_active()))
        playlist_queue_position(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_playqueue_remove(RemoteObject *obj, gint pos, GError **error) {
    if (pos < (guint)playlist_get_length(playlist_get_active()))
        playlist_queue_remove(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_playqueue_clear(RemoteObject *obj, GError **error) {
    playlist_clear_queue(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_get_playqueue_length(RemoteObject *obj, gint *length, GError **error) {
    *length = playlist_queue_get_length(playlist_get_active());
    return TRUE;
}

gboolean audacious_rc_queue_get_list_pos(RemoteObject *obj, gint qpos, gint *pos, GError **error) {
    if (playback_get_playing())
        *pos = playlist_get_queue_qposition_number(playlist_get_active(), qpos);

    return TRUE;
}

gboolean audacious_rc_queue_get_queue_pos(RemoteObject *obj, gint pos, gint *qpos, GError **error) {
    if (playback_get_playing())
        *qpos = playlist_get_queue_position_number(playlist_get_active(), pos);

    return TRUE;
}

gboolean audacious_rc_playqueue_is_queued(RemoteObject *obj, gint pos, gboolean *is_queued, GError **error) {
    *is_queued = playlist_is_position_queued(playlist_get_active(), pos);
    return TRUE;
}

gboolean audacious_rc_playlist_ins_url_string(RemoteObject *obj, gchar *url, gint pos, GError **error) {
    if (pos >= 0 && url && strlen(url)) {
        playlist_ins_url(playlist_get_active(), url, pos);
    }
    return TRUE;
}

gboolean audacious_rc_playlist_add(RemoteObject *obj, gpointer list, GError **error) {
    return playlist_add_url(playlist_get_active(), (gchar *) list) > 0;
}

gboolean audacious_rc_playlist_enqueue_to_temp(RemoteObject *obj, gchar *url, GError **error) {
    GList *playlists = NULL;
    Playlist *new_pl = playlist_new();
    gchar *pl_name = NULL;

    pl_name = (gchar*)playlist_get_current_name(new_pl);
    if(!pl_name)
        pl_name = g_strdup("New Playlist");
    playlist_set_current_name(new_pl, pl_name);
    g_free(pl_name);

    playlists = playlist_get_playlists();
    playlist_add_playlist(new_pl);

//    DISABLE_MANAGER_UPDATE();
    playlist_select_playlist(new_pl);
//    ENABLE_MANAGER_UPDATE();

    playlist_add_url(new_pl, url);

    return TRUE;
}

/* New on Nov 7: Equalizer */ 
gboolean audacious_rc_get_eq(RemoteObject *obj, gdouble *preamp, GArray **bands, GError **error)
{
    int i;

    *preamp = (gdouble)equalizerwin_get_preamp();
    *bands = g_array_sized_new(FALSE, FALSE, sizeof(gdouble), AUD_EQUALIZER_NBANDS);

    for(i=0; i < AUD_EQUALIZER_NBANDS; i++){
        gdouble val = (gdouble)equalizerwin_get_band(i);
        g_array_append_val(*bands, val);
    }

    return TRUE;
}

gboolean audacious_rc_get_eq_preamp(RemoteObject *obj, gdouble *preamp, GError **error)
{
    *preamp = (gdouble)equalizerwin_get_preamp();
    return TRUE;
}

gboolean audacious_rc_get_eq_band(RemoteObject *obj, gint band, gdouble *value, GError **error)
{
    *value = (gdouble)equalizerwin_get_band(band);
    return TRUE;
}

gboolean audacious_rc_set_eq(RemoteObject *obj, gdouble preamp, GArray *bands, GError **error)
{
    gdouble element;
    int i;
    
    equalizerwin_set_preamp((gfloat)preamp);

    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++) {
        element = g_array_index(bands, gdouble, i);
        equalizerwin_set_band(i, (gfloat)element);
    }
    equalizerwin_eq_changed();

    return TRUE;
}

gboolean audacious_rc_set_eq_preamp(RemoteObject *obj, gdouble preamp, GError **error)
{
    equalizerwin_set_preamp((gfloat)preamp);
    equalizerwin_eq_changed();
    return TRUE;
}

gboolean audacious_rc_set_eq_band(RemoteObject *obj, gint band, gdouble value, GError **error)
{
    equalizerwin_set_band(band, (gfloat)value);
    equalizerwin_eq_changed();
    return TRUE;
}

gboolean audacious_rc_equalizer_activate(RemoteObject *obj, gboolean active, GError **error)
{
    equalizer_activate(active);
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
