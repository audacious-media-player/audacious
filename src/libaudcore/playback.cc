/*
 * playback.c
 * Copyright 2009-2013 John Lindgren
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

#include "drct.h"
#include "input.h"
#include "internal.h"

#include <glib.h>
#include <pthread.h>
#include <string.h>

#include "audstrings.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "output.h"
#include "playlist-internal.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

static pthread_t playback_thread_handle;
static int end_source = 0;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;

/* level 1 data (persists to end of song) */
static bool_t playing = FALSE;
static int time_offset = 0;
static int stop_time = -1;
static bool_t paused = FALSE;
static bool_t ready_flag = FALSE;
static bool_t playback_error = FALSE;
static bool_t song_finished = FALSE;

static int seek_request = -1; /* under control_mutex */
static int repeat_a = -1; /* under control_mutex */

static volatile int repeat_b = -1; /* atomic */
static volatile int stop_flag = FALSE; /* atomic */

static int current_bitrate = -1, current_samplerate = -1, current_channels = -1;

static int current_entry = -1;
static String current_filename;
static String current_title;
static int current_length = -1;

static InputPlugin * current_decoder = NULL;
static VFSFile * current_file = NULL;
static ReplayGainInfo current_gain;

/* level 2 data (persists to end of playlist) */
static bool_t stopped = TRUE;
static int failed_entries = 0;

/* clears gain info if tuple == NULL */
static void read_gain_from_tuple (const Tuple & tuple)
{
    memset (& current_gain, 0, sizeof current_gain);

    if (! tuple)
        return;

    int album_gain = tuple.get_int (FIELD_GAIN_ALBUM_GAIN);
    int album_peak = tuple.get_int (FIELD_GAIN_ALBUM_PEAK);
    int track_gain = tuple.get_int (FIELD_GAIN_TRACK_GAIN);
    int track_peak = tuple.get_int (FIELD_GAIN_TRACK_PEAK);
    int gain_unit = tuple.get_int (FIELD_GAIN_GAIN_UNIT);
    int peak_unit = tuple.get_int (FIELD_GAIN_PEAK_UNIT);

    if (gain_unit)
    {
        current_gain.album_gain = album_gain / (float) gain_unit;
        current_gain.track_gain = track_gain / (float) gain_unit;
    }

    if (peak_unit)
    {
        current_gain.album_peak = album_peak / (float) peak_unit;
        current_gain.track_peak = track_peak / (float) peak_unit;
    }
}

static bool_t update_from_playlist (void)
{
    int entry = playback_entry_get_position ();
    String title = playback_entry_get_title ();
    int length = playback_entry_get_length ();

    if (entry == current_entry && str_equal (title, current_title) && length == current_length)
        return FALSE;

    current_entry = entry;
    current_title = title;
    current_length = length;
    return TRUE;
}

EXPORT bool_t aud_drct_get_ready (void)
{
    if (! playing)
        return FALSE;

    pthread_mutex_lock (& ready_mutex);
    bool_t ready = ready_flag;
    pthread_mutex_unlock (& ready_mutex);
    return ready;
}

static void set_ready (void)
{
    g_return_if_fail (playing);

    pthread_mutex_lock (& ready_mutex);

    update_from_playlist ();
    event_queue ("playback ready", NULL);
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

    if (GPOINTER_TO_INT (hook_data) < PLAYLIST_UPDATE_METADATA || ! aud_drct_get_ready ())
        return;

    if (update_from_playlist ())
        event_queue ("title change", NULL);
}

EXPORT int aud_drct_get_time (void)
{
    if (! playing)
        return 0;

    wait_until_ready ();

    return output_get_time () - time_offset;
}

EXPORT void aud_drct_pause (void)
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

static void playback_cleanup (void)
{
    g_return_if_fail (playing);
    wait_until_ready ();

    if (! song_finished)
    {
        g_atomic_int_set (& stop_flag, TRUE);
        output_abort_write ();
    }

    pthread_join (playback_thread_handle, NULL);
    output_close_audio ();

    hook_dissociate ("playlist update", update_cb);

    event_queue_cancel ("playback ready", NULL);
    event_queue_cancel ("playback seek", NULL);
    event_queue_cancel ("info change", NULL);
    event_queue_cancel ("title change", NULL);

    if (end_source)
    {
        g_source_remove (end_source);
        end_source = 0;
    }

    /* level 1 data cleanup */
    playing = FALSE;
    time_offset = 0;
    stop_time = -1;
    paused = FALSE;
    ready_flag = FALSE;
    playback_error = FALSE;
    song_finished = FALSE;

    seek_request = -1;
    repeat_a = -1;

    g_atomic_int_set (& repeat_b, -1);
    g_atomic_int_set (& stop_flag, FALSE);

    current_bitrate = current_samplerate = current_channels = -1;

    current_entry = -1;
    current_filename = String ();
    current_title = String ();
    current_length = -1;

    current_decoder = NULL;

    if (current_file)
    {
        vfs_fclose (current_file);
        current_file = NULL;
    }

    read_gain_from_tuple (Tuple ());

    aud_set_bool (NULL, "stop_after_current_song", FALSE);
}

