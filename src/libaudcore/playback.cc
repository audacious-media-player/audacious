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

#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "audstrings.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "mainloop.h"
#include "output.h"
#include "playlist-internal.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

static pthread_t playback_thread_handle;
static QueuedFunc end_queue;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;

/* level 1 data (persists to end of song) */
static bool playing = false;
static int time_offset = 0;
static int stop_time = -1;
static bool paused = false;
static bool ready_flag = false;
static bool playback_error = false;
static bool song_finished = false;

static int seek_request = -1; /* under control_mutex */
static int repeat_a = -1; /* under control_mutex */
static int repeat_b = -1; /* under control_mutex */

static bool stop_flag = false; /* atomic */

static int current_bitrate = -1, current_samplerate = -1, current_channels = -1;

static int current_entry = -1;
static String current_filename;
static String current_title;
static int current_length = -1;

static InputPlugin * current_decoder = nullptr;
static VFSFile current_file;
static ReplayGainInfo current_gain;

/* level 2 data (persists to end of playlist) */
static bool stopped = true;
static int failed_entries = 0;

/* clears gain info if tuple == nullptr */
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

static bool update_from_playlist (void)
{
    int entry = playback_entry_get_position ();
    String title = playback_entry_get_title ();
    int length = playback_entry_get_length ();

    if (entry == current_entry && title == current_title && length == current_length)
        return false;

    current_entry = entry;
    current_title = title;
    current_length = length;
    return true;
}

EXPORT bool aud_drct_get_ready (void)
{
    if (! playing)
        return false;

    pthread_mutex_lock (& ready_mutex);
    bool ready = ready_flag;
    pthread_mutex_unlock (& ready_mutex);
    return ready;
}

static void set_ready (void)
{
    assert (playing);

    pthread_mutex_lock (& ready_mutex);

    update_from_playlist ();
    event_queue ("playback ready", nullptr);
    ready_flag = true;

    pthread_cond_signal (& ready_cond);
    pthread_mutex_unlock (& ready_mutex);
}

static void wait_until_ready (void)
{
    assert (playing);
    pthread_mutex_lock (& ready_mutex);

    /* on restart, we still have to wait, but presumably not long */
    while (! ready_flag)
        pthread_cond_wait (& ready_cond, & ready_mutex);

    pthread_mutex_unlock (& ready_mutex);
}

static void update_cb (void * hook_data, void *)
{
    assert (playing);

    if (hook_data < PLAYLIST_UPDATE_METADATA || ! aud_drct_get_ready ())
        return;

    if (update_from_playlist ())
        event_queue ("title change", nullptr);
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
        hook_call ("playback pause", nullptr);
    else
        hook_call ("playback unpause", nullptr);
}

static void playback_cleanup (void)
{
    assert (playing);
    wait_until_ready ();

    if (! song_finished)
    {
        __sync_bool_compare_and_swap (& stop_flag, false, true);
        output_abort_write ();
    }

    pthread_join (playback_thread_handle, nullptr);
    output_close_audio ();

    hook_dissociate ("playlist update", update_cb);

    event_queue_cancel ("playback ready", nullptr);
    event_queue_cancel ("playback seek", nullptr);
    event_queue_cancel ("info change", nullptr);
    event_queue_cancel ("title change", nullptr);

    end_queue.stop ();

    /* level 1 data cleanup */
    playing = false;
    time_offset = 0;
    stop_time = -1;
    paused = false;
    ready_flag = false;
    playback_error = false;
    song_finished = false;

    seek_request = -1;
    repeat_a = -1;
    repeat_b = -1;
    stop_flag = false;

    current_bitrate = current_samplerate = current_channels = -1;

    current_entry = -1;
    current_filename = String ();
    current_title = String ();
    current_length = -1;

    current_decoder = nullptr;
    current_file = VFSFile ();

    read_gain_from_tuple (Tuple ());

    aud_set_bool (nullptr, "stop_after_current_song", false);
}

void playback_stop (void)
{
    if (stopped)
        return;

    if (playing)
        playback_cleanup ();

    output_drain ();

    /* level 2 data cleanup */
    stopped = true;
    failed_entries = 0;

    hook_call ("playback stop", nullptr);
}

static void do_stop (int playlist)
{
    aud_playlist_set_playing (-1);
    aud_playlist_set_position (playlist, aud_playlist_get_position (playlist));
}

static void do_next (int playlist)
{
    if (! playlist_next_song (playlist, aud_get_bool (nullptr, "repeat")))
    {
        aud_playlist_set_position (playlist, -1);
        hook_call ("playlist end reached", nullptr);
    }
}

