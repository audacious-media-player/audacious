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
static gint playback_entry;

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
    gint old_entry;

    playback = ip_data.current_input_playback;
    g_return_if_fail (playback != NULL);

    old_entry = playback_entry;
    playback_entry = playlist_get_position (playlist_get_playing ());

    if (cfg.show_numbers_in_pl && playback_entry != old_entry)
        hook_call ("title change", NULL);
}

void
playback_error(void)
{
    event_queue("playback audio error", NULL);
}

static gint
playback_get_time_real(void)
{
    InputPlayback * playback;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, 0);

    if (! playback_is_ready (playback))
        return seek_when_ready;

    if (! playback->playing || playback->eof || playback->error)
        return 0;

    if (playback->plugin->get_time != NULL)
        return playback->plugin->get_time (playback);

    /** @attention This assumes that output_time will return sanely (zero)
     * even before audio is opened. Not ideal, but what else can we do? -jlindgren */
    return playback->output->output_time ();
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

        if (playback->end)
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
    gboolean error, play;

    g_return_val_if_fail (ip_data.current_input_playback != NULL, FALSE);

    error = ip_data.current_input_playback->error;
    playback_finalize (ip_data.current_input_playback);

    if (error)
    {
        playback_error ();
        play = FALSE;
    }
    else
    {
        g_print("no error\n");

        if (cfg.no_playlist_advance)
            play = cfg.repeat;
        else
        {
            g_print("advance\n");

            gint playlist = playlist_get_playing ();

            play = playlist_next_song (playlist, FALSE);

            if (! play)
                play = playlist_next_song (playlist, TRUE) && cfg.repeat;
        }

        if (cfg.stopaftersong) {
            g_print("stop after song\n");
            play = FALSE;
        }
    }

    if (play)
        playback_initiate ();
    else
        hook_call ("playback stop", NULL);

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

    playback->plugin->mseek(playback, playback->start);

    if (playback->end != 0 && playback->end != -1)
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
    playback->set_params = set_params;
    playback->set_title = set_title;
    playback->pass_audio = output_pass_audio;
    playback->set_replaygain_info = output_set_replaygain_info;
    playback->set_tuple = set_tuple;

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

            if (pr->tuple != NULL)
                mowgli_object_unref (pr->tuple);

            g_free (pr);
        }
    }

    if (decoder == NULL || ! decoder->enabled)
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
    playback->output = &psuedo_output_plugin;
    playback->segmented = playlist_entry_is_segmented (playlist, entry);
    playback->start = playlist_entry_get_start_time (playlist, entry);
    playback->end = playlist_entry_get_end_time (playlist, entry);

    set_current_input_playback(playback);
    playback_entry = entry;

    playback_run(playback);

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
        if (playback->plugin->mseek != NULL)
            playback->plugin->mseek (playback, playback->start + time);
        else if (playback->plugin->seek != NULL)
            playback->plugin->seek (playback, (playback->start + time) / 1000);

        if (playback->end != 0 || playback->end != -1)
        {
            g_source_remove(playback->end_timeout);
            playback->end_timeout = g_timeout_add(playback->end - time, playback_segmented_end, playback);
        }
    }
    else
        seek_when_ready = time;

    hook_call ("playback seek", playback);
}

static void set_title_and_length (InputPlayback * playback, const gchar * title,
 gint length)
{
    g_return_if_fail (playback != NULL);
    g_warning ("Plugin %s failed to use InputPlayback::set_tuple "
     "for a length/title adjustment.", playback->plugin->filename);

    if (title != NULL)
    {
        g_free (playback->title);
        playback->title = g_strdup (title);
        event_queue ("title change", NULL);
    }

    if (length != 0)
        playback->length = length;
}

static void set_params (InputPlayback * playback, const gchar * title, gint
 length, gint bitrate, gint samplerate, gint channels)
{
    g_return_if_fail (playback != NULL);

    /* deprecated usage */
    if (title != NULL || length != 0)
        set_title_and_length (playback, title, length);

    playback->rate = bitrate;
    playback->freq = samplerate;
    playback->nch = channels;

    event_queue ("info change", NULL);
}

static void set_title (InputPlayback * playback, const gchar * title)
{
    set_title_and_length (playback, title, 0);
}

static gboolean set_tuple_cb (void * tuple)
{
    InputPlayback * playback;
    gint playlist, entry;
    const gchar * title;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, FALSE);

    playlist = playlist_get_playing ();
    entry = playlist_get_position (playlist);
    playlist_entry_set_tuple (playlist, entry, tuple);

    title = playlist_entry_get_title (playlist, entry);

    if (title == NULL)
        title = playlist_entry_get_filename (playlist, entry);

    g_free (playback->title);
    playback->title = g_strdup (title);
    playback->length = playlist_entry_get_length (playlist, entry);

    hook_call ("title change", NULL);
    return FALSE;
}

static void set_tuple (InputPlayback * playback, Tuple * tuple)
{
    /* playlist_entry_set_tuple must execute in main thread */
    g_timeout_add (0, set_tuple_cb, tuple);
}

void ip_set_info (const gchar * title, gint length, gint bitrate, gint
 samplerate, gint channels)
{
    g_warning ("InputPlugin::set_info is deprecated. Use "
     "InputPlayback::set_tuple and/or InputPlayback::set_params instead.");
    set_params (ip_data.current_input_playback, title, length, bitrate,
     samplerate, channels);
}

void ip_set_info_text (const gchar * title)
{
    g_warning ("InputPlugin::set_info_text is deprecated. Use "
     "InputPlayback::set_tuple instead.\n");
    set_title (ip_data.current_input_playback, title);
}

gchar * playback_get_title (void)
{
    InputPlayback * playback;
    gchar * suffix, * title;

    playback = ip_data.current_input_playback;
    g_return_val_if_fail (playback != NULL, NULL);

    suffix = (playback->length > 0) ? g_strdup_printf (" (%d:%02d)",
     playback->length / 60000, playback->length / 1000 % 60) : NULL;

    if (cfg.show_numbers_in_pl)
        title = g_strdup_printf ("%d. %s%s", 1 + playback_entry,
         playback->title, (suffix != NULL) ? suffix : "");
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