void playback_stop (void)
{
    if (stopped)
        return;

    if (playing)
        playback_cleanup ();

    output_drain ();

    /* level 2 data cleanup */
    stopped = TRUE;
    failed_entries = 0;

    hook_call ("playback stop", NULL);
}

static void do_stop (int playlist)
{
    aud_playlist_set_playing (-1);
    aud_playlist_set_position (playlist, aud_playlist_get_position (playlist));
}

static void do_next (int playlist)
{
    if (! playlist_next_song (playlist, aud_get_bool (NULL, "repeat")))
    {
        aud_playlist_set_position (playlist, -1);
        hook_call ("playlist end reached", NULL);
    }
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

    int playlist = aud_playlist_get_playing ();

    if (aud_get_bool (NULL, "stop_after_current_song"))
    {
        do_stop (playlist);

        if (! aud_get_bool (NULL, "no_playlist_advance"))
            do_next (playlist);
    }
    else if (aud_get_bool (NULL, "no_playlist_advance"))
    {
        if (aud_get_bool (NULL, "repeat") && ! failed_entries)
            playback_play (0, FALSE);
        else
            do_stop (playlist);
    }
    else
    {
        if (failed_entries < 10)
            do_next (playlist);
        else
            do_stop (playlist);
    }

    return FALSE;
}

static bool_t open_file (void)
{
    /* no need to open a handle for custom URI schemes */
    if (current_decoder->schemes && current_decoder->schemes[0])
        return TRUE;

    current_file = vfs_fopen (current_filename, "r");
    return (current_file != NULL);
}

static void * playback_thread (void * unused)
{
    Tuple tuple;
    int length;

    if (! current_decoder)
    {
        PluginHandle * p = playback_entry_get_decoder ();
        current_decoder = p ? (InputPlugin *) aud_plugin_get_header (p) : NULL;

        if (! current_decoder)
        {
            SPRINTF (error, _("No decoder found for %s."), (const char *) current_filename);
            aud_ui_show_error (error);
            playback_error = TRUE;
            goto DONE;
        }
    }

    tuple = playback_entry_get_tuple ();
    length = playback_entry_get_length ();

    if (length < 1)
        seek_request = -1;

    if (tuple && length > 0)
    {
        if (tuple.get_value_type (FIELD_SEGMENT_START) == TUPLE_INT)
        {
            time_offset = tuple.get_int (FIELD_SEGMENT_START);
            if (time_offset)
                seek_request = time_offset + MAX (seek_request, 0);
        }

        if (tuple.get_value_type (FIELD_SEGMENT_END) == TUPLE_INT)
            stop_time = tuple.get_int (FIELD_SEGMENT_END);
    }

    read_gain_from_tuple (tuple);

    if (! open_file ())
    {
        SPRINTF (error, _("%s could not be opened."), (const char *) current_filename);
        aud_ui_show_error (error);
        playback_error = TRUE;
        goto DONE;
    }

    playback_error = ! current_decoder->play (current_filename, current_file);

DONE:
    if (! ready_flag)
        set_ready ();

    end_source = g_timeout_add (0, end_cb, NULL);
    return NULL;
}

void playback_play (int seek_time, bool_t pause)
{
    String new_filename = playback_entry_get_filename ();
    g_return_if_fail (new_filename);

    if (playing)
        playback_cleanup ();

    current_filename = new_filename;

    playing = TRUE;
    paused = pause;

    seek_request = (seek_time > 0) ? seek_time : -1;

    stopped = FALSE;

    hook_associate ("playlist update", update_cb, NULL);
    pthread_create (& playback_thread_handle, NULL, playback_thread, NULL);

    hook_call ("playback begin", NULL);
}

EXPORT bool_t aud_drct_get_playing (void)
{
    return playing;
}

EXPORT bool_t aud_drct_get_paused (void)
{
    return paused;
}

