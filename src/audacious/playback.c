/*
 * playback.c
 * Copyright 2005-2011 Audacious Development Team
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <glib.h>
#include <pthread.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/eventqueue.h>
#include <libaudcore/hook.h>

#include "audconfig.h"
#include "config.h"
#include "i18n.h"
#include "interface.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"

static gboolean playback_start (gint playlist, gint entry, gint seek_time,
 gboolean pause);

static InputPlayback playback_api;

static gboolean playing = FALSE;
static gboolean playback_error;
static gint failed_entries;

static gint current_entry;
static const gchar * current_filename;
static InputPlugin * current_decoder;
static void * current_data;
static gint current_bitrate, current_samplerate, current_channels;
static gchar * current_title;
static gint current_length;

static ReplayGainInfo gain_from_playlist;

static gint time_offset, start_time, stop_time;
static gboolean paused;

static pthread_t playback_thread_handle;
static gint end_source = 0;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;
static gboolean ready_flag;
static gint ready_source = 0;

static gint set_tuple_source = 0;
static Tuple * tuple_to_be_set = NULL;

static void cancel_set_tuple (void)
{
    if (set_tuple_source != 0)
    {
        g_source_remove (set_tuple_source);
        set_tuple_source = 0;
    }

    if (tuple_to_be_set != NULL)
    {
        tuple_free (tuple_to_be_set);
        tuple_to_be_set = NULL;
    }
}

/* clears gain info if tuple == NULL */
static void read_gain_from_tuple (const Tuple * tuple)
{
    gint album_gain, album_peak, track_gain, track_peak, gain_unit, peak_unit;

    memset (& gain_from_playlist, 0, sizeof gain_from_playlist);

    if (tuple == NULL)
        return;

    album_gain = tuple_get_int (tuple, FIELD_GAIN_ALBUM_GAIN, NULL);
    album_peak = tuple_get_int (tuple, FIELD_GAIN_ALBUM_PEAK, NULL);
    track_gain = tuple_get_int (tuple, FIELD_GAIN_TRACK_GAIN, NULL);
    track_peak = tuple_get_int (tuple, FIELD_GAIN_TRACK_PEAK, NULL);
    gain_unit = tuple_get_int (tuple, FIELD_GAIN_GAIN_UNIT, NULL);
    peak_unit = tuple_get_int (tuple, FIELD_GAIN_PEAK_UNIT, NULL);

    if (gain_unit)
    {
        gain_from_playlist.album_gain = album_gain / (gfloat) gain_unit;
        gain_from_playlist.track_gain = track_gain / (gfloat) gain_unit;
    }

    if (peak_unit)
    {
        gain_from_playlist.album_peak = album_peak / (gfloat) peak_unit;
        gain_from_playlist.track_peak = track_peak / (gfloat) peak_unit;
    }
}

static gboolean ready_cb (void * unused)
{
    g_return_val_if_fail (playing, FALSE);

    hook_call ("playback ready", NULL);
    hook_call ("title change", NULL);
    ready_source = 0;
    return FALSE;
}

gboolean playback_get_ready (void)
{
    g_return_val_if_fail (playing, FALSE);
    pthread_mutex_lock (& ready_mutex);
    gboolean ready = ready_flag;
    pthread_mutex_unlock (& ready_mutex);
    return ready;
}

static void set_pb_ready (InputPlayback * p)
{
    g_return_if_fail (playing);

    pthread_mutex_lock (& ready_mutex);
    ready_flag = TRUE;
    pthread_cond_signal (& ready_cond);
    pthread_mutex_unlock (& ready_mutex);

    ready_source = g_timeout_add (0, ready_cb, NULL);
}

static void wait_until_ready (void)
{
    g_return_if_fail (playing);
    pthread_mutex_lock (& ready_mutex);

    while (! ready_flag)
        pthread_cond_wait (& ready_cond, & ready_mutex);

    pthread_mutex_unlock (& ready_mutex);
}

static void update_cb (void * hook_data, void * user_data)
{
    g_return_if_fail (playing);

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA)
        return;

    gint playlist = playlist_get_playing ();
    gint entry = playlist_get_position (playlist);
    const gchar * title = playlist_entry_get_title (playlist, entry, FALSE);

    if (title == NULL)
        title = playlist_entry_get_filename (playlist, entry);

    gint length = playlist_entry_get_length (playlist, entry, FALSE);

    if (entry == current_entry && ! strcmp (title, current_title) && length ==
     current_length)
        return;

    current_entry = entry;
    g_free (current_title);
    current_title = g_strdup (title);
    current_length = length;

    if (playback_get_ready ())
        hook_call ("title change", NULL);
}

