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

#include "drct.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "plugin.h"

static bool_t open_audio_wrapper (int format, int rate, int channels);
static void write_audio_wrapper (void * data, int length);

static const struct OutputAPI output_api = {
 .open_audio = open_audio_wrapper,
 .set_replaygain_info = output_set_replaygain_info,
 .write_audio = write_audio_wrapper,
 .written_time = output_written_time};

static InputPlayback playback_api;

static pthread_t playback_thread_handle;
static int end_source = 0;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;

/* level 1 data (persists to end of song) */
static bool_t playing = FALSE;
static bool_t restart_flag = FALSE;
static int time_offset = 0;
static int start_time = 0;
static bool_t paused = FALSE;
static bool_t ready_flag = FALSE;
static bool_t playback_error = FALSE;
static bool_t song_finished = FALSE;

static int current_bitrate = -1, current_samplerate = -1, current_channels = -1;

/* level 2 data (persists when restarting same song) */
static int current_entry = -1;
static char * current_filename = NULL; /* pooled */
static char * current_title = NULL; /* pooled */
static int current_length = -1;

static InputPlugin * current_decoder = NULL;
static VFSFile * current_file = NULL;
static ReplayGainInfo gain_from_playlist;

static int repeat_a = -1, repeat_b = -1;

/* level 3 data (persists to end of playlist) */
static bool_t stopped = TRUE;
static int failed_entries = 0;

/* playback thread control */
static bool_t stop_flag = FALSE;
static int seek_request = -1;

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

    /* pointer comparison works for pooled strings */
    if (entry == current_entry && title == current_title && length == current_length)
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

bool_t drct_get_ready (void)
{
    if (! playing)
        return FALSE;

    pthread_mutex_lock (& ready_mutex);

    /* on restart, always report ready */
    bool_t ready = ready_flag || restart_flag;

    pthread_mutex_unlock (& ready_mutex);
    return ready;
}

static void set_ready (void)
{
    g_return_if_fail (playing);
    pthread_mutex_lock (& ready_mutex);

    /* on restart, don't update or send "playback ready" */
    if (! restart_flag)
    {
        update_from_playlist ();
        event_queue ("playback ready", NULL);
    }

    ready_flag = TRUE;

    pthread_cond_signal (& ready_cond);
    pthread_mutex_unlock (& ready_mutex);
}

static void wait_until_ready (void)
{
    g_return_if_fail (playing);
    pthread_mutex_lock (& ready_mutex);

    /* on restart, we still have to wait, but presumably not long */
    while (! ready_flag)
        pthread_cond_wait (& ready_cond, & ready_mutex);

    pthread_mutex_unlock (& ready_mutex);
}

static void update_cb (void * hook_data, void * user_data)
{
    g_return_if_fail (playing);

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA || ! drct_get_ready ())
        return;

    if (update_from_playlist ())
        event_queue ("title change", NULL);
}

int drct_get_time (void)
{
    if (! playing)
        return 0;

    wait_until_ready ();

    return output_get_time () - time_offset;
}

void drct_pause (void)
{
    if (! playing)
        return;

    wait_until_ready ();

    paused = ! paused;

    output_pause (paused);

    if (paused)
        hook_call ("playback pause", NULL);
    else
        hook_call ("playback unpause", NULL);
}

static void default_stop (void)
{
    pthread_mutex_lock (& control_mutex);
    stop_flag = TRUE;
    output_abort_write ();
    pthread_mutex_unlock (& control_mutex);
}

static void playback_finish (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    /* calling stop() is unnecessary if the song finished on its own;
     * also, it might flush the output buffer, breaking gapless playback */
    if (! song_finished)
        default_stop ();

    pthread_join (playback_thread_handle, NULL);
    output_close_audio ();

    hook_dissociate ("playlist update", update_cb);

    if (end_source)
    {
        g_source_remove (end_source);
        end_source = 0;
    }

    /* level 1 data cleanup */
    playing = FALSE;
    restart_flag = FALSE;
    time_offset = 0;
    start_time = 0;
    paused = FALSE;
    ready_flag = FALSE;
    playback_error = FALSE;
    song_finished = FALSE;

    current_bitrate = current_samplerate = current_channels = -1;
}