EXPORT void aud_drct_seek (int time)
{
    if (! playing)
        return;

    wait_until_ready ();

    if (current_length < 1)
        return;

    pthread_mutex_lock (& control_mutex);

    seek_request = time_offset + CLAMP (time, 0, current_length);
    output_abort_write ();

    pthread_mutex_unlock (& control_mutex);
}

EXPORT bool_t aud_input_open_audio (int format, int rate, int channels)
{
    g_return_val_if_fail (playing, FALSE);

    if (! output_open_audio (format, rate, channels))
        return FALSE;

    output_set_replaygain_info (& current_gain);

    if (paused)
        output_pause (TRUE);

    current_samplerate = rate;
    current_channels = channels;

    if (ready_flag)
        event_queue ("info change", NULL);

    return TRUE;
}

EXPORT void aud_input_set_gain (const ReplayGainInfo * info)
{
    g_return_if_fail (playing);
    memcpy (& current_gain, info, sizeof current_gain);
    output_set_replaygain_info (& current_gain);
}

EXPORT void aud_input_write_audio (void * data, int length)
{
    g_return_if_fail (playing);

    if (! ready_flag)
        set_ready ();

    int b = g_atomic_int_get (& repeat_b);

    if (b >= 0)
    {
        if (! output_write_audio (data, length, b))
        {
            pthread_mutex_lock (& control_mutex);
            seek_request = MAX (repeat_a, time_offset);
            pthread_mutex_unlock (& control_mutex);
        }
    }
    else
    {
        if (! output_write_audio (data, length, stop_time))
            g_atomic_int_set (& stop_flag, TRUE);
    }
}

EXPORT int aud_input_written_time (void)
{
    g_return_val_if_fail (playing, -1);
    return output_written_time ();
}

EXPORT Tuple aud_input_get_tuple (void)
{
    g_return_val_if_fail (playing, Tuple ());
    return playback_entry_get_tuple ();
}

EXPORT void aud_input_set_tuple (Tuple && tuple)
{
    g_return_if_fail (playing);
    playback_entry_set_tuple (std::move (tuple));
}

EXPORT void aud_input_set_bitrate (int bitrate)
{
    g_return_if_fail (playing);
    current_bitrate = bitrate;

    if (ready_flag)
        event_queue ("info change", NULL);
}

EXPORT bool_t aud_input_check_stop (void)
{
    g_return_val_if_fail (playing, TRUE);
    return g_atomic_int_get (& stop_flag);
}

EXPORT int aud_input_check_seek (void)
{
    g_return_val_if_fail (playing, -1);

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

EXPORT String aud_drct_get_filename (void)
{
    if (! playing)
        return String ();

    return current_filename;
}

EXPORT String aud_drct_get_title (void)
{
    if (! playing)
        return String ();

    wait_until_ready ();

    char s[32];

    if (current_length > 0)
    {
        char t[16];
        str_format_time (t, sizeof t, current_length);
        snprintf (s, sizeof s, " (%s)", t);
    }
    else
        s[0] = 0;

    if (aud_get_bool (NULL, "show_numbers_in_pl"))
        return str_printf ("%d. %s%s", 1 + current_entry, (const char *) current_title, s);

    return str_printf ("%s%s", (const char *) current_title, s);
}

EXPORT int aud_drct_get_length (void)
{
    if (playing)
        wait_until_ready ();

    return current_length;
}

EXPORT void aud_drct_get_info (int * bitrate, int * samplerate, int * channels)
{
    if (playing)
        wait_until_ready ();

    * bitrate = current_bitrate;
    * samplerate = current_samplerate;
    * channels = current_channels;
}

EXPORT void aud_drct_get_volume (int * l, int * r)
{
    output_get_volume (l, r);
}

EXPORT void aud_drct_set_volume (int l, int r)
{
    output_set_volume (CLAMP (l, 0, 100), CLAMP (r, 0, 100));
}

EXPORT void aud_drct_set_ab_repeat (int a, int b)
{
    if (! playing)
        return;

    wait_until_ready ();

    if (current_length < 1)
        return;

    if (a >= 0)
        a += time_offset;
    if (b >= 0)
        b += time_offset;

    pthread_mutex_lock (& control_mutex);

    repeat_a = a;
    g_atomic_int_set (& repeat_b, b);

    if (b != -1 && output_get_time () >= b)
    {
        seek_request = MAX (a, time_offset);
        output_abort_write ();
    }

    pthread_mutex_unlock (& control_mutex);
}

EXPORT void aud_drct_get_ab_repeat (int * a, int * b)
{
    * a = (playing && repeat_a != -1) ? repeat_a - time_offset : -1;
    * b = (playing && repeat_b != -1) ? repeat_b - time_offset : -1;
}
