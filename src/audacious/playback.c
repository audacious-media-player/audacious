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

#include <libaudcore/audstrings.h>
#include <libaudcore/eventqueue.h>
#include <libaudcore/hook.h>

#include "audconfig.h"
#include "config.h"
#include "i18n.h"
#include "interface.h"
#include "main.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"

static void set_params (InputPlayback * playback, const gchar * title, gint
 length, gint bitrate, gint samplerate, gint channels);
static void set_tuple (InputPlayback * playback, Tuple * tuple);
static void set_gain_from_playlist (InputPlayback * playback);

static void playback_free (InputPlayback * playback);
static gboolean playback_play_file (gint playlist, gint entry, gint seek_time,
 gboolean pause);

InputPlayback * current_playback = NULL;

static gint time_offset;
static gboolean paused;
static gboolean stopping;
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

    if ((title = playlist_entry_get_title (playlist, entry, FALSE)) == NULL)
        title = playlist_entry_get_filename (playlist, entry);

    length = playlist_entry_get_length (playlist, entry, FALSE);

    if (! strcmp (title, current_playback->title) && length ==
     current_playback->length)
        return;

    g_free (current_playback->title);
    current_playback->title = g_strdup (title);
    current_playback->length = length;
    hook_call ("title change", NULL);
}

gint playback_get_time (void)
{
    if (! playback_is_ready ())
        return 0;

    gint time = -1;

    if (current_playback->plugin->get_time != NULL)
        time = current_playback->plugin->get_time (current_playback);

    if (time < 0)
        time = get_output_time ();

    return time - time_offset;
}

void playback_play (gint seek_time, gboolean pause)
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

    failed_entries = 0;
    playback_play_file (playlist, entry, seek_time, pause);
}

void playback_pause (void)
{
    if (! playback_is_ready ())
        return;

    paused = ! paused;

    g_return_if_fail (current_playback->plugin->pause != NULL);
    current_playback->plugin->pause (current_playback, paused);

    if (paused)
        hook_call("playback pause", NULL);
    else
        hook_call("playback unpause", NULL);
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

    current_playback->plugin->stop (current_playback);

    /* some plugins do this themselves */
    if (current_playback->thread != NULL)
        g_thread_join (current_playback->thread);

    cancel_set_tuple ();
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

static gboolean playback_ended (void * unused)
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

        if (playback_play_file (playlist, playlist_get_position (playlist), 0,
         FALSE))
            break;

        failed_entries ++;
    }

    return FALSE;
}

typedef struct
{
    gint start_time, stop_time;
    gboolean pause;
}
PlayParams;

static void * playback_monitor_thread (void * data)
{
    if (current_playback->plugin->play != NULL)
    {
        PlayParams * params = data;
        VFSFile * file = vfs_fopen (current_playback->filename, "r");

        current_playback->error = ! current_playback->plugin->play
         (current_playback, current_playback->filename, file,
         params->start_time, params->stop_time, params->pause);

        if (file != NULL)
            vfs_fclose (file);
    }
    else
    {
        fprintf (stderr, "%s should be updated to provide play().\n",
         current_playback->plugin->description);
        g_return_val_if_fail (current_playback->plugin->play_file != NULL, NULL);
        current_playback->plugin->play_file (current_playback);
    }

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
static void playback_pass_audio (InputPlayback * playback, gint format, gint
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

    playback->output = & output_api;

    /* init vtable functors */
    playback->set_pb_ready = playback_set_pb_ready;
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

    g_slice_free(InputPlayback, playback);
}

static void playback_run (gint start_time, gint stop_time, gboolean pause)
{
    current_playback->playing = FALSE;
    current_playback->eof = FALSE;
    current_playback->error = FALSE;

    paused = pause;
    stopping = FALSE;
    ready_source = 0;

    static PlayParams params;
    params.start_time = start_time;
    params.stop_time = stop_time;
    params.pause = pause;

    current_playback->thread = g_thread_create (playback_monitor_thread,
     & params, TRUE, NULL);
}

static gboolean playback_play_file (gint playlist, gint entry, gint seek_time,
 gboolean pause)
{
    const gchar * filename = playlist_entry_get_filename (playlist, entry);
    const gchar * title = playlist_entry_get_title (playlist, entry, FALSE);
    InputPlugin * decoder = playlist_entry_get_decoder (playlist, entry);
    Tuple * tuple = (Tuple *) playlist_entry_get_tuple (playlist, entry, FALSE);

    g_return_val_if_fail (current_playback == NULL, FALSE);

    if (decoder == NULL)
    {
        gchar * error = g_strdup_printf (_("No decoder found for %s."), filename);

        interface_show_error_message (error);
        g_free (error);
        return FALSE;
    }

    read_gain_from_tuple (tuple); /* even if tuple == NULL */

    current_playback = playback_new ();
    current_playback->plugin = decoder;
    current_playback->filename = g_strdup (filename);
    current_playback->title = g_strdup ((title != NULL) ? title : filename);
    current_playback->length = playlist_entry_get_length (playlist, entry, FALSE);

    if (playlist_entry_is_segmented (playlist, entry))
    {
        time_offset = playlist_entry_get_start_time (playlist, entry);
        playback_run (time_offset + seek_time, playlist_entry_get_end_time
         (playlist, entry), pause);
    }
    else
    {
        time_offset = 0;
        playback_run (seek_time, -1, pause);
    }

#ifdef USE_DBUS /* Fix me: Use a "playback begin" hook in dbus.c. */
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

    if (! playback_is_ready ())
        return;

    time = CLAMP (time, 0, current_playback->length);
    time += time_offset;

    if (current_playback->plugin->mseek != NULL)
        current_playback->plugin->mseek (current_playback, time);
    else if (current_playback->plugin->seek != NULL)
    {
        fprintf (stderr, "%s should be updated to provide mseek().\n",
         current_playback->plugin->description);
        current_playback->plugin->seek (current_playback, time / 1000);
    }

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

void
input_get_volume(gint * l, gint * r)
{
    if (current_playback && current_playback->plugin->get_volume &&
     current_playback->plugin->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void
input_set_volume(gint l, gint r)
{
    gint h_vol[2] = {l, r};

    hook_call("volume set", h_vol);

    if (current_playback && current_playback->plugin->set_volume &&
     current_playback->plugin->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