static void end_cb (void * unused)
{
    if (! playing)
        return;

    if (! playback_error)
        song_finished = true;

    hook_call ("playback end", nullptr);

    if (playback_error)
        failed_entries ++;
    else
        failed_entries = 0;

    int playlist = aud_playlist_get_playing ();

    if (aud_get_bool (nullptr, "stop_after_current_song"))
    {
        do_stop (playlist);

        if (! aud_get_bool (nullptr, "no_playlist_advance"))
            do_next (playlist);
    }
    else if (aud_get_bool (nullptr, "no_playlist_advance"))
    {
        if (aud_get_bool (nullptr, "repeat") && ! failed_entries)
            playback_play (0, false);
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
}

static bool open_file (void)
{
    /* no need to open a handle for custom URI schemes */
    if (current_decoder->schemes && current_decoder->schemes[0])
        return true;

    current_file = VFSFile (current_filename, "r");
    return (bool) current_file;
}

static void * playback_thread (void *)
{
    Tuple tuple;
    int length;

    if (! current_decoder)
    {
        PluginHandle * p = playback_entry_get_decoder ();
        current_decoder = p ? (InputPlugin *) aud_plugin_get_header (p) : nullptr;

        if (! current_decoder)
        {
            aud_ui_show_error (str_printf (_("No decoder found for %s."),
             (const char *) current_filename));
            playback_error = true;
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
                seek_request = time_offset + aud::max (seek_request, 0);
        }

        if (tuple.get_value_type (FIELD_SEGMENT_END) == TUPLE_INT)
            stop_time = tuple.get_int (FIELD_SEGMENT_END);
    }

    read_gain_from_tuple (tuple);

    if (! open_file ())
    {
        aud_ui_show_error (str_printf (_("%s could not be opened."),
         (const char *) current_filename));
        playback_error = true;
        goto DONE;
    }

    playback_error = ! current_decoder->play (current_filename, current_file);

DONE:
    if (! ready_flag)
        set_ready ();

    end_queue.queue (end_cb, nullptr);
    return nullptr;
}

void playback_play (int seek_time, bool pause)
{
    String new_filename = playback_entry_get_filename ();

    if (! new_filename)
    {
        AUDWARN ("Nothing to play!");
        return;
    }

    if (playing)
        playback_cleanup ();

    current_filename = new_filename;

    playing = true;
    paused = pause;

    seek_request = (seek_time > 0) ? seek_time : -1;

    stopped = false;

    hook_associate ("playlist update", update_cb, nullptr);
    pthread_create (& playback_thread_handle, nullptr, playback_thread, nullptr);

    hook_call ("playback begin", nullptr);
}

EXPORT bool aud_drct_get_playing (void)
{
    return playing;
}

EXPORT bool aud_drct_get_paused (void)
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

    seek_request = time_offset + aud::clamp (time, 0, current_length);
    output_abort_write ();

    pthread_mutex_unlock (& control_mutex);
}

EXPORT bool aud_input_open_audio (int format, int rate, int channels)
{
    assert (playing);

    if (! output_open_audio (format, rate, channels))
        return false;

    output_set_replaygain_info (& current_gain);

    if (paused)
        output_pause (true);

    current_samplerate = rate;
    current_channels = channels;

    if (ready_flag)
        event_queue ("info change", nullptr);

    return true;
}

EXPORT void aud_input_set_gain (const ReplayGainInfo * info)
{
    assert (playing);
    memcpy (& current_gain, info, sizeof current_gain);
    output_set_replaygain_info (& current_gain);
}

EXPORT void aud_input_write_audio (void * data, int length)
{
    assert (playing);

    if (! ready_flag)
        set_ready ();

    pthread_mutex_lock (& control_mutex);
    int a = repeat_a, b = repeat_b;
    pthread_mutex_unlock (& control_mutex);

    if (b >= 0)
    {
        if (! output_write_audio (data, length, b))
        {
            pthread_mutex_lock (& control_mutex);
            seek_request = aud::max (a, time_offset);
            pthread_mutex_unlock (& control_mutex);
        }
    }
    else
    {
        if (! output_write_audio (data, length, stop_time))
            __sync_bool_compare_and_swap (& stop_flag, false, true);
    }
}

EXPORT int aud_input_written_time (void)
{
    assert (playing);
    return output_written_time ();
}

EXPORT Tuple aud_input_get_tuple (void)
{
    assert (playing);
    return playback_entry_get_tuple ();
}

EXPORT void aud_input_set_tuple (Tuple && tuple)
{
    assert (playing);
    playback_entry_set_tuple (std::move (tuple));
}

EXPORT void aud_input_set_bitrate (int bitrate)
{
    assert (playing);
    current_bitrate = bitrate;

    if (ready_flag)
        event_queue ("info change", nullptr);
}

EXPORT bool aud_input_check_stop (void)
{
    assert (playing);
    return __sync_fetch_and_add (& stop_flag, 0);
}

EXPORT int aud_input_check_seek (void)
{
    assert (playing);

    pthread_mutex_lock (& control_mutex);
    int seek = seek_request;

    if (seek != -1)
    {
        output_set_time (seek);
        seek_request = -1;

        event_queue ("playback seek", nullptr);
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

    StringBuf prefix = aud_get_bool (nullptr, "show_numbers_in_pl") ?
     str_printf ("%d. ", 1 + current_entry) : StringBuf (0);

    StringBuf time = (current_length > 0) ? str_format_time (current_length) : StringBuf ();
    StringBuf suffix = time ? str_concat ({" (", time, ")"}) : StringBuf (0);

    return String (str_concat ({prefix, current_title, suffix}));
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
    output_set_volume (aud::clamp (l, 0, 100), aud::clamp (r, 0, 100));
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
    repeat_b = b;

    if (b != -1 && output_get_time () >= b)
    {
        seek_request = aud::max (a, time_offset);
        output_abort_write ();
    }

    pthread_mutex_unlock (& control_mutex);
}

EXPORT void aud_drct_get_ab_repeat (int * a, int * b)
{
    * a = (playing && repeat_a != -1) ? repeat_a - time_offset : -1;
    * b = (playing && repeat_b != -1) ? repeat_b - time_offset : -1;
}
