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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <dbus/dbus-glib.h>
#include "audacious/dbus.h"
#include "audacious/dbus-client-bindings.h"
#include "audctrl.h"

static GError *error = NULL; //it must be hidden from outside, otherwise symbol conflict likely to happen.

/**
 * audacious_remote_playlist:
 * @proxy: DBus proxy for audacious
 * @list: A list of URIs to play.
 * @num: Number of URIs to play.
 * @enqueue: Whether or not the new playlist should be added on, or replace the current playlist.
 *
 * Sends a playlist to audacious.
 **/
void audacious_remote_playlist(DBusGProxy *proxy, gchar **list, gint num, gboolean enqueue) {
    GList *glist = NULL;
    gchar **data = list;

    g_return_if_fail(list != NULL);
    g_return_if_fail(num > 0);

    if (!enqueue)
        audacious_remote_playlist_clear(proxy);

    // construct a GList
    while(data) {
        glist = g_list_append(glist, (gpointer)data);
        data++;
    }

    org_atheme_audacious_playlist_add(proxy, (gpointer)glist, &error);

    g_list_free(glist);
    glist = NULL;

    if (!enqueue)
        audacious_remote_play(proxy);
}

/**
 * audacious_remote_get_version:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious for it's version.
 *
 * Return value: The version of Audacious.
 **/
gchar *audacious_remote_get_version(DBusGProxy *proxy) {
    char *string = NULL;
    org_atheme_audacious_version(proxy, &string, &error);
    g_clear_error(&error);

    return (string ? string : NULL);
}

/**
 * audacious_remote_playlist_add:
 * @proxy: DBus proxy for audacious
 * @list: A GList of URIs to add to the playlist.
 *
 * Sends a list of URIs to Audacious to add to the playlist.
 **/
void audacious_remote_playlist_add(DBusGProxy *proxy, GList *list) {
	GList *iter;
	for (iter = list; iter != NULL; iter = g_list_next(iter))
		org_atheme_audacious_playlist_add(proxy, iter->data, &error);
	g_clear_error(&error);
}

/**
 * audacious_remote_playlist_delete:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to delete.
 *
 * Deletes a playlist entry.
 **/