static void playback_cleanup (void)
{
    g_return_if_fail (current_filename);
    playback_finish ();

    event_queue_cancel ("playback ready", NULL);
    event_queue_cancel ("playback seek", NULL);
    event_queue_cancel ("info change", NULL);
    event_queue_cancel ("title change", NULL);

    set_bool (NULL, "stop_after_current_song", FALSE);

    /* level 2 data cleanup */
    current_entry = -1;
    str_unref (current_filename);
    current_filename = NULL;
    str_unref (current_title);
    current_title = NULL;
    current_length = -1;

    current_decoder = NULL;

    if (current_file)
    {
        vfs_fclose (current_file);
        current_file = NULL;
    }

    read_gain_from_tuple (NULL);

    repeat_a = repeat_b = -1;
}

void playback_stop (void)
{
    if (stopped)
        return;

    if (current_filename)
        playback_cleanup ();

    output_drain ();

    /* level 3 data cleanup */
    stopped = TRUE;
    failed_entries = 0;

    hook_call ("playback stop", NULL);
}

static bool_t end_cb (void * unused)
{
    g_return_val_if_fail (playing, FALSE);

    if (! playback_error)
        song_finished = TRUE;

    hook_call ("playback end", NULL);

    if (playback_error)
        failed_entries ++;
    else
        failed_entries = 0;

    int playlist = playlist_get_playing ();

    if (get_bool (NULL, "stop_after_current_song"))
        goto STOP;

    if (repeat_a >= 0 || repeat_b >= 0)
    {
        if (failed_entries)
            goto STOP;

        playback_play (MAX (repeat_a, 0), FALSE);
    }
    else if (get_bool (NULL, "no_playlist_advance"))
    {
        if (failed_entries || ! get_bool (NULL, "repeat"))
            goto STOP;

        playback_play (0, FALSE);
    }
    else
    {
        if (failed_entries >= 10)
            goto STOP;

        if (! playlist_next_song (playlist, get_bool (NULL, "repeat")))
        {
            playlist_set_position (playlist, -1);
            hook_call ("playlist end reached", NULL);
        }
    }

    return FALSE;

STOP:
    /* stop playback and set position to beginning of song */
    playlist_set_playing (-1);
    playlist_set_position (playlist, playlist_get_position (playlist));
    return FALSE;
}

static bool_t open_file (void)
{
    /* no need to open a handle for custom URI schemes */
    if (current_decoder->schemes && current_decoder->schemes[0])
        return TRUE;

    if (current_file)
    {
        /* if possible, seek to beginning of file rather than reopening */
        if (vfs_fseek (current_file, 0, SEEK_SET) == 0)
            return TRUE;

        vfs_fclose (current_file);
    }

    current_file = vfs_fopen (current_filename, "r");
    return (current_file != NULL);
}

static void * playback_thread (void * unused)
{
    if (! current_decoder)
    {
        PluginHandle * p = playback_entry_get_decoder ();
        current_decoder = p ? plugin_get_header (p) : NULL;

        if (! current_decoder)
        {
            SPRINTF (error, _("No decoder found for %s."), current_filename);
            interface_show_error (error);
            playback_error = TRUE;
            goto DONE;
        }
    }

    Tuple * tuple = playback_entry_get_tuple ();
    read_gain_from_tuple (tuple);

    int end_time = -1;

    if (tuple && playback_entry_get_length () > 0)
    {
        if (tuple_get_value_type (tuple, FIELD_SEGMENT_START, NULL) == TUPLE_INT)
        {
            time_offset = tuple_get_int (tuple, FIELD_SEGMENT_START, NULL);
            start_time += time_offset;
        }

        if (repeat_b >= 0)
            end_time = time_offset + repeat_b;
        else if (tuple_get_value_type (tuple, FIELD_SEGMENT_END, NULL) == TUPLE_INT)
            end_time = tuple_get_int (tuple, FIELD_SEGMENT_END, NULL);
    }

    if (tuple)
        tuple_unref (tuple);

    if (! open_file ())
    {
        SPRINTF (error, _("%s could not be opened."), current_filename);
        interface_show_error (error);
        playback_error = TRUE;
        goto DONE;
    }

    stop_flag = FALSE;
    seek_request = -1;

    playback_error = ! current_decoder->play (& playback_api, current_filename,
     current_file, start_time, end_time, paused);

DONE:
    if (! ready_flag)
        set_ready ();

    end_source = g_timeout_add (0, end_cb, NULL);
    return NULL;
}

