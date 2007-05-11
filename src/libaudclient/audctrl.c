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
    org_atheme_audacious_play(proxy, &error);
    g_clear_error(&error);
}

void audacious_remote_pause(DBusGProxy *proxy) {
    org_atheme_audacious_pause(proxy, &error);
    g_clear_error(&error);
}

void audacious_remote_stop(DBusGProxy *proxy) {
    org_atheme_audacious_stop(proxy, &error);
    g_clear_error(&error);
}

gboolean audacious_remote_is_playing(DBusGProxy *proxy) {
    gboolean is_playing;
    org_atheme_audacious_playing(proxy, &is_playing, &error);
    g_clear_error(&error);
    return is_playing;
}

gboolean audacious_remote_is_paused(DBusGProxy *proxy) {
    gboolean is_paused;
    org_atheme_audacious_paused(proxy, &is_paused, &error);
    g_clear_error(&error);
    return is_paused;
}

gint audacious_remote_get_playlist_pos(DBusGProxy *proxy) {
    gint pos;
    org_atheme_audacious_position(proxy, &pos, &error);
    g_clear_error(&error);
    return pos;
}

void audacious_remote_set_playlist_pos(DBusGProxy *proxy, gint pos) {
}

gint audacious_remote_get_playlist_length(DBusGProxy *proxy) {
    gint len;
    org_atheme_audacious_length(proxy, &len, &error);
    g_clear_error(&error);
    return len;
}

void audacious_remote_playlist_clear(DBusGProxy *proxy) {
    org_atheme_audacious_clear(proxy, &error);
    g_clear_error(&error);
}

