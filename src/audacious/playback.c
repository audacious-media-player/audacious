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
static void set_tuple (InputPlayback * playback, Tuple * tuple);
static void set_gain_from_playlist (InputPlayback * playback);

static gboolean playback_segmented_end(gpointer data);

static void playback_free (InputPlayback * playback);
static gboolean playback_play_file (gint playlist, gint entry);

InputPlayback * current_playback = NULL;

static gboolean paused;
static gboolean stopping;
static gint seek_when_ready;
static gint ready_source;
static gint failed_entries;
static gint set_tuple_source = 0;
static Tuple * tuple_to_be_set = NULL;
static ReplayGainInfo gain_from_playlist;

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
static void read_gain_from_tuple (Tuple * tuple)
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
    g_return_val_if_fail (current_playback != NULL, FALSE);

    g_mutex_lock (current_playback->pb_ready_mutex);
    ready_source = 0;
    g_mutex_unlock (current_playback->pb_ready_mutex);

    if (paused)
    {
        paused = ! paused; /* playback_pause toggles it */
        playback_pause ();
    }

    if (seek_when_ready > 0)
        playback_seek (seek_when_ready);

    hook_call ("title change", NULL);
    return FALSE;
}

static gboolean playback_is_ready (void)
{
    gboolean ready;

    g_return_val_if_fail (current_playback != NULL, FALSE);

    g_mutex_lock (current_playback->pb_ready_mutex);
    ready = (current_playback->pb_ready_val && ! ready_source);
    g_mutex_unlock (current_playback->pb_ready_mutex);
    return ready;
}

static gint
playback_set_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    playback->pb_ready_val = 1;
    ready_source = g_timeout_add (0, ready_cb, NULL);
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
    gint playlist, entry, length;
    const gchar * title;

    g_return_if_fail (current_playback != NULL);

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA ||
     ! playback_is_ready ())
        return;

    playlist = playlist_get_playing ();
    entry = playlist_get_position (playlist);

    if ((title = playlist_entry_get_title (playlist, entry)) == NULL)
        title = playlist_entry_get_filename (playlist, entry);

    length = playlist_entry_get_length (playlist, entry);

    if (! strcmp (title, current_playback->title) && length ==
     current_playback->length)
        return;

    g_free (current_playback->title);
    current_playback->title = g_strdup (title);
    current_playback->length = length;
    hook_call ("title change", NULL);
}

static gint
playback_get_time_real(void)
{
    gint time = -1;

    g_return_val_if_fail (current_playback != NULL, 0);

    if (! playback_is_ready ())
        return seek_when_ready;

    if (! current_playback->playing || current_playback->error)
        return 0;

    if (current_playback->plugin->get_time != NULL)
        time = current_playback->plugin->get_time (current_playback);

    if (time < 0)
        time = get_output_time ();

    return time;
}

gint playback_get_time (void)
{
    g_return_val_if_fail (current_playback != NULL, 0);

    if (current_playback->start > 0)
        return playback_get_time_real () - current_playback->start;
    else
        return playback_get_time_real ();
}

void playback_initiate (void)
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
    g_return_if_fail (current_playback != NULL);

    paused = ! paused;

    if (playback_is_ready ())
    {
        if (current_playback->end_timeout)
        {
            g_source_remove(current_playback->end_timeout);
            current_playback->end_timeout = 0;
        }

        g_return_if_fail (current_playback->plugin->pause != NULL);
        current_playback->plugin->pause (current_playback, paused);
    }

    if (paused)
        hook_call("playback pause", NULL);
    else
    {
        hook_call("playback unpause", NULL);

        if (current_playback->end > 0)
            current_playback->end_timeout = g_timeout_add (current_playback->end
             - playback_get_time_real (), playback_segmented_end, NULL);
    }
}

static void playback_finalize (void)
{
    hook_dissociate ("playlist update", update_cb);

    g_mutex_lock (current_playback->pb_ready_mutex);

    while (! current_playback->pb_ready_val)
        g_cond_wait (current_playback->pb_ready_cond,
         current_playback->pb_ready_mutex);

    if (ready_source)
    {
        g_source_remove (ready_source);
        ready_source = 0;
    }

    g_mutex_unlock (current_playback->pb_ready_mutex);

    if (current_playback->playing)
        current_playback->plugin->stop (current_playback);

    /* some plugins do this themselves */
    if (current_playback->thread != NULL)
        g_thread_join (current_playback->thread);

    cancel_set_tuple ();

    if (current_playback->end_timeout)
        g_source_remove (current_playback->end_timeout);

    playback_free (current_playback);
    current_playback = NULL;
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
    g_return_if_fail (current_playback != NULL);

    stopping = TRUE;
    playback_finalize ();
    stopping = FALSE;

    complete_stop ();
}