gint playback_get_time (void)
{
    g_return_val_if_fail (playing, 0);

    if (! playback_get_ready ())
        return 0;

    gint time = -1;

    if (current_decoder->get_time != NULL)
        time = current_decoder->get_time (& playback_api);

    if (time < 0)
        time = get_output_time ();

    return time - time_offset;
}

void playback_play (gint seek_time, gboolean pause)
{
    g_return_if_fail (! playing);

    gint playlist = playlist_get_playing ();

    if (playlist == -1)
    {
        playlist = playlist_get_active ();
        playlist_set_playing (playlist);
    }

    gint entry = playlist_get_position (playlist);

    if (entry == -1)
    {
        playlist_next_song (playlist, TRUE);
        entry = playlist_get_position (playlist);

        if (entry == -1)
            return;
    }

    failed_entries = 0;
    playback_start (playlist, entry, seek_time, pause);
}

void playback_pause (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    paused = ! paused;

    g_return_if_fail (current_decoder->pause != NULL);
    current_decoder->pause (& playback_api, paused);

    if (paused)
        hook_call ("playback pause", NULL);
    else
        hook_call ("playback unpause", NULL);
}

static void playback_cleanup (void)
{
    g_return_if_fail (playing);

    pthread_join (playback_thread_handle, NULL);
    playing = FALSE;
    playback_error = FALSE;

    g_free (current_title);

    if (ready_source)
    {
        g_source_remove (ready_source);
        ready_source = 0;
    }

    cancel_set_tuple ();
    hook_dissociate ("playlist update", update_cb);
}

static void complete_stop (void)
{
    output_drain ();
    hook_call ("playback stop", NULL);

    if (cfg.stopaftersong)
    {
        cfg.stopaftersong = FALSE;
        hook_call ("toggle stop after song", NULL);
    }
}

void playback_stop (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    current_decoder->stop (& playback_api);
    playback_cleanup ();
    complete_stop ();

    if (end_source)
    {
        g_source_remove (end_source);
        end_source = 0;
    }
}

static gboolean end_cb (void * unused)
{
    g_return_val_if_fail (playing, FALSE);

    hook_call ("playback end", NULL);

    if (playback_error)
        failed_entries ++;
    else
        failed_entries = 0;

    playback_cleanup ();

    gint playlist = playlist_get_playing ();

    while (1)
    {
        gboolean play;

        if (cfg.no_playlist_advance)
            play = cfg.repeat && ! failed_entries;
        else if (! (play = playlist_next_song (playlist, cfg.repeat)))
            playlist_set_position (playlist, -1);
        else if (failed_entries >= 10)
            play = FALSE;

        if (cfg.stopaftersong)
            play = FALSE;

        if (! play)
        {
            complete_stop ();
            hook_call ("playlist end reached", NULL);
            break;
        }

        if (playback_start (playlist, playlist_get_position (playlist), 0, FALSE))
            break;

        failed_entries ++;
    }

    end_source = 0;
    return FALSE;
}

static void * playback_thread (void * unused)
{
    gchar * real = filename_split_subtune (current_filename, NULL);
    VFSFile * file = vfs_fopen (real, "r");
    g_free (real);

    playback_error = ! current_decoder->play (& playback_api, current_filename,
     file, start_time, stop_time, paused);

    if (file != NULL)
        vfs_fclose (file);

    if (! ready_flag)
        set_pb_ready (& playback_api);

    end_source = g_timeout_add (0, end_cb, NULL);
    return NULL;
}

static gboolean playback_start (gint playlist, gint entry, gint seek_time,
 gboolean pause)
{
    g_return_val_if_fail (! playing, FALSE);

    current_entry = entry;
    current_filename = playlist_entry_get_filename (playlist, entry);

    vfs_prepare_filename (current_filename);

    PluginHandle * p = playlist_entry_get_decoder (playlist, entry, FALSE);
    current_decoder = p ? plugin_get_header (p) : NULL;

    if (current_decoder == NULL)
    {
        gchar * error = g_strdup_printf (_("No decoder found for %s."),
         current_filename);
        /* The interface may not be up yet at this point. --jlindgren */
        event_queue_with_data_free ("interface show error", error);
        return FALSE;
    }

    current_data = NULL;
    current_bitrate = 0;
    current_samplerate = 0;
    current_channels = 0;

    const gchar * title = playlist_entry_get_title (playlist, entry, FALSE);
    current_title = g_strdup ((title != NULL) ? title : current_filename);

    current_length = playlist_entry_get_length (playlist, entry, FALSE);
    read_gain_from_tuple (playlist_entry_get_tuple (playlist, entry, FALSE));

    if (current_length > 0 && playlist_entry_is_segmented (playlist, entry))
    {
        time_offset = playlist_entry_get_start_time (playlist, entry);
        stop_time = playlist_entry_get_end_time (playlist, entry);
    }
    else
    {
        time_offset = 0;
        stop_time = -1;
    }

    if (current_length > 0)
        start_time = time_offset + seek_time;
    else
        start_time = 0;

    playing = TRUE;
    playback_error = FALSE;
    paused = pause;
    ready_flag = FALSE;

    pthread_create (& playback_thread_handle, NULL, playback_thread, NULL);

    hook_associate ("playlist update", update_cb, NULL);
    hook_call ("playback begin", NULL);
    return TRUE;
}

