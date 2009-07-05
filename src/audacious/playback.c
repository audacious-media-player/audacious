/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "configdb.h"
#include "eventqueue.h"
#include "hook.h"
#include "input.h"
#include "output.h"
#include "playlist.h"
#include "pluginenum.h"
#include "util.h"

#include "playback.h"
#include "playback_evlisteners.h"

static PlaybackInfo playback_info = { 0, 0, 0 };

static gint
playback_set_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    playback->pb_ready_val = 1;
    g_cond_signal(playback->pb_ready_cond);
    g_mutex_unlock(playback->pb_ready_mutex);
    return 0;
}

static gint
playback_wait_for_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    while (playback->pb_ready_val != 1)
        g_cond_wait(playback->pb_ready_cond, playback->pb_ready_mutex);
    g_mutex_unlock(playback->pb_ready_mutex);
    return 0;
}

static void
playback_set_pb_change(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_change_mutex);
    g_cond_signal(playback->pb_change_cond);
    g_mutex_unlock(playback->pb_change_mutex);
}

static void
playback_set_pb_params(InputPlayback *playback, gchar *title,
    gint length, gint rate, gint freq, gint nch)
{
    playback->title = g_strdup(title);
    playback->length = length;
    playback->rate = rate;
    playback->freq = freq;
    playback->nch = nch;

    /* XXX: this can be removed/merged here someday */
    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->set_info(title, length, rate, freq, nch);
}

static void
playback_set_pb_title(InputPlayback *playback, gchar *title)
{
    playback->title = g_strdup(title);

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->set_info_text(title);
}

void
playback_error(void)
{
    event_queue("playback audio error", NULL);
}

gint
playback_get_time(void)
{
    InputPlayback * playback;

    if (! ip_data.playing)
        return 0;

    playback = ip_data.current_input_playback;

    if (! playback || ! playback->playing || playback->eof || playback->error)
        return 0;

    if (playback->plugin->get_time)
        return playback->plugin->get_time (playback);

    /* Note: This assumes that output_time will return sanely (zero) even before
     audio is opened. Not ideal, but what else can we do? -jlindgren */
    return playback->output->output_time ();
}

gint
playback_get_length(void)
{
    InputPlayback *playback;
    g_return_val_if_fail(playback_get_playing(), -1);
    playback = get_current_input_playback();

    if (playback->length)
        return playback->length;

    if (playback && playback->plugin->get_song_tuple) {
        plugin_set_current((Plugin *)(playback->plugin));
        Tuple *tuple = playback->plugin->get_song_tuple(playback->filename);
        if (tuple_get_value_type(tuple, FIELD_LENGTH, NULL) == TUPLE_INT)
            return tuple_get_int(tuple, FIELD_LENGTH, NULL);
    }

    return -1;
}

void
playback_initiate(void)
{
    PlaylistEntry *entry = NULL;
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(playlist_get_length(playlist) != 0);

    /* initialize playback event listeners if not done already */
    playback_evlistener_init();

    if (playback_get_playing())
        playback_stop();

    entry = playlist_get_entry_to_play(playlist);
    g_return_if_fail(entry != NULL);

#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif

    if (!playback_play_file(entry))
        return;

    hook_call("playback begin", entry);
}

void
playback_pause(void)
{
    InputPlayback *playback;

    if (!playback_get_playing())
        return;

    if ((playback = get_current_input_playback()) == NULL)
        return;

    ip_data.paused = !ip_data.paused;

    if (playback->plugin->pause)
    {
        plugin_set_current((Plugin *)(playback->plugin));
        playback->plugin->pause(playback, ip_data.paused);
    }

    if (ip_data.paused)
        hook_call("playback pause", NULL);
    else
        hook_call("playback unpause", NULL);

#ifdef USE_DBUS
    if (ip_data.paused)
        mpris_emit_status_change(mpris, MPRIS_STATUS_PAUSE);
    else
        mpris_emit_status_change(mpris, MPRIS_STATUS_PLAY);
#endif
}

void
playback_stop(void)
{
    InputPlayback *playback;

    if ((playback = get_current_input_playback()) == NULL)
        return;

    if (ip_data.playing)
    {
        /* wait for plugin to signal it has reached
           the 'playback safe state' before stopping */
        playback_wait_for_pb_ready(playback);

        if (playback_get_paused() == TRUE)
        {
            if (get_written_time() > 0)
              output_flush(get_written_time()); /* to avoid noise */
            playback_pause();
        }

        ip_data.playing = FALSE;
        ip_data.stop = TRUE; /* So that audio is really closed. */

        /* TODO: i'm unsure if this will work. we might have to
           signal the change in stop() (e.g. in the plugins
           directly.) --nenolod */
        playback->set_pb_change(playback);

        if (playback->plugin->stop)
        {
            plugin_set_current((Plugin *)(playback->plugin));
            playback->plugin->stop(playback);
        }

        if (playback->thread != NULL)
        {
            g_thread_join(playback->thread);
            playback->thread = NULL;
        }

        ip_data.stop = FALSE;

        playback_free(playback);
        set_current_input_playback(NULL);
#ifdef USE_DBUS
        mpris_emit_status_change(mpris, MPRIS_STATUS_STOP);
#endif
    }

    hook_call("playback stop", NULL);
}