void playback_play (int seek_time, bool_t pause)
{
    char * new_filename = playback_entry_get_filename ();
    g_return_if_fail (new_filename);

    /* pointer comparison works for pooled strings */
    if (new_filename == current_filename)
    {
        if (playing)
            playback_finish ();

        str_unref (new_filename);
        restart_flag = TRUE;
    }
    else
    {
        if (current_filename)
            playback_cleanup ();

        current_filename = new_filename;
    }

    playing = TRUE;
    time_offset = 0;
    start_time = MAX (seek_time, 0);
    paused = pause;
    stopped = FALSE;

    hook_associate ("playlist update", update_cb, NULL);
    pthread_create (& playback_thread_handle, NULL, playback_thread, NULL);

    /* on restart, send "playback seek" instead of "playback begin" */
    if (restart_flag)
        hook_call ("playback seek", NULL);
    else
        hook_call ("playback begin", NULL);
}

bool_t drct_get_playing (void)
{
    return playing;
}

bool_t drct_get_paused (void)
{
    return paused;
}

static void default_seek (int time)
{
    pthread_mutex_lock (& control_mutex);
    seek_request = time;
    output_abort_write ();
    pthread_mutex_unlock (& control_mutex);
}

void drct_seek (int time)
{
    if (! playing)
        return;

    wait_until_ready ();

    if (current_length < 1)
        return;

    default_seek (time_offset + CLAMP (time, 0, current_length));
}

static bool_t open_audio_wrapper (int format, int rate, int channels)
{
    if (! output_open_audio (format, rate, channels))
        return FALSE;

    if (paused)
        output_pause (TRUE);

    if (start_time)
        output_set_time (start_time);

    return TRUE;
}

static void write_audio_wrapper (void * data, int length)
{
    if (! ready_flag)
        set_ready ();

    output_write_audio (data, length);
}

static void set_params (InputPlayback * p, int bitrate, int samplerate,
 int channels)
{
    g_return_if_fail (playing);

    current_bitrate = bitrate;
    current_samplerate = samplerate;
    current_channels = channels;

    if (drct_get_ready ())
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

static bool_t check_stop (void)
{
    pthread_mutex_lock (& control_mutex);
    bool_t stop = stop_flag;
    pthread_mutex_unlock (& control_mutex);
    return stop;
}

static int check_seek (void)
{
    pthread_mutex_lock (& control_mutex);
    int seek = seek_request;

    if (seek != -1)
    {
        output_set_time (seek);
        seek_request = -1;

        event_queue ("playback seek", NULL);
    }

    pthread_mutex_unlock (& control_mutex);
    return seek;
}

static InputPlayback playback_api = {
    .output = & output_api,
    .set_params = set_params,
    .set_tuple = set_tuple,
    .set_gain_from_playlist = set_gain_from_playlist,
    .check_stop = check_stop,
    .check_seek = check_seek
};

char * drct_get_filename (void)
{
    if (! playing)
        return NULL;

    return str_ref (current_filename);
}

char * drct_get_title (void)
{
    if (! playing)
        return NULL;

    wait_until_ready ();

    char s[32];

    if (current_length > 0)
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
        return str_printf ("%d. %s%s", 1 + current_entry, current_title, s);

    return str_printf ("%s%s", current_title, s);
}

int drct_get_length (void)
{
    if (playing)
        wait_until_ready ();

    return current_length;
}

void drct_get_info (int * bitrate, int * samplerate, int * channels)
{
    if (playing)
        wait_until_ready ();

    * bitrate = current_bitrate;
    * samplerate = current_samplerate;
    * channels = current_channels;
}

void drct_get_volume (int * l, int * r)
{
    output_get_volume (l, r);
}

void drct_set_volume (int l, int r)
{
    output_set_volume (CLAMP (l, 0, 100), CLAMP (r, 0, 100));
}

void drct_set_ab_repeat (int a, int b)
{
    if (! playing)
        return;

    wait_until_ready ();

    if (current_length < 1)
        return;

    repeat_a = a;

    if (repeat_b != b)
    {
        repeat_b = b;

        /* Restart playback so the new setting takes effect.  We could add
         * something like InputPlugin::set_stop_time(), but this is the only
         * place it would be used. */
        int seek_time = drct_get_time ();
        bool_t was_paused = paused;

        if (repeat_b >= 0 && seek_time >= repeat_b)
            seek_time = MAX (repeat_a, 0);

        playback_finish ();
        playback_play (seek_time, was_paused);
    }
}

void drct_get_ab_repeat (int * a, int * b)
{
    * a = playing ? repeat_a : -1;
    * b = playing ? repeat_b : -1;
}
