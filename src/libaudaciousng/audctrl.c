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

#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include "audacious/dbus.h"
#include "audacious/dbus-client-bindings.h"
#include "audctrl.h"

GError *error = NULL;

void audacious_remote_playlist(DBusGProxy *proxy, gchar **list, gint num,
                               gboolean enqueue) {
}

gint audacious_remote_get_version(DBusGProxy *proxy) {
    return 0;
}

void audacious_remote_playlist_add(DBusGProxy *proxy, GList *list) {
}

void audacious_remote_playlist_delete(DBusGProxy *proxy, gint pos) {
}

void audacious_remote_play(DBusGProxy *proxy) {
    org_atheme_audacious_playback_play(proxy, &error);
    g_error_free(error);
}

void audacious_remote_pause(DBusGProxy *proxy) {
    org_atheme_audacious_playback_pause(proxy, &error);
    g_error_free(error);
}

void audacious_remote_stop(DBusGProxy *proxy) {
    org_atheme_audacious_playback_stop(proxy, &error);
    g_error_free(error);
}

gboolean audacious_remote_is_playing(DBusGProxy *proxy) {
    gboolean is_playing;
    org_atheme_audacious_playback_playing(proxy, &is_playing, &error);
    g_error_free(error);
    return is_playing;
}

gboolean audacious_remote_is_paused(DBusGProxy *proxy) {
    gboolean is_paused;
    org_atheme_audacious_playback_paused(proxy, &is_paused, &error);
    g_error_free(error);
    return is_paused;
}

gint audacious_remote_get_playlist_pos(DBusGProxy *proxy) {
    gint pos;
    org_atheme_audacious_playlist_position(proxy, &pos, &error);
    g_error_free(error);
    return pos;
}

void audacious_remote_set_playlist_pos(DBusGProxy *proxy, gint pos) {
}

gint audacious_remote_get_playlist_length(DBusGProxy *proxy) {
    gint len;
    org_atheme_audacious_playlist_length(proxy, &len, &error);
    g_error_free(error);
    return len;
}

void audacious_remote_playlist_clear(DBusGProxy *proxy) {
    org_atheme_audacious_playlist_clear(proxy, &error);
    g_error_free(error);
}

gint audacious_remote_get_output_time(DBusGProxy *proxy) {
    gint time;
    org_atheme_audacious_playback_time(proxy, &time, &error);
    g_error_free(error);
    return time;
}

void audacious_remote_jump_to_time(DBusGProxy *proxy, gint pos) {
}

/**
 * audacious_remote_get_volume:
 * @proxy: DBus proxy for audacious
 * @vl: Pointer to integer containing the left channel's volume.
 * @vr: Pointer to integer containing the right channel's volume.
 *
 * Queries audacious about the current volume.
 **/
void audacious_remote_get_volume(DBusGProxy *proxy, gint * vl, gint * vr) {
    org_atheme_audacious_playback_volume(proxy, vl, vr, &error);
    g_error_free(error);
}

/**
 * audacious_remote_get_main_volume:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current volume.
 *
 * Return value: The current volume.
 **/
gint audacious_remote_get_main_volume(DBusGProxy *proxy) {
    gint vl, vr;

    audacious_remote_get_volume(proxy, &vl, &vr);

    return (vl > vr) ? vl : vr;
}

/**
 * audacious_remote_set_volume:
 * @proxy: DBus proxy for audacious
 * @vl: The volume for the left channel.
 * @vr: The volume for the right channel.
 *
 * Sets the volume for the left and right channels in Audacious.
 **/
void audacious_remote_set_volume(DBusGProxy *proxy, gint vl, gint vr) {
    org_atheme_audacious_playback_set_volume(proxy, vl, vr,  &error);
    g_error_free(error);
}


/**
 * audacious_remote_set_main_volume:
 * @proxy: DBus proxy for audacious
 * @v: The volume to set.
 *
 * Sets the volume in Audacious.
 **/
void audacious_remote_set_main_volume(DBusGProxy *proxy, gint v) {
    gint b, vl, vr;

    b = audacious_remote_get_balance(proxy);

    if (b < 0) {
        vl = v;
        vr = (v * (100 - abs(b))) / 100;
    } else if (b > 0) {
        vl = (v * (100 - b)) / 100;
        vr = v;
    } else
        vl = vr = v;
    audacious_remote_set_volume(proxy, vl, vr);
}

/**
 * audacious_remote_get_balance:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current balance.
 *
 * Return value: The current balance.
 **/
gint audacious_remote_get_balance(DBusGProxy *proxy) {
    gint balance;
    org_atheme_audacious_playback_balance(proxy, &balance,  &error);
    g_error_free(error);
    return balance;
}

void audacious_remote_playlist_add_url_string(DBusGProxy *proxy,
                                              gchar *string) {
    org_atheme_audacious_playlist_add_url(proxy, string, &error);
    g_error_free(error);
}