gboolean playback_get_playing (void)
{
    return playing;
}

gboolean playback_get_paused (void)
{
    g_return_val_if_fail (playing, FALSE);
    return paused;
}

void playback_seek (gint time)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    if (current_decoder->mseek == NULL || playback_get_length () < 1)
        return;

    current_decoder->mseek (& playback_api, time_offset + CLAMP (time, 0,
     current_length));

    hook_call ("playback seek", NULL);
}

static void set_data (InputPlayback * p, void * data)
{
    g_return_if_fail (playing);
    current_data = data;
}

static void * get_data (InputPlayback * p)
{
    g_return_val_if_fail (playing, NULL);
    return current_data;
}

static void set_params (InputPlayback * p, gint bitrate, gint samplerate,
 gint channels)
{
    g_return_if_fail (playing);

    current_bitrate = bitrate;
    current_samplerate = samplerate;
    current_channels = channels;

    event_queue ("info change", NULL);
}

static gboolean set_tuple_cb (void * unused)
{
    g_return_val_if_fail (playing, FALSE);
    pthread_mutex_lock (& ready_mutex);

    gint playlist = playlist_get_playing ();
    playlist_entry_set_tuple (playlist, playlist_get_position (playlist),
     tuple_to_be_set);
    set_tuple_source = 0;
    tuple_to_be_set = NULL;

    pthread_mutex_unlock (& ready_mutex);
    return FALSE;
}

static void set_tuple (InputPlayback * p, Tuple * tuple)
{
    g_return_if_fail (playing);
    pthread_mutex_lock (& ready_mutex);

    cancel_set_tuple ();
    set_tuple_source = g_timeout_add (0, set_tuple_cb, NULL);
    tuple_to_be_set = tuple;

    read_gain_from_tuple (tuple);
    pthread_mutex_unlock (& ready_mutex);
}

static void set_gain_from_playlist (InputPlayback * p)
{
    g_return_if_fail (playing);
    p->output->set_replaygain_info (& gain_from_playlist);
}

static InputPlayback playback_api = {
    .output = & output_api,
    .set_data = set_data,
    .get_data = get_data,
    .set_pb_ready = set_pb_ready,
    .set_params = set_params,
    .set_tuple = set_tuple,
    .set_gain_from_playlist = set_gain_from_playlist,
};

gchar * playback_get_title (void)
{
    g_return_val_if_fail (playing, NULL);

    if (! playback_get_ready ())
        return g_strdup (_("Buffering ..."));

    gchar s[128];

    if (current_length)
    {
        gint len = current_length / 1000;

        if (len < 3600)
            snprintf (s, sizeof s, cfg.leading_zero ? " (%02d:%02d)" :
             " (%d:%02d)", len / 60, len % 60);
        else
            snprintf (s, sizeof s, " (%d:%02d:%02d)", len / 3600, (len / 60) %
             60, len % 60);
    }
    else
        s[0] = 0;

    if (cfg.show_numbers_in_pl)
        return g_strdup_printf ("%d. %s%s", 1 + playlist_get_position
         (playlist_get_playing ()), current_title, s);

    return g_strdup_printf ("%s%s", current_title, s);
}

gint playback_get_length (void)
{
    g_return_val_if_fail (playing, 0);
    return current_length;
}

void playback_get_info (gint * bitrate, gint * samplerate, gint * channels)
{
    g_return_if_fail (playing);
    * bitrate = current_bitrate;
    * samplerate = current_samplerate;
    * channels = current_channels;
}

void playback_get_volume (gint * l, gint * r)
{
    if (playing && current_decoder->get_volume != NULL &&
     current_decoder->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void playback_set_volume(gint l, gint r)
{
    gint h_vol[2] = {l, r};

    hook_call ("volume set", h_vol);

    if (playing && current_decoder->set_volume != NULL &&
     current_decoder->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