static gboolean playback_ended (void * user_data)
{
    gint playlist = playlist_get_playing ();
    gboolean play;

    g_return_val_if_fail (current_playback != NULL, FALSE);

    if (current_playback->error)
        failed_entries ++;
    else
        failed_entries = 0;

    playback_finalize ();

    while (1)
    {
        if (cfg.no_playlist_advance)
            play = cfg.repeat && ! failed_entries;
        else
        {
            if (! (play = playlist_next_song (playlist, cfg.repeat)))
                playlist_set_position (playlist, -1);

            if (failed_entries >= 10)
                play = FALSE;
        }

        if (cfg.stopaftersong)
            play = FALSE;

        if (! play)
        {
            complete_stop ();
            break;
        }

        if (playback_play_file (playlist, playlist_get_position (playlist)))
            break;

        failed_entries ++;
    }

    return FALSE;
}

static gboolean playback_segmented_end (void * unused)
{
    if (playlist_next_song (playlist_get_playing (), cfg.repeat))
        playback_initiate ();

    return FALSE;
}

static gboolean playback_segmented_start (void * unused)
{
    g_return_val_if_fail (current_playback != NULL, FALSE);

    if (current_playback->plugin->mseek != NULL)
        current_playback->plugin->mseek (current_playback,
         current_playback->start);
    else if (current_playback->plugin->seek != NULL)
        current_playback->plugin->seek (current_playback,
         current_playback->start / 1000);

    if (current_playback->end > 0)
        current_playback->end_timeout = g_timeout_add (current_playback->end -
         current_playback->start, playback_segmented_end, current_playback);

    return FALSE;
}

static gpointer
playback_monitor_thread(gpointer data)
{
    if (current_playback->segmented)
        g_idle_add (playback_segmented_start, current_playback);

    current_playback->plugin->play_file (current_playback);

    g_mutex_lock (current_playback->pb_ready_mutex);
    current_playback->pb_ready_val = TRUE;

    if (ready_source != 0)
    {
        g_source_remove (ready_source);
        ready_source = 0;
    }

    g_cond_signal (current_playback->pb_ready_cond);
    g_mutex_unlock (current_playback->pb_ready_mutex);

    if (! stopping)
        g_timeout_add (0, playback_ended, NULL);

    return NULL;
}

/* compatibility */
static void playback_set_replaygain_info (InputPlayback * playback,
 ReplayGainInfo * info)
{
    fprintf (stderr, "Plugin %s should be updated to use OutputAPI::"
     "set_replaygain_info or (better) InputPlayback::set_gain_from_playlist.\n",
     playback->plugin->description);

    playback->output->set_replaygain_info (info);
}

/* compatibility */
static void playback_pass_audio (InputPlayback * playback, AFormat format, gint
 channels, gint size, void * data, gint * going)
{
    static gboolean warned = FALSE;

    if (! warned)
    {
        fprintf (stderr, "Plugin %s should be updated to use OutputAPI::"
         "write_audio.\n", playback->plugin->description);
        warned = TRUE;
    }

    playback->output->write_audio (data, size);
}

static InputPlayback * playback_new (void)
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
    playback->set_tuple = set_tuple;
    playback->set_gain_from_playlist = set_gain_from_playlist;

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
static void playback_free (InputPlayback * playback)
{
    g_free(playback->filename);
    g_free(playback->title);

    g_mutex_free(playback->pb_ready_mutex);
    g_cond_free(playback->pb_ready_cond);

    g_mutex_free(playback->pb_change_mutex);
    g_cond_free(playback->pb_change_cond);

    g_slice_free(InputPlayback, playback);
}

static void playback_run (void)
{
    current_playback->playing = FALSE;
    current_playback->eof = FALSE;
    current_playback->error = FALSE;

    paused = FALSE;
    stopping = FALSE;
    seek_when_ready = 0;
    ready_source = 0;

    current_playback->thread = g_thread_create (playback_monitor_thread,
     current_playback, TRUE, NULL);
}

