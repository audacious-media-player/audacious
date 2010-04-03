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
#include "probe.h"
#include "util.h"

#include "playback.h"

static void set_params (InputPlayback * playback, const gchar * title, gint
 length, gint bitrate, gint samplerate, gint channels);
static void set_title (InputPlayback * playback, const gchar * title);
static void set_tuple (InputPlayback * playback, Tuple * tuple);
static gboolean playback_segmented_end(gpointer data);

static gboolean playback_play_file (gint playlist, gint entry);

static gboolean pause_when_ready;
static gint seek_when_ready;
static gint ready_source;
static gint failed_entries;

static gboolean ready_cb (void * data)
{
    InputPlayback * playback = data;

    g_return_val_if_fail (playback != NULL, FALSE);

    g_mutex_lock (playback->pb_ready_mutex);
    ready_source = 0;
    g_mutex_unlock (playback->pb_ready_mutex);

    if (pause_when_ready)
        playback_pause ();

    if (seek_when_ready > 0)
        playback_seek (seek_when_ready);

    hook_call ("title change", NULL);
    return FALSE;
}

static gboolean playback_is_ready (InputPlayback * playback)
{
    gboolean ready;

    g_return_val_if_fail (playback != NULL, FALSE);

    g_mutex_lock (playback->pb_ready_mutex);
    ready = (playback->pb_ready_val && ! ready_source);
    g_mutex_unlock (playback->pb_ready_mutex);
    return ready;
}

static gint
playback_set_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    playback->pb_ready_val = 1;
    ready_source = g_timeout_add (0, ready_cb, playback);
    g_cond_signal(playback->pb_ready_cond);
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

static void update_cb (void * hook_data, void * user_data)
{
    InputPlayback * playback;
    gint playlist, entry;
    const gchar * title;

    playback = ip_data.current_input_playback;
    g_return_if_fail (playback != NULL);

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA ||
     ! playback_is_ready (playback))
        return;

    playlist = playlist_get_playing ();
    entry = playlist_get_position (playlist);

    if ((title = playlist_entry_get_title (playlist, entry)) == NULL)
        title = playlist_entry_get_filename (playlist, entry);

    g_free (playback->title);
    playback->title = g_strdup (title);
    playback->length = playlist_entry_get_length (playlist, entry);

    hook_call ("title change", NULL);
}

static gint
playback_get_time_real(void)
{
    InputPlayback * playback;
    gint time = -1;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, 0);

    if (! playback_is_ready (playback))
        return seek_when_ready;

    if (! playback->playing || playback->eof || playback->error)
        return 0;

    if (playback->plugin->get_time != NULL)
        time = playback->plugin->get_time (playback);

    if (time == -1)
        time = get_output_time ();

    return time;
}

gint
playback_get_time(void)
{
    InputPlayback * playback;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, 0);

    return playback_get_time_real() - playback->start;
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
        playlist_next_song (playlist, TRUE);
        entry = playlist_get_position (playlist);

        if (entry == -1)
            return;
    }

    if (playback_get_playing())
        playback_stop();

#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif

    failed_entries = 0;
    playback_play_file (playlist, entry);
}

void playback_pause (void)
{
    InputPlayback * playback;
    gboolean paused;

    playback = ip_data.current_input_playback;
    g_return_if_fail (playback != NULL);

    if (playback_is_ready (playback))
    {
        paused = ! ip_data.paused;

        if (playback->end_timeout)
            g_source_remove(playback->end_timeout);

        g_return_if_fail (playback->plugin->pause != NULL);
        playback->plugin->pause (playback, paused);

        ip_data.paused = paused;
    }
    else
    {
        paused = ! pause_when_ready;
        pause_when_ready = paused;
    }

    if (paused)
        hook_call("playback pause", NULL);
    else
    {
        hook_call("playback unpause", NULL);

        if (playback->end > 0)
            playback->end_timeout = g_timeout_add(playback_get_time_real() - playback->start, playback_segmented_end, playback);
    }
}

static void playback_finalize (InputPlayback * playback)
{
    hook_dissociate ("playlist update", update_cb);

    g_mutex_lock (playback->pb_ready_mutex);

    while (! playback->pb_ready_val)
        g_cond_wait (playback->pb_ready_cond, playback->pb_ready_mutex);

    if (ready_source)
    {
        g_source_remove (ready_source);
        ready_source = 0;
    }

    g_mutex_unlock (playback->pb_ready_mutex);

    if (playback->playing)
        playback->plugin->stop (playback);

    /* some plugins do this themselves */
    if (playback->thread != NULL)
        g_thread_join (playback->thread);

    playback_free (playback);
    ip_data.current_input_playback = NULL;

    if (playback->end_timeout)
        g_source_remove(playback->end_timeout);
}

