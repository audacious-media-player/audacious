/*
 * playback.c
 * Copyright 2009-2012 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <pthread.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "plugin.h"

static void playback_start (int playlist, int entry, int seek_time, bool_t pause);

static const struct OutputAPI output_api = {
 .open_audio = output_open_audio,
 .set_replaygain_info = output_set_replaygain_info,
 .write_audio = output_write_audio,
 .abort_write = output_abort_write,
 .pause = output_pause,
 .written_time = output_written_time,
 .flush = output_set_time};

static InputPlayback playback_api;

static bool_t playing = FALSE;
static bool_t playback_error;
static int failed_entries;

static char * current_filename; /* pooled */

static int current_entry;
static char * current_title; /* pooled */
static int current_length;

static InputPlugin * current_decoder;
static void * current_data;
static int current_bitrate, current_samplerate, current_channels;

static ReplayGainInfo gain_from_playlist;

static int time_offset, initial_seek;
static bool_t paused;

static pthread_t playback_thread_handle;
static int end_source = 0;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;
static bool_t ready_flag;

/* clears gain info if tuple == NULL */
static void read_gain_from_tuple (const Tuple * tuple)
{
    memset (& gain_from_playlist, 0, sizeof gain_from_playlist);

    if (tuple == NULL)
        return;

    int album_gain = tuple_get_int (tuple, FIELD_GAIN_ALBUM_GAIN, NULL);
    int album_peak = tuple_get_int (tuple, FIELD_GAIN_ALBUM_PEAK, NULL);
    int track_gain = tuple_get_int (tuple, FIELD_GAIN_TRACK_GAIN, NULL);
    int track_peak = tuple_get_int (tuple, FIELD_GAIN_TRACK_PEAK, NULL);
    int gain_unit = tuple_get_int (tuple, FIELD_GAIN_GAIN_UNIT, NULL);
    int peak_unit = tuple_get_int (tuple, FIELD_GAIN_PEAK_UNIT, NULL);

    if (gain_unit)
    {
        gain_from_playlist.album_gain = album_gain / (float) gain_unit;
        gain_from_playlist.track_gain = track_gain / (float) gain_unit;
    }

    if (peak_unit)
    {
        gain_from_playlist.album_peak = album_peak / (float) peak_unit;
        gain_from_playlist.track_peak = track_peak / (float) peak_unit;
    }
}

static bool_t update_from_playlist (void)
{
    int entry = playback_entry_get_position ();
    char * title = playback_entry_get_title ();
    int length = playback_entry_get_length ();

    if (entry == current_entry && ! g_strcmp0 (title, current_title) && length == current_length)
    {
        str_unref (title);
        return FALSE;
    }

    current_entry = entry;
    str_unref (current_title);
    current_title = title;
    current_length = length;
    return TRUE;
}

bool_t playback_get_ready (void)
{
    g_return_val_if_fail (playing, FALSE);
    pthread_mutex_lock (& ready_mutex);
    bool_t ready = ready_flag;
    pthread_mutex_unlock (& ready_mutex);
    return ready;
}

static void set_pb_ready (InputPlayback * p)
{
    g_return_if_fail (playing);
    pthread_mutex_lock (& ready_mutex);

    update_from_playlist ();
    ready_flag = TRUE;

    pthread_cond_signal (& ready_cond);
    pthread_mutex_unlock (& ready_mutex);

    event_queue ("playback ready", NULL);
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

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA || ! playback_get_ready ())
        return;

    if (update_from_playlist ())
        event_queue ("title change", NULL);
}

int playback_get_time (void)
{
    g_return_val_if_fail (playing, 0);
    wait_until_ready ();

    int time = -1;

    if (current_decoder && current_decoder->get_time)
        time = current_decoder->get_time (& playback_api);

    if (time < 0)
        time = output_get_time ();

    return time - time_offset;
}

void playback_play (int seek_time, bool_t pause)
{
    g_return_if_fail (! playing);

    int playlist = playlist_get_playing ();
    if (playlist < 0)
        return;

    int entry = playlist_get_position (playlist);
    if (entry < 0)
        return;

    failed_entries = 0;
    playback_start (playlist, entry, seek_time, pause);
}

void playback_pause (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    if (! current_decoder || ! current_decoder->pause)
        return;

    paused = ! paused;
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

    event_queue_cancel ("playback ready", NULL);
    event_queue_cancel ("playback seek", NULL);
    event_queue_cancel ("info change", NULL);
    event_queue_cancel ("title change", NULL);

    if (end_source)
    {
        g_source_remove (end_source);
        end_source = 0;
    }

    str_unref (current_filename);
    current_filename = NULL;
    str_unref (current_title);
    current_title = NULL;

    hook_dissociate ("playlist update", update_cb);
}

static void complete_stop (void)
{
    output_drain ();
    hook_call ("playback stop", NULL);
    set_bool (NULL, "stop_after_current_song", FALSE);
}

void playback_stop (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    if (current_decoder)
        current_decoder->stop (& playback_api);

    playback_cleanup ();
    complete_stop ();
}

