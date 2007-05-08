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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "dbus.h"
#include "dbus-bindings.h"

#include "main.h"
#include "ui_equalizer.h"
#include "ui_main.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "libaudacious/beepctrl.h"
#include "memorypool.h"
#include "titlestring.h"
#include "ui_jumptotrack.h"

G_DEFINE_TYPE(RemoteObject, audacious_remote, G_TYPE_OBJECT);

void audacious_remote_class_init(RemoteObjectClass *klass) {}

void audacious_remote_init(RemoteObject *object) {
    GError *error = NULL;
    DBusGProxy *driver_proxy;
    unsigned int request_ret;

    // Initialize the DBus connection
    object->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (object->connection == NULL) {
        g_warning("Unable to connect to dbus: %s", error->message);
        g_error_free(error);
        return;
    }
    
    dbus_g_object_type_install_info(audacious_remote_get_type(),
                                    &dbus_glib_audacious_remote_object_info);
    
    // Register DBUS path
    dbus_g_connection_register_g_object(object->connection, DBUS_OBJECT_PATH,
                                        G_OBJECT(object));

    // Register the service name, the constants here are defined in
    // dbus-glib-bindings.h
    driver_proxy = dbus_g_proxy_new_for_name(object->connection,
                                             DBUS_SERVICE_DBUS, DBUS_PATH_DBUS,
                                             DBUS_INTERFACE_DBUS);

    if (!org_freedesktop_DBus_request_name(driver_proxy, DBUS_SERVICE, 0,
            &request_ret, &error)) {
        g_warning("Unable to register service: %s", error->message);
        g_error_free(error);
    }
    
    g_object_unref(driver_proxy);
}

void init_dbus() {
    g_type_init();
    g_object_new(audacious_remote_get_type(), NULL);
}

// Audacious General Information
gboolean audacious_remote_version(RemoteObject *obj, gchar **version,
                                  GError **error) {
    *version = g_strdup(VERSION);
    return TRUE;
}

// Playback Information/Manipulation
gboolean audacious_remote_play(RemoteObject *obj, GError **error) {
    if (playback_get_paused())
        playback_pause();
    else if (playlist_get_length(playlist_get_active()))
        playback_initiate();
    else
        mainwin_eject_pushed();
    return TRUE;
}

gboolean audacious_remote_pause(RemoteObject *obj, GError **error) {
    playback_pause();
    return TRUE;
}

gboolean audacious_remote_stop(RemoteObject *obj, GError **error) {
    ip_data.stop = TRUE;
    playback_stop();
    ip_data.stop = FALSE;
    mainwin_clear_song_info();
    return TRUE;
}

gboolean audacious_remote_playing(RemoteObject *obj, gboolean *is_playing,
                                  GError **error) {
    *is_playing = playback_get_playing();
    return TRUE;
}

gboolean audacious_remote_paused(RemoteObject *obj, gboolean *is_paused,
                                 GError **error) {
    *is_paused = playback_get_paused();
    return TRUE;
}

gboolean audacious_remote_stopped(RemoteObject *obj, gboolean *is_stopped,
                                  GError **error) {
    *is_stopped = !playback_get_playing();
    return TRUE;
}

gboolean audacious_remote_status(RemoteObject *obj, gchar **status,
                                 GError **error) {
    if (playback_get_paused())
        *status = g_strdup("paused");
    else if (playback_get_playing())
        *status = g_strdup("playing");
    else
        *status = g_strdup("stopped");
    return TRUE;
}

gboolean audacious_remote_seek(RemoteObject *obj, guint pos, GError **error) {
    if (playlist_get_current_length(playlist_get_active()) > 0 &&
            pos < (guint)playlist_get_current_length(playlist_get_active()))
            playback_seek(pos / 1000);

    return TRUE;
}

// Playlist Information/Manipulation
gboolean audacious_remote_position(RemoteObject *obj, int *pos, GError **error)
{
    return TRUE;
}

gboolean audacious_remote_advance(RemoteObject *obj, GError **error) {
    playlist_next(playlist_get_active());
    return TRUE;
}

gboolean audacious_remote_reverse(RemoteObject *obj, GError **error) {
    playlist_prev(playlist_get_active());
    return TRUE;
}

gboolean audacious_remote_length(RemoteObject *obj, int *length,
                                 GError **error) {
    return TRUE;
}

gboolean audacious_remote_song_title(RemoteObject *obj, int pos,
                                     gchar **title, GError **error) {
    return TRUE;
}

gboolean audacious_remote_song_filename(RemoteObject *obj, int pos,
                                        gchar **filename, GError **error) {
    return TRUE;
}

gboolean audacious_remote_song_length(RemoteObject *obj, int pos, int *length,
                                      GError **error) {
    return TRUE;
}

gboolean audacious_remote_song_frames(RemoteObject *obj, int pos, int *length,
                                      GError **error) {
    return TRUE;
}

gboolean audacious_remote_jump(RemoteObject *obj, int pos, GError **error) {
    return TRUE;
}

gboolean audacious_remote_add_url(RemoteObject *obj, gchar *url,
                                  GError **error) {
    return TRUE;
}

gboolean audacious_remote_delete(RemoteObject *obj, int pos, GError **error) {
    return TRUE;
}

gboolean audacious_remote_clear(RemoteObject *obj, GError **error) {
    return TRUE;
}

gboolean audacious_remote_repeating(RemoteObject *obj, gboolean *is_repeating,
                                    GError **error) {
    return TRUE;
}

gboolean audacious_remote_repeat(RemoteObject *obj, GError **error) {
    return TRUE;
}

gboolean audacious_remote_shuffling(RemoteObject *obj, gboolean *is_shuffling,
                                    GError **error) {
    return TRUE;
}

gboolean audacious_remote_shuffle(RemoteObject *obj, GError **error) {
    return TRUE;
}