void playback_stop (void)
{
    g_return_if_fail (ip_data.current_input_playback != NULL);

    ip_data.stop = TRUE;

    playback_finalize (ip_data.current_input_playback);

    ip_data.playing = FALSE;
    ip_data.stop = FALSE;

    output_drain ();
    hook_call ("playback stop", NULL);
}

static void
run_no_output_plugin_dialog(void)
{
    const gchar *markup =
        N_("<b><big>No output plugin selected.</big></b>\n"
           "You have not selected an output plugin.");

    interface_show_error_message(markup);
}

static gboolean playback_ended (void * user_data)
{
    gint playlist = playlist_get_playing ();
    gboolean play;

    g_return_val_if_fail (ip_data.current_input_playback != NULL, FALSE);

    if (ip_data.current_input_playback->error)
        failed_entries ++;
    else
        failed_entries = 0;

    playback_finalize (ip_data.current_input_playback);

    while (1)
    {
        if (cfg.no_playlist_advance)
            play = cfg.repeat && ! failed_entries;
        else
        {
            if (! (play = playlist_next_song (playlist, cfg.repeat)))
                playlist_set_position (playlist, -1);

            if (failed_entries >= 100 || failed_entries >= playlist_entry_count
             (playlist) * 2)
                play = FALSE;
        }

        if (cfg.stopaftersong)
            play = FALSE;

        if (! play)
        {
            output_drain ();
            hook_call ("playback stop", NULL);
            break;
        }

        if (playback_play_file (playlist, playlist_get_position (playlist)))
            break;

        failed_entries ++;
    }

    return FALSE;
}

static gboolean
playback_segmented_end(gpointer data)
{
    if (playlist_next_song (playlist_get_playing (), cfg.repeat))
        playback_initiate ();

    return FALSE;
}

static gboolean
playback_segmented_start(gpointer data)
{
    InputPlayback *playback = (InputPlayback *) data;

    if (playback->plugin->mseek != NULL)
        playback->plugin->mseek (playback, playback->start);
    else if (playback->plugin->seek != NULL)
        playback->plugin->seek (playback, playback->start / 1000);

    if (playback->end > 0)
        playback->end_timeout = g_timeout_add(playback->end - playback->start, playback_segmented_end, playback);

    return FALSE;
}

static gpointer
playback_monitor_thread(gpointer data)
{
    InputPlayback *playback = (InputPlayback *) data;

    if (playback->segmented)
        g_idle_add(playback_segmented_start, playback);

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->play_file(playback);

    g_mutex_lock (playback->pb_ready_mutex);
    playback->pb_ready_val = TRUE;

    if (ready_source != 0)
    {
        g_source_remove (ready_source);
        ready_source = 0;
    }

    g_cond_signal (playback->pb_ready_cond);
    g_mutex_unlock (playback->pb_ready_mutex);

    if (! ip_data.stop)
        g_timeout_add (0, playback_ended, NULL);

    return NULL;
}

/* compatibility */
static void playback_set_replaygain_info (InputPlayback * playback,
 ReplayGainInfo * info)
{
    playback->output->set_replaygain_info (info);
}

/* compatibility */
static void playback_pass_audio (InputPlayback * playback, AFormat format, gint
 channels, gint size, void * data, gint * going)
{
    playback->output->write_audio (data, size);
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

    playback->output = & output_api;

    /* init vtable functors */
    playback->set_pb_ready = playback_set_pb_ready;
    playback->set_pb_change = playback_set_pb_change;
    playback->set_params = set_params;
    playback->set_title = set_title;
    playback->set_tuple = set_tuple;

    /* compatibility */
    playback->set_replaygain_info = playback_set_replaygain_info;
    playback->pass_audio = playback_pass_audio;

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

    pause_when_ready = FALSE;
    seek_when_ready = 0;
    ready_source = 0;

    playback->thread = g_thread_create(playback_monitor_thread, playback, TRUE, NULL);
}