static bool_t end_cb (void * unused)
{
    g_return_val_if_fail (playing, FALSE);

    hook_call ("playback end", NULL);

    if (playback_error)
        failed_entries ++;
    else
        failed_entries = 0;

    playback_cleanup ();

    int playlist = playlist_get_playing ();
    bool_t play;

    if (get_bool (NULL, "no_playlist_advance"))
        play = get_bool (NULL, "repeat") && ! failed_entries;
    else if (! (play = playlist_next_song (playlist, get_bool (NULL, "repeat"))))
        playlist_set_position (playlist, -1);
    else if (failed_entries >= 10)
        play = FALSE;

    if (get_bool (NULL, "stop_after_current_song"))
        play = FALSE;

    if (play)
        playback_start (playlist, playlist_get_position (playlist), 0, FALSE);
    else
    {
        complete_stop ();
        hook_call ("playlist end reached", NULL);
    }

    return FALSE;
}

static void * playback_thread (void * unused)
{
    PluginHandle * p = playback_entry_get_decoder ();
    current_decoder = p ? plugin_get_header (p) : NULL;

    if (! current_decoder)
    {
        char * error = g_strdup_printf (_("No decoder found for %s."),
         current_filename);
        interface_show_error (error);
        g_free (error);
        playback_error = TRUE;
        goto DONE;
    }

    current_data = NULL;
    current_bitrate = 0;
    current_samplerate = 0;
    current_channels = 0;

    Tuple * tuple = playback_entry_get_tuple ();
    read_gain_from_tuple (tuple);
    if (tuple)
        tuple_unref (tuple);

    bool_t seekable = (playback_entry_get_length () > 0);

    VFSFile * file = vfs_fopen (current_filename, "r");

    time_offset = seekable ? playback_entry_get_start_time () : 0;
    playback_error = ! current_decoder->play (& playback_api, current_filename,
     file, seekable ? time_offset + initial_seek : 0,
     seekable ? playback_entry_get_end_time () : -1, paused);

    output_close_audio ();

    if (file)
        vfs_fclose (file);

DONE:
    if (! ready_flag)
        set_pb_ready (& playback_api);

    end_source = g_timeout_add (0, end_cb, NULL);
    return NULL;
}

static void playback_start (int playlist, int entry, int seek_time, bool_t pause)
{
    g_return_if_fail (! playing);

    current_filename = playlist_entry_get_filename (playlist, entry);
    g_return_if_fail (current_filename);

    playing = TRUE;
    playback_error = FALSE;
    ready_flag = FALSE;

    current_entry = -1;
    current_title = NULL;
    current_length = 0;

    initial_seek = seek_time;
    paused = pause;

    hook_associate ("playlist update", update_cb, NULL);
    pthread_create (& playback_thread_handle, NULL, playback_thread, NULL);

    hook_call ("playback begin", NULL);
}

bool_t playback_get_playing (void)
{
    return playing;
}

bool_t playback_get_paused (void)
{
    g_return_val_if_fail (playing, FALSE);
    return paused;
}

void playback_seek (int time)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    if (! current_decoder || ! current_decoder->mseek || current_length < 1)
        return;

    current_decoder->mseek (& playback_api, time_offset + CLAMP (time, 0,
     current_length));

    /* If the plugin is using our output system, don't call "playback seek"
     * immediately but wait for output_set_time() to be called.  This ensures
     * that a "playback seek" handler can call playback_get_time() and get the
     * new time. */
    if (! output_is_open ())
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

static void set_params (InputPlayback * p, int bitrate, int samplerate,
 int channels)
{
    g_return_if_fail (playing);

    current_bitrate = bitrate;
    current_samplerate = samplerate;
    current_channels = channels;

    if (playback_get_ready ())
        event_queue ("info change", NULL);
}

static void set_tuple (InputPlayback * p, Tuple * tuple)
{
    g_return_if_fail (playing);
    read_gain_from_tuple (tuple);
    playback_entry_set_tuple (tuple);
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

char * playback_get_filename (void)
{
    g_return_val_if_fail (playing, NULL);
    return str_ref (current_filename);
}

char * playback_get_title (void)
{
    g_return_val_if_fail (playing, NULL);
    wait_until_ready ();

    char s[32];

    if (current_length)
    {
        int len = current_length / 1000;

        if (len < 3600)
            snprintf (s, sizeof s, get_bool (NULL, "leading_zero") ?
             " (%02d:%02d)" : " (%d:%02d)", len / 60, len % 60);
        else
            snprintf (s, sizeof s, " (%d:%02d:%02d)", len / 3600, (len / 60) %
             60, len % 60);
    }
    else
        s[0] = 0;

    if (get_bool (NULL, "show_numbers_in_pl"))
        return str_printf ("%d. %s%s", 1 + playlist_get_position
         (playlist_get_playing ()), current_title, s);

    return str_printf ("%s%s", current_title, s);
}

int playback_get_length (void)
{
    g_return_val_if_fail (playing, 0);
    wait_until_ready ();

    return current_length;
}

void playback_get_info (int * bitrate, int * samplerate, int * channels)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    * bitrate = current_bitrate;
    * samplerate = current_samplerate;
    * channels = current_channels;
}

void playback_get_volume (int * l, int * r)
{
    if (playing && playback_get_ready () && current_decoder &&
     current_decoder->get_volume && current_decoder->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void playback_set_volume (int l, int r)
{
    if (playing && playback_get_ready () && current_decoder &&
     current_decoder->set_volume && current_decoder->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