void audacious_remote_playlist_delete(DBusGProxy *proxy, guint pos) {
    org_atheme_audacious_delete(proxy, pos, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_play:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to begin playback.
 **/
void audacious_remote_play(DBusGProxy *proxy) {
    org_atheme_audacious_play(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_pause:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to pause.
 **/
void audacious_remote_pause(DBusGProxy *proxy) {
    org_atheme_audacious_pause(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_stop:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to stop.
 **/
void audacious_remote_stop(DBusGProxy *proxy) {
    org_atheme_audacious_stop(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_is_playing:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about whether it is playing or not.
 *
 * Return value: TRUE if playing, FALSE otherwise.
 **/
gboolean audacious_remote_is_playing(DBusGProxy *proxy) {
    gboolean is_playing = FALSE;
    org_atheme_audacious_playing(proxy, &is_playing, &error);
    g_clear_error(&error);
    return is_playing;
}

/**
 * audacious_remote_is_paused:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about whether it is paused or not.
 *
 * Return value: TRUE if playing, FALSE otherwise.
 **/
gboolean audacious_remote_is_paused(DBusGProxy *proxy) {
    gboolean is_paused = FALSE;
    org_atheme_audacious_paused(proxy, &is_paused, &error);
    g_clear_error(&error);
    return is_paused;
}

/**
 * audacious_remote_get_playlist_pos:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current playlist position.
 *
 * Return value: The current playlist position.
 **/
gint audacious_remote_get_playlist_pos(DBusGProxy *proxy) {
    guint pos = 0;
    org_atheme_audacious_position(proxy, &pos, &error);
    g_clear_error(&error);
    return pos;
}

/**
 * audacious_remote_set_playlist_pos:
 * @proxy: DBus proxy for audacious
 * @pos: Playlist position to jump to.
 *
 * Tells audacious to jump to a different playlist position.
 **/
void audacious_remote_set_playlist_pos(DBusGProxy *proxy, guint pos) {
    org_atheme_audacious_jump (proxy, pos, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_get_playlist_length:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current playlist length.
 *
 * Return value: The amount of entries in the playlist.
 **/
gint audacious_remote_get_playlist_length(DBusGProxy *proxy) {
    gint len = 0;
    org_atheme_audacious_length(proxy, &len, &error);
    g_clear_error(&error);
    return len;
}

/**
 * audacious_remote_playlist_clear:
 * @proxy: DBus proxy for audacious
 *
 * Clears the playlist.
 **/
void audacious_remote_playlist_clear(DBusGProxy *proxy) {
    org_atheme_audacious_clear(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_get_output_time:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the current output position.
 *
 * Return value: The current output position.
 **/
gint audacious_remote_get_output_time(DBusGProxy *proxy) {
    guint time = 0;
    org_atheme_audacious_time(proxy, &time, &error);
    g_clear_error(&error);
    return time;
}

/**
 * audacious_remote_jump_to_time:
 * @proxy: DBus proxy for audacious
 * @pos: The time (in milliseconds) to jump to.
 *
 * Tells audacious to seek to a new time position.
 **/
void audacious_remote_jump_to_time(DBusGProxy *proxy, guint pos) {
    org_atheme_audacious_seek (proxy, pos, &error);
    g_clear_error(&error);    
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
    gint vl = 0, vr = 0;

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
    gint balance = 50;
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
    gint b = 50, vl = 0, vr = 0;

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
 * audacious_remote_set_balance:
 * @proxy: DBus proxy for audacious
 * @b: The balance to set.
 *
 * Sets the balance in Audacious.
 **/
void audacious_remote_set_balance(DBusGProxy *proxy, gint b) {
    gint v = 0, vl = 0, vr = 0;

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

/**
 * audacious_remote_get_skin:
 * @proxy: DBus proxy for audacious
 *
 * Queries Audacious about it's skin.
 *
 * Return value: A path to the currently selected skin.
 **/
gchar *audacious_remote_get_skin(DBusGProxy *proxy) {
    gchar *skin = NULL;
    org_atheme_audacious_get_skin (proxy, &skin, &error); // xxx
    g_clear_error(&error);
    return skin;
}

/**
 * audacious_remote_set_skin:
 * @proxy: DBus proxy for audacious
 * @skinfile: Path to a skinfile to use with Audacious.
 *
 * Tells audacious to start using the skinfile provided.
 **/
void audacious_remote_set_skin(DBusGProxy *proxy, gchar *skinfile) {
    org_atheme_audacious_set_skin(proxy, skinfile, &error);
	g_clear_error(&error);
}

/**
 * audacious_remote_get_playlist_file:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's file.
 *
 * Return value: A path to the file in the playlist at %pos position.
 **/
gchar *audacious_remote_get_playlist_file(DBusGProxy *proxy, guint pos) {
    gchar *out = NULL;
    org_atheme_audacious_song_filename(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

/**
 * audacious_remote_get_playlist_title:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's title.
 *
 * Return value: The title for the entry in the playlist at %pos position.
 **/
gchar *audacious_remote_get_playlist_title(DBusGProxy *proxy, guint pos) {
    gchar *out = NULL;
    org_atheme_audacious_song_title(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

/**
 * audacious_remote_get_playlist_time:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's length.
 *
 * Return value: The length of the entry in the playlist at %pos position.
 **/
gint audacious_remote_get_playlist_time(DBusGProxy *proxy, guint pos) {
    gint out = 0;
    org_atheme_audacious_song_frames(proxy, pos, &out, &error);
    g_clear_error(&error);
    return out;
}

/**
 * audacious_remote_get_info:
 * @proxy: DBus proxy for audacious
 * @rate: Pointer to an integer containing the bitrate.
 * @freq: Pointer to an integer containing the frequency.
 * @nch: Pointer to an integer containing the number of channels.
 *
 * Queries Audacious about the current audio format.
 **/
void audacious_remote_get_info(DBusGProxy *proxy, gint *rate, gint *freq,
                               gint *nch) {
    org_atheme_audacious_info(proxy, rate, freq, nch, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_main_win_toggle:
 * @proxy: DBus proxy for audacious
 * @show: Whether or not to show the main window.
 *
 * Toggles the main window's visibility.
 **/
void audacious_remote_main_win_toggle(DBusGProxy *proxy, gboolean show) {
    org_atheme_audacious_show_main_win(proxy, show, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_pl_win_toggle:
 * @proxy: DBus proxy for audacious
 * @show: Whether or not to show the playlist window.
 *
 * Toggles the playlist window's visibility.
 **/
void audacious_remote_pl_win_toggle(DBusGProxy *proxy, gboolean show) {
    org_atheme_audacious_show_playlist(proxy, show, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_eq_win_toggle:
 * @proxy: DBus proxy for audacious
 * @show: Whether or not to show the equalizer window.
 *
 * Toggles the equalizer window's visibility.
 **/
void audacious_remote_eq_win_toggle(DBusGProxy *proxy, gboolean show) {
    org_atheme_audacious_show_equalizer(proxy, show, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_is_main_win:
 * @proxy: DBus proxy for audacious
 *
 * Queries Audacious about the main window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean audacious_remote_is_main_win(DBusGProxy *proxy) {
    gboolean visible = TRUE;
    org_atheme_audacious_main_win_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

/**
 * audacious_remote_is_pl_win:
 * @proxy: DBus proxy for audacious
 *
 * Queries Audacious about the playlist window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean audacious_remote_is_pl_win(DBusGProxy *proxy) {
    gboolean visible = TRUE;
    org_atheme_audacious_playlist_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

/**
 * audacious_remote_is_eq_win:
 * @proxy: DBus proxy for audacious
 *
 * Queries Audacious about the equalizer window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean audacious_remote_is_eq_win(DBusGProxy *proxy) {
    gboolean visible = FALSE;
    org_atheme_audacious_equalizer_visible(proxy, &visible, &error);
    g_clear_error(&error);
    return visible;
}

/**
 * audacious_remote_show_prefs_box:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to show the preferences pane.
 **/
void audacious_remote_show_prefs_box(DBusGProxy *proxy) {
    org_atheme_audacious_show_prefs_box(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_show_about_box:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to show the about box.
 **/
void audacious_remote_show_about_box(DBusGProxy *proxy) {
    org_atheme_audacious_show_about_box(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_toggle_aot:
 * @proxy: DBus proxy for audacious
 * @ontop: Whether or not Audacious should be always-on-top.
 *
 * Tells audacious to toggle the always-on-top feature.
 **/
void audacious_remote_toggle_aot(DBusGProxy *proxy, gboolean ontop) {
    org_atheme_audacious_toggle_aot(proxy, ontop, &error);
	g_clear_error(&error);
}

/**
 * audacious_remote_eject:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to display the open files pane.
 **/
void audacious_remote_eject(DBusGProxy *proxy) {
    org_atheme_audacious_eject(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playlist_prev:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to move backwards in the playlist.
 **/
void audacious_remote_playlist_prev(DBusGProxy *proxy) {
    org_atheme_audacious_reverse(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playlist_next:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to move forward in the playlist.
 **/
void audacious_remote_playlist_next(DBusGProxy *proxy) {
    org_atheme_audacious_advance(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playlist_add_url_string:
 * @proxy: DBus proxy for audacious
 * @string: The URI to add.
 *
 * Tells audacious to add an URI to the playlist.
 **/
void audacious_remote_playlist_add_url_string(DBusGProxy *proxy,
                                              gchar *string) {
    org_atheme_audacious_add_url(proxy, string, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_is_running:
 * @proxy: DBus proxy for audacious
 *
 * Checks to see if an Audacious server is running.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean audacious_remote_is_running(DBusGProxy *proxy) {
    char *string = NULL;
    org_atheme_audacious_version(proxy, &string, &error);
    g_clear_error(&error);
    if(string) {
        g_free(string);
        return TRUE;
    }
    else
        return FALSE;
}

/**
 * audacious_remote_toggle_repeat:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to toggle the repeat feature.
 **/
void audacious_remote_toggle_repeat(DBusGProxy *proxy) {
    org_atheme_audacious_toggle_repeat(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_toggle_shuffle:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to toggle the shuffle feature.
 **/
void audacious_remote_toggle_shuffle(DBusGProxy *proxy) {
    org_atheme_audacious_toggle_shuffle (proxy, &error);
    g_clear_error(&error);    
}

/**
 * audacious_remote_is_repeat:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about whether or not the repeat feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean audacious_remote_is_repeat(DBusGProxy *proxy) {
    gboolean is_repeat;
    org_atheme_audacious_repeat(proxy, &is_repeat, &error);
    g_clear_error(&error);
    return is_repeat;
}

/**
 * audacious_remote_is_shuffle:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about whether or not the shuffle feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean audacious_remote_is_shuffle(DBusGProxy *proxy) {
    gboolean is_shuffle;
    org_atheme_audacious_shuffle(proxy, &is_shuffle, &error);
    g_clear_error(&error);
    return is_shuffle;
}

/**
 * audacious_remote_get_eq:
 * @proxy: DBus proxy for audacious
 * @preamp: Pointer to value for preamp setting.
 * @bands: Pointer to array of band settings.
 *
 * Queries audacious about the equalizer settings.
 **/
void audacious_remote_get_eq(DBusGProxy *proxy, gfloat *preamp,
                             gfloat **bands) {
//XXX
}

/**
 * audacious_remote_get_eq_preamp:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the equalizer preamp's setting.
 *
 * Return value: The equalizer preamp's setting.
 **/
gfloat audacious_remote_get_eq_preamp(DBusGProxy *proxy) {
//XXX
    return 0.0;
}

/**
 * audacious_remote_get_eq_band:
 * @proxy: DBus proxy for audacious
 * @band: Which band to lookup the value for.
 *
 * Queries audacious about an equalizer band's value.
 *
 * Return value: The equalizer band's value.
 **/
gfloat audacious_remote_get_eq_band(DBusGProxy *proxy, gint band) {
//XXX
    return 0.0;
}

/**
 * audacious_remote_set_eq:
 * @proxy: DBus proxy for audacious
 * @preamp: Value for preamp setting.
 * @bands: Array of band settings.
 *
 * Tells audacious to set the equalizer up using the provided values.
 **/
void audacious_remote_set_eq(DBusGProxy *proxy, gfloat preamp,
                             gfloat *bands) {
//XXX
}

/**
 * audacious_remote_set_eq_preamp:
 * @proxy: DBus proxy for audacious
 * @preamp: Value for preamp setting.
 *
 * Tells audacious to set the equalizer's preamp setting.
 **/
void audacious_remote_set_eq_preamp(DBusGProxy *proxy, gfloat preamp) {
//XXX
}

/**
 * audacious_remote_set_eq_band:
 * @proxy: DBus proxy for audacious
 * @band: The band to set the value for.
 * @value: The value to set that band to.
 *
 * Tells audacious to set an equalizer band's setting.
 **/
void audacious_remote_set_eq_band(DBusGProxy *proxy, gint band,
                                  gfloat value) {
//XXX
}

/**
 * audacious_remote_quit:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to quit.
 **/
void audacious_remote_quit(DBusGProxy *proxy) {
    org_atheme_audacious_quit(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_play_pause:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to either play or pause.
 **/
void audacious_remote_play_pause(DBusGProxy *proxy) {
    org_atheme_audacious_play_pause(proxy, &error);
}

/**
 * audacious_remote_playlist_ins_url_string:
 * @proxy: DBus proxy for audacious
 * @string: The URI to add.
 * @pos: The position to add the URI at.
 *
 * Tells audacious to add an URI to the playlist at a specific position.
 **/
void audacious_remote_playlist_ins_url_string(DBusGProxy *proxy,
                                              gchar *string, guint pos) {
    org_atheme_audacious_playlist_ins_url_string (proxy, string, pos, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playqueue_add:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to add to the queue.
 *
 * Tells audacious to add a playlist entry to the playqueue.
 **/
void audacious_remote_playqueue_add(DBusGProxy *proxy, guint pos) {
    org_atheme_audacious_playqueue_add (proxy, pos, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playqueue_remove:
 * @proxy: DBus proxy for audacious
 * @pos: The playlist position to remove from the queue.
 *
 * Tells audacious to remove a playlist entry from the playqueue.
 **/
void audacious_remote_playqueue_remove(DBusGProxy *proxy, guint pos) {
    org_atheme_audacious_playqueue_remove (proxy, pos, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_get_playqueue_length:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about the playqueue's length.
 *
 * Return value: The number of entries in the playqueue.
 **/
gint audacious_remote_get_playqueue_length(DBusGProxy *proxy) {
    gint len = 0;
    // this returns the lenght of the playlist, NOT the length of the playqueue
    org_atheme_audacious_length(proxy, &len, &error);
    g_clear_error(&error);
    return len;
}

/**
 * audacious_remote_toggle_advance:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to toggle the no-playlist-advance feature.
 **/
void audacious_remote_toggle_advance(DBusGProxy *proxy) {
    org_atheme_audacious_toggle_auto_advance(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_is_advance:
 * @proxy: DBus proxy for audacious
 *
 * Queries audacious about whether or not the no-playlist-advance feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean audacious_remote_is_advance(DBusGProxy *proxy) {
    gboolean is_advance = FALSE;
    org_atheme_audacious_auto_advance(proxy, &is_advance, &error);
    g_clear_error(&error);
    return is_advance;
}

/**
 * audacious_remote_activate:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to display the main window and become the selected window.
 **/
void audacious_remote_activate(DBusGProxy *proxy) {
    org_atheme_audacious_activate(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_show_jtf_box:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to show the Jump-to-File pane.
 **/
void audacious_remote_show_jtf_box(DBusGProxy *proxy) {
    org_atheme_audacious_show_jtf_box(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playqueue_clear:
 * @proxy: DBus proxy for audacious
 *
 * Tells audacious to clear the playqueue.
 **/
void audacious_remote_playqueue_clear(DBusGProxy *proxy) {
    org_atheme_audacious_playqueue_clear(proxy, &error);
    g_clear_error(&error);
}

/**
 * audacious_remote_playqueue_is_queued:
 * @proxy: DBus proxy for audacious
 * @pos: Position to check queue for.
 *
 * Queries audacious about whether or not a playlist entry is in the playqueue.
 *
 * Return value: TRUE if yes, FALSE otherwise.
 **/
gboolean audacious_remote_playqueue_is_queued(DBusGProxy *proxy, guint pos) {
    gboolean is_queued;
    org_atheme_audacious_playqueue_is_queued (proxy, pos, &is_queued, &error);
    g_clear_error(&error);
    return is_queued;
}

/**
 * audacious_remote_get_playqueue_queue_position:
 * @proxy: DBus proxy for audacious
 * @pos: Position to check queue for.
 *
 * Queries audacious about what the playqueue position is for a playlist entry.
 *
 * Return value: the playqueue position for a playlist entry
 **/
gint audacious_remote_get_playqueue_queue_position(DBusGProxy *proxy, guint pos) {
    guint qpos = 0;
    org_atheme_audacious_queue_get_queue_pos (proxy, pos, &qpos, &error);
    g_clear_error(&error);
    return qpos;
}

/**
 * audacious_remote_get_playqueue_list_position:
 * @proxy: DBus proxy for audacious
 * @pos: Position to check queue for.
 *
 * Queries audacious about what the playlist position is for a playqueue entry.
 *
 * Return value: the playlist position for a playqueue entry
 **/
gint audacious_remote_get_playqueue_list_position(DBusGProxy *proxy, guint qpos) {
    guint pos = 0;
    org_atheme_audacious_queue_get_list_pos (proxy, qpos, &pos, &error);
    g_clear_error(&error);
    return pos;
}

/**
 * audacious_remote_playlist_enqueue_to_temp:
 * @proxy: DBus proxy for audacious
 * @string: The URI to enqueue to a temporary playlist.
 *
 * Tells audacious to add an URI to a temporary playlist.
 **/
void audacious_remote_playlist_enqueue_to_temp(DBusGProxy *proxy,
                                               gchar *string) {
    org_atheme_audacious_playlist_enqueue_to_temp(proxy, string, &error);
	g_clear_error(&error);
}

/**
 * audacious_get_tuple_field_data:
 * @proxy: DBus proxy for audacious
 * @field: The name of the tuple field to retrieve.
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's tuple information.
 *
 * Return value: The requested field's data for the entry in the playlist at %pos position.
 **/
gchar *audacious_get_tuple_field_data(DBusGProxy *proxy, gchar *field,
                                      guint pos) {
//XXX
	g_clear_error(&error);
    return NULL;
}