static void
run_no_output_plugin_dialog(void)
{
    const gchar *markup =
        N_("<b><big>No output plugin selected.</big></b>\n"
           "You have not selected an output plugin.");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(NULL,
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gpointer
playback_monitor_thread(gpointer data)
{
    gboolean restart;
    InputPlayback *playback = (InputPlayback *) data;

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->play_file(playback);

    /* if play_file has not reached the 'safe state' before returning (an error
       occurred), set the playback ready value to 1 now, to allow for proper stop */
    playback_set_pb_ready(playback);

    restart = ip_data.playing;

    playback->thread = NULL;
    ip_data.playing = FALSE;

    if (playback->error)
        playback_error ();
    else if (restart)
        event_queue ("playback eof", NULL);

    return NULL;
}

InputPlayback *
playback_new(void)
{
    InputPlayback *playback = (InputPlayback *) g_slice_new0(InputPlayback);

    playback->pb_ready_mutex = g_mutex_new();
    playback->pb_ready_cond = g_cond_new();
    playback->pb_ready_val = 0;

    playback->pb_change_mutex = g_mutex_new();
    playback->pb_change_cond = g_cond_new();

    /* init vtable functors */
    playback->set_pb_ready = playback_set_pb_ready;
    playback->set_pb_change = playback_set_pb_change;
    playback->set_params = playback_set_pb_params;
    playback->set_title = playback_set_pb_title;
    playback->pass_audio = output_pass_audio;
    playback->set_replaygain_info = output_set_replaygain_info;

    return playback;
}

/**
 * Destroys InputPlayback.
 *
 * Playback comes from playback_new() function but there can be also
 * other sources for allocated playback data (like filename and title)
 * and this tries to deallocate all that data.
 */
void playback_free(InputPlayback *playback)
{
    g_free(playback->filename);
    g_free(playback->title);

    g_mutex_free(playback->pb_ready_mutex);
    g_cond_free(playback->pb_ready_cond);

    g_mutex_free(playback->pb_change_mutex);
    g_cond_free(playback->pb_change_cond);

    g_slice_free(InputPlayback, playback);
}

void
playback_run(InputPlayback *playback)
{
    playback->playing = 0;
    playback->eof = 0;
    playback->error = 0;

    g_mutex_lock(playback->pb_ready_mutex);

    playback->thread = g_thread_create(playback_monitor_thread, playback, TRUE, NULL);
    g_cond_wait(playback->pb_ready_cond, playback->pb_ready_mutex);

    g_mutex_unlock(playback->pb_ready_mutex);
}

gboolean
playback_play_file(PlaylistEntry *entry)
{
    InputPlayback *playback;

    g_return_val_if_fail(entry != NULL, FALSE);

    if (!get_current_output_plugin()) {
        run_no_output_plugin_dialog();
        playback_stop();
        return FALSE;
    }

    if (!entry->decoder)
    {
        ProbeResult *pr = input_check_file(entry->filename, FALSE);

        if (pr != NULL)
        {
            entry->decoder = pr->ip;
            entry->tuple = pr->tuple;

            g_free(pr);
        }
        else
        {
            playback_stop();
            return FALSE;
        }
    }

    if (!entry->decoder || !entry->decoder->enabled)
    {
        set_current_input_playback(NULL);

        return FALSE;
    }

    ip_data.playing = TRUE;

    playback = playback_new();

    playback->plugin = entry->decoder;
    playback->filename = g_strdup(entry->filename);
    playback->output = &psuedo_output_plugin;

    set_current_input_playback(playback);

    playback_run(playback);

#ifdef USE_DBUS
    mpris_emit_status_change(mpris, MPRIS_STATUS_PLAY);
#endif

    hook_call("playback play file", NULL);

    return TRUE;
}

gboolean
playback_get_playing(void)
{
    return ip_data.playing;
}

gboolean
playback_get_paused(void)
{
    return ip_data.paused;
}

void
playback_seek(gint time)
{
    InputPlayback *playback = get_current_input_playback();

    g_return_if_fail(ip_data.playing);
    g_return_if_fail(playback != NULL);

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->seek(playback, time);
    playback->set_pb_change(playback);

    hook_call ("playback seek", playback);
}

void
playback_seek_relative(gint offset)
{
    gint time = CLAMP(playback_get_time() / 1000 + offset,
                      0, playlist_get_current_length(playlist_get_active()) / 1000 - 1);
    playback_seek(time);
}

void
playback_get_sample_params(gint * bitrate,
                           gint * frequency,
                           gint * n_channels)
{
    if (bitrate)
        *bitrate = playback_info.bitrate;

    if (frequency)
        *frequency = playback_info.frequency;

    if (n_channels)
        *n_channels = playback_info.n_channels;
}

void
playback_set_sample_params(gint bitrate,
                           gint frequency,
                           gint n_channels)
{
    if (bitrate >= 0)
        playback_info.bitrate = bitrate;

    if (frequency >= 0)
        playback_info.frequency = frequency;

    if (n_channels >= 0)
        playback_info.n_channels = n_channels;
}