static gboolean playback_play_file (gint playlist, gint entry)
{
    const gchar * filename = playlist_entry_get_filename (playlist, entry);
    const gchar * title = playlist_entry_get_title (playlist, entry);
    InputPlugin * decoder = playlist_entry_get_decoder (playlist, entry);
    Tuple * tuple = (Tuple *) playlist_entry_get_tuple (playlist, entry);

    g_return_val_if_fail (current_playback == NULL, FALSE);

    if (decoder == NULL)
        decoder = file_find_decoder (filename, FALSE);

    if (decoder == NULL)
    {
        gchar * error = g_strdup_printf (_("No decoder found for %s."), filename);

        interface_show_error_message (error);
        g_free (error);
        return FALSE;
    }

    if (tuple == NULL)
    {
        tuple = file_read_tuple (filename, decoder);

        if (tuple != NULL)
            playlist_entry_set_tuple (playlist, entry, tuple);
    }

    read_gain_from_tuple (tuple); /* even if tuple == NULL */

    current_playback = playback_new ();
    current_playback->plugin = decoder;
    current_playback->filename = g_strdup (filename);
    current_playback->title = g_strdup ((title != NULL) ? title : filename);
    current_playback->length = playlist_entry_get_length (playlist, entry);
    current_playback->segmented = playlist_entry_is_segmented (playlist, entry);
    current_playback->start = playlist_entry_get_start_time (playlist, entry);
    current_playback->end = playlist_entry_get_end_time (playlist, entry);

    playback_run ();

#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif

    hook_associate ("playlist update", update_cb, NULL);
    hook_call ("playback begin", NULL);
    return TRUE;
}

gboolean playback_get_playing (void)
{
    return (current_playback != NULL);
}

gboolean playback_get_paused (void)
{
    g_return_val_if_fail (current_playback != NULL, FALSE);

    return paused;
}

void playback_seek (gint time)
{
    g_return_if_fail (current_playback != NULL);

    if (current_playback->length <= 0)
        return;

    time = CLAMP (time, 0, current_playback->length);

    if (playback_is_ready ())
    {
        if (current_playback->start > 0)
            time += current_playback->start;

        if (current_playback->plugin->mseek != NULL)
            current_playback->plugin->mseek (current_playback, time);
        else if (current_playback->plugin->seek != NULL)
            current_playback->plugin->seek (current_playback, time / 1000);

        if (current_playback->end > 0)
        {
            if (current_playback->end_timeout)
                g_source_remove (current_playback->end_timeout);

            current_playback->end_timeout = g_timeout_add (current_playback->end
             - time, playback_segmented_end, NULL);
        }
    }
    else
        seek_when_ready = time;

    hook_call ("playback seek", NULL);
}

static void set_params (InputPlayback * playback, const gchar * title, gint
 length, gint bitrate, gint samplerate, gint channels)
{
    playback->rate = bitrate;
    playback->freq = samplerate;
    playback->nch = channels;

    event_queue ("info change", NULL);
}

static gboolean set_tuple_cb (void * unused)
{
    gint playlist = playlist_get_playing ();

    g_return_val_if_fail (current_playback != NULL, FALSE);
    g_mutex_lock (current_playback->pb_ready_mutex);

    playlist_entry_set_tuple (playlist, playlist_get_position (playlist),
     tuple_to_be_set);
    set_tuple_source = 0;
    tuple_to_be_set = NULL;

    g_mutex_unlock (current_playback->pb_ready_mutex);

    return FALSE;
}

static void set_tuple (InputPlayback * playback, Tuple * tuple)
{
    g_mutex_lock (playback->pb_ready_mutex);

    /* playlist_entry_set_tuple must execute in main thread */
    cancel_set_tuple ();
    set_tuple_source = g_timeout_add (0, set_tuple_cb, NULL);
    tuple_to_be_set = tuple;

    read_gain_from_tuple (tuple);

    g_mutex_unlock (playback->pb_ready_mutex);
}

static void set_gain_from_playlist (InputPlayback * playback)
{
    playback->output->set_replaygain_info (& gain_from_playlist);
}

gchar * playback_get_title (void)
{
    gchar * suffix, * title;

    g_return_val_if_fail (current_playback != NULL, NULL);

    if (! playback_is_ready ())
        return g_strdup (_("Buffering ..."));

    suffix = (current_playback->length > 0) ? g_strdup_printf (" (%d:%02d)",
     current_playback->length / 60000, current_playback->length / 1000 % 60) :
     NULL;

    if (cfg.show_numbers_in_pl)
        title = g_strdup_printf ("%d. %s%s", 1 + playlist_get_position
         (playlist_get_playing ()), current_playback->title, (suffix != NULL) ?
         suffix : "");
    else
        title = g_strdup_printf ("%s%s", current_playback->title, (suffix !=
         NULL) ? suffix : "");

    g_free (suffix);
    return title;
}

gint playback_get_length (void)
{
    g_return_val_if_fail (current_playback != NULL, 0);

    return current_playback->length;
}

void playback_get_info (gint * bitrate, gint * samplerate, gint * channels)
{
    g_return_if_fail (current_playback != NULL);

    * bitrate = current_playback->rate;
    * samplerate = current_playback->freq;
    * channels = current_playback->nch;
}