static gboolean playback_play_file (gint playlist, gint entry)
{
    const gchar * filename = playlist_entry_get_filename (playlist, entry);
    const gchar * title = playlist_entry_get_title (playlist, entry);
    InputPlugin * decoder = playlist_entry_get_decoder (playlist, entry);
    InputPlayback *playback;

    if (current_output_plugin == NULL)
    {
        run_no_output_plugin_dialog ();
        return FALSE;
    }

    if (decoder == NULL)
        decoder = file_probe (filename, FALSE);

    if (decoder == NULL)
    {
        g_warning("Cannot play %s: no decoder found.\n", filename);
        return FALSE;
    }

    ip_data.playing = TRUE;
    ip_data.paused = FALSE;
    ip_data.stop = FALSE;

    playback = playback_new();

    playback->plugin = decoder;
    playback->filename = g_strdup (filename);
    playback->title = g_strdup ((title != NULL) ? title : filename);
    playback->length = playlist_entry_get_length (playlist, entry);
    playback->segmented = playlist_entry_is_segmented (playlist, entry);
    playback->start = playlist_entry_get_start_time (playlist, entry);
    playback->end = playlist_entry_get_end_time (playlist, entry);

    set_current_input_playback(playback);

    playback_run(playback);

#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif

    hook_associate ("playlist update", update_cb, NULL);
    hook_call ("playback begin", NULL);
    return TRUE;
}

gboolean playback_get_playing (void)
{
    return (ip_data.current_input_playback != NULL);
}

gboolean playback_get_paused (void)
{
    InputPlayback * playback;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, FALSE);

    if (playback_is_ready (playback))
        return ip_data.paused;
    else
        return pause_when_ready;
}

void playback_seek (gint time)
{
    InputPlayback * playback;

    playback = ip_data.current_input_playback;
    g_return_if_fail (playback != NULL);

    if (playback->length <= 0)
        return;

    time = CLAMP (time, 0, playback->length);

    if (playback_is_ready (playback))
    {
        if (playback->start > 0)
            time += playback->start;

        if (playback->plugin->mseek != NULL)
            playback->plugin->mseek (playback, time);
        else if (playback->plugin->seek != NULL)
            playback->plugin->seek (playback, time / 1000);

        if (playback->end > 0)
        {
            if (playback->end_timeout)
                g_source_remove(playback->end_timeout);

            playback->end_timeout = g_timeout_add(playback->end - time, playback_segmented_end, playback);
        }
    }
    else
        seek_when_ready = time;

    hook_call ("playback seek", playback);
}

static void set_params (InputPlayback * playback, const gchar * title, gint
 length, gint bitrate, gint samplerate, gint channels)
{
    g_return_if_fail (playback != NULL);

    /* deprecated usage */
    if (title != NULL || length != 0)
        g_warning ("%s needs to be updated to use InputPlayback::set_tuple\n",
         playback->plugin->description);

    playback->rate = bitrate;
    playback->freq = samplerate;
    playback->nch = channels;

    event_queue ("info change", NULL);
}

/* deprecated */
static void set_title (InputPlayback * playback, const gchar * title)
{
    g_warning ("%s needs to be updated to use InputPlayback::set_tuple\n",
     playback->plugin->description);
}

static gboolean set_tuple_cb (void * tuple)
{
    gint playlist = playlist_get_playing ();

    playlist_entry_set_tuple (playlist, playlist_get_position (playlist), tuple);
    return FALSE;
}

static void set_tuple (InputPlayback * playback, Tuple * tuple)
{
    /* playlist_entry_set_tuple must execute in main thread */
    g_timeout_add (0, set_tuple_cb, tuple);
}

gchar * playback_get_title (void)
{
    InputPlayback * playback;
    gchar * suffix, * title;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, NULL);

    if (! playback_is_ready (playback))
        return g_strdup (_("Buffering ..."));

    suffix = (playback->length > 0) ? g_strdup_printf (" (%d:%02d)",
     playback->length / 60000, playback->length / 1000 % 60) : NULL;

    if (cfg.show_numbers_in_pl)
        title = g_strdup_printf ("%d. %s%s", 1 + playlist_get_position
         (playlist_get_playing ()), playback->title, (suffix != NULL) ? suffix :
         "");
    else
        title = g_strdup_printf ("%s%s", playback->title, (suffix !=
         NULL) ? suffix : "");

    g_free (suffix);
    return title;
}

gint playback_get_length (void)
{
    g_return_val_if_fail (ip_data.current_input_playback != NULL, 0);

    return ip_data.current_input_playback->length;
}

void playback_get_info (gint * bitrate, gint * samplerate, gint * channels)
{
    InputPlayback * playback;

    playback = ip_data.current_input_playback;
    g_return_if_fail (playback != NULL);

    * bitrate = playback->rate;
    * samplerate = playback->freq;
    * channels = playback->nch;
}
