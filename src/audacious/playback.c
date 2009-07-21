/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
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

#include "audstrings.h"
#include "configdb.h"
#include "eventqueue.h"
#include "hook.h"
#include "input.h"
#include "output.h"
#include "playlist-new.h"
#include "pluginenum.h"
#include "util.h"

#include "playback.h"

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
    gint playlist, entry;

    playlist = playlist_get_playing ();

    if (playlist == -1)
    {
        playlist = playlist_get_active ();
        playlist_set_playing (playlist);
    }

    entry = playlist_get_position (playlist);

    if (entry == -1)
    {
        playlist_next_song (playlist, FALSE);
        entry = playlist_get_position (playlist);

        if (entry == -1)
            return;
    }

    if (playback_get_playing())
        playback_stop();

#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif

    if (! playback_play_file (playlist_entry_get_filename (playlist, entry),
     playlist_entry_get_decoder (playlist, entry)))
        return;

    hook_call ("playback begin", NULL);
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

static gboolean on_to_the_next (void * user_data)
{
    if (cfg.no_playlist_advance)
    {
        if (cfg.repeat)
            playback_initiate ();
        else
            hook_call ("playback stop", NULL);
    }
    else if (playlist_next_song (playlist_get_playing (), cfg.repeat) &&
     ! cfg.stopaftersong)
        playback_initiate ();
    else
        hook_call ("playback stop", NULL);

    return FALSE;
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
        g_timeout_add (0, on_to_the_next, NULL);

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

gboolean playback_play_file (const gchar * filename, InputPlugin * decoder)
{
    InputPlayback *playback;

    if (!get_current_output_plugin()) {
        run_no_output_plugin_dialog();
        playback_stop();
        return FALSE;
    }

    if (decoder == NULL)
    {
        ProbeResult * pr = input_check_file (filename);

        if (pr != NULL)
        {
            decoder = pr->ip;
            g_free(pr);
        }
        else
        {
            playback_stop();
            return FALSE;
        }
    }

    if (decoder == NULL || ! decoder->enabled)
    {
        set_current_input_playback(NULL);

        return FALSE;
    }

    ip_data.playing = TRUE;

    playback = playback_new();

    playback->plugin = decoder;
    playback->filename = g_strdup (filename);
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
    playback->plugin->seek (playback, CLAMP (time, 0, playback->length / 1000));
    playback->set_pb_change(playback);

    hook_call ("playback seek", playback);
}

void
playback_seek_relative(gint offset)
{
    playback_seek (playback_get_time () / 1000 + offset);
}

void playback_set_info (gchar * title, gint length, gint bitrate, gint
 samplerate, gint channels)
{
    InputPlayback * playback = get_current_input_playback ();

    if (playback == NULL)
        return;

    g_free (playback->title);
    playback->title = convert_title_text (g_strdup (title));
    playback->length = length;
    playback->rate = bitrate;
    playback->freq = samplerate;
    playback->nch = channels;

    event_queue ("title change", NULL);
    event_queue ("info change", NULL);
}

void playback_set_title (gchar * title)
{
    InputPlayback * playback = get_current_input_playback ();

    if (playback == NULL)
        return;

    g_free (playback->title);
    playback->title = convert_title_text (g_strdup (title));

    event_queue ("title change", NULL);
}

void playback_get_info (gint * bitrate, gint * samplerate, gint * channels)
{
    InputPlayback * playback = get_current_input_playback ();

    if (playback == NULL)
    {
        * bitrate = 0;
        * samplerate = 0;
        * channels = 0;
        return;
    }

    * bitrate = playback->rate;
    * samplerate = playback->freq;
    * channels = playback->nch;
}

gchar * playback_get_title (void)
{
    InputPlayback * playback = get_current_input_playback ();

    if (playback == NULL)
        return NULL;

    if (cfg.show_numbers_in_pl)
        return g_strdup_printf ("%d. %s (%d:%02d)", playlist_get_position
         (playlist_get_playing ()), playback->title, playback->length / 60000,
         playback->length / 1000 % 60);
    else
        return g_strdup_printf ("%s (%d:%02d)", playback->title,
         playback->length / 60000, playback->length / 1000 % 60);
}