gint audacious_remote_get_output_time(DBusGProxy *proxy) {
    gint time;
    org_atheme_audacious_time(proxy, &time, &error);
    g_clear_error(&error);
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
    org_atheme_audacious_volume(proxy, vl, vr, &error);
    g_clear_error(&error);
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
 * audacious_remote_get_balance:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current balance.
 *
 * Return value: The current balance.
 **/
gint audacious_remote_get_balance(DBusGProxy *proxy) {
    gint balance;
    org_atheme_audacious_balance(proxy, &balance,  &error);
    g_clear_error(&error);
    return balance;
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
    org_atheme_audacious_set_volume(proxy, vl, vr,  &error);
    g_clear_error(&error);
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

void audacious_remote_set_balance(DBusGProxy *proxy, gint b) {
    gint v, vl, vr;

    if (b < -100)
        b = -100;
    if (b > 100)
        b = 100;

    v = audacious_remote_get_main_volume(proxy);

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

gchar *audacious_remote_get_skin(DBusGProxy *proxy) {
}

void audacious_remote_set_skin(DBusGProxy *proxy, gchar *skinfile) {
}

gchar *audacious_remote_get_playlist_file(DBusGProxy *proxy, gint pos) {
    gchar *out;
    org_atheme_audacious_song_filename(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

gchar *audacious_remote_get_playlist_title(DBusGProxy *proxy, gint pos) {
    gchar *out;
    org_atheme_audacious_song_title(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

gint audacious_remote_get_playlist_time(DBusGProxy *proxy, gint pos) {
    gint out;
    org_atheme_audacious_song_frames(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

void audacious_remote_get_info(DBusGProxy *proxy, gint *rate, gint *freq,
                               gint *nch) {
}

void audacious_remote_main_win_toggle(DBusGProxy *proxy, gboolean show) {
    const char* path = dbus_g_proxy_get_path(proxy);
    g_print("path: %s\n", path);
    org_atheme_audacious_show_main_win(proxy, show, &error);
    g_clear_error(&error);
}

void audacious_remote_pl_win_toggle(DBusGProxy *proxy, gboolean show) {
    org_atheme_audacious_show_playlist(proxy, show, &error);
    g_clear_error(&error);
}

void audacious_remote_eq_win_toggle(DBusGProxy *proxy, gboolean show) {
    org_atheme_audacious_show_equalizer(proxy, show, &error);
    g_clear_error(&error);
}

/**
 * xmms_remote_is_main_win:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries Audacious about the main window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean audacious_remote_is_main_win(DBusGProxy *proxy) {
    gboolean visible;
    org_atheme_audacious_main_win_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

gboolean audacious_remote_is_pl_win(DBusGProxy *proxy) {
    gboolean visible;
    org_atheme_audacious_playlist_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

gboolean audacious_remote_is_eq_win(DBusGProxy *proxy) {
    gboolean visible;
    org_atheme_audacious_equalizer_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

void audacious_remote_show_prefs_box(DBusGProxy *proxy) {
}

void audacious_remote_toggle_aot(DBusGProxy *proxy, gboolean ontop) {
}

void audacious_remote_eject(DBusGProxy *proxy) {
    org_atheme_audacious_eject(proxy, &error);
    g_clear_error(&error);
}

void audacious_remote_playlist_prev(DBusGProxy *proxy) {
}

void audacious_remote_playlist_next(DBusGProxy *proxy) {
}


void audacious_remote_playlist_add_url_string(DBusGProxy *proxy,
                                              gchar *string) {
    org_atheme_audacious_add_url(proxy, string, &error);
    g_clear_error(&error);
}

gboolean audacious_remote_is_running(DBusGProxy *proxy) {
}

void audacious_remote_toggle_repeat(DBusGProxy *proxy) {
}

void audacious_remote_toggle_shuffle(DBusGProxy *proxy) {
}

gboolean audacious_remote_is_repeat(DBusGProxy *proxy) {
}

gboolean audacious_remote_is_shuffle(DBusGProxy *proxy) {
}

void audacious_remote_get_eq(DBusGProxy *proxy, gfloat *preamp,
                             gfloat **bands) {
}

gfloat audacious_remote_get_eq_preamp(DBusGProxy *proxy) {
}

gfloat audacious_remote_get_eq_band(DBusGProxy *proxy, gint band) {
}

void audacious_remote_set_eq(DBusGProxy *proxy, gfloat preamp,
                             gfloat *bands) {
}

void audacious_remote_set_eq_preamp(DBusGProxy *proxy, gfloat preamp) {
}

void audacious_remote_set_eq_band(DBusGProxy *proxy, gint band,
                                  gfloat value) {
}

void audacious_remote_quit(DBusGProxy *proxy) {
    org_atheme_audacious_quit(proxy, &error);
    g_clear_error(&error);
}

void audacious_remote_play_pause(DBusGProxy *proxy) {
}

void audacious_remote_playlist_ins_url_string(DBusGProxy *proxy,
                                              gchar *string, gint pos) {
}

void audacious_remote_playqueue_add(DBusGProxy *proxy, gint pos) {
}

void audacious_remote_playqueue_remove(DBusGProxy *proxy, gint pos) {
}

gint audacious_remote_get_playqueue_length(DBusGProxy *proxy) {
}

void audacious_remote_toggle_advance(DBusGProxy *proxy) {
}

gboolean audacious_remote_is_advance(DBusGProxy *proxy) {
}

void audacious_remote_activate(DBusGProxy *proxy) {
}

void audacious_remote_show_jtf_box(DBusGProxy *proxy) {
}

void audacious_remote_playqueue_clear(DBusGProxy *proxy) {
}

gboolean audacious_remote_playqueue_is_queued(DBusGProxy *proxy, gint pos) {
}

gint audacious_remote_get_playqueue_position(DBusGProxy *proxy, gint pos) {
}

gint audacious_remote_get_playqueue_queue_position(DBusGProxy *proxy,
                                                   gint pos) {
}

void audacious_remote_playlist_enqueue_to_temp(DBusGProxy *proxy,
                                               gchar *string) {
}

gchar *audacious_get_tuple_field_data(DBusGProxy *proxy, gchar *field,
                                      gint pos) {
}
