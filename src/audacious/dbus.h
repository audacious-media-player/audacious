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

#ifndef _AUDDBUS_H
#define _AUDDBUS_H

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

#define DBUS_SERVICE "org.atheme.audacious"
#define DBUS_OBJECT_PATH "/org/atheme/audacious"

typedef struct {
    GObject parent;
    DBusGConnection *connection;
} RemoteObject;

typedef struct {
    GObjectClass parent_class;
} RemoteObjectClass;

void init_dbus();

// Audacious General Information
gboolean audacious_remote_version(RemoteObject *obj, gchar **version,
                                  GError **error);

// Playback Information/Manipulation
gboolean audacious_remote_play(RemoteObject *obj, GError **error);
gboolean audacious_remote_pause(RemoteObject *obj, GError **error);
gboolean audacious_remote_stop(RemoteObject *obj, GError **error);
gboolean audacious_remote_playing(RemoteObject *obj, gboolean *is_playing,
                                  GError **error);
gboolean audacious_remote_paused(RemoteObject *obj, gboolean *is_paused,
                                 GError **error);
gboolean audacious_remote_stopped(RemoteObject *obj, gboolean *is_stopped,
                                  GError **error);
gboolean audacious_remote_status(RemoteObject *obj, gchar **status,
                                 GError **error);
gboolean audacious_remote_seek(RemoteObject *obj, guint pos, GError **error);

// Playlist Information/Manipulation
gboolean audacious_remote_position(RemoteObject *obj, int *pos, GError **error);
gboolean audacious_remote_advance(RemoteObject *obj, GError **error);
gboolean audacious_remote_reverse(RemoteObject *obj, GError **error);
gboolean audacious_remote_length(RemoteObject *obj, int *length,
                                 GError **error);
gboolean audacious_remote_song_title(RemoteObject *obj, int pos,
                                     gchar **title, GError **error);
gboolean audacious_remote_song_filename(RemoteObject *obj, int pos,
                                        gchar **filename, GError **error);
gboolean audacious_remote_song_length(RemoteObject *obj, int pos, int *length,
                                      GError **error);
gboolean audacious_remote_song_frames(RemoteObject *obj, int pos, int *length,
                                      GError **error);
gboolean audacious_remote_jump(RemoteObject *obj, int pos, GError **error);
gboolean audacious_remote_add_url(RemoteObject *obj, gchar *url,
                                  GError **error);
gboolean audacious_remote_delete(RemoteObject *obj, int pos, GError **error);
gboolean audacious_remote_clear(RemoteObject *obj, GError **error);
gboolean audacious_remote_repeating(RemoteObject *obj, gboolean *is_repeating,
                                    GError **error);
gboolean audacious_remote_repeat(RemoteObject *obj, GError **error);
gboolean audacious_remote_shuffling(RemoteObject *obj, gboolean *is_shuffling,
                                    GError **error);
gboolean audacious_remote_shuffle(RemoteObject *obj, GError **error);
#endif // !_AUDDBUS_H
