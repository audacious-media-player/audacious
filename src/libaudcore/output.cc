/*
 * output.c
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
#include "output.h"
#include "runtime.h"

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "equalizer.h"
#include "internal.h"
#include "plugin.h"
#include "plugins.h"

static pthread_mutex_t mutex_major = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_minor = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_MAJOR pthread_mutex_lock (& mutex_major)
#define UNLOCK_MAJOR pthread_mutex_unlock (& mutex_major)
#define LOCK_MINOR pthread_mutex_lock (& mutex_minor)
#define UNLOCK_MINOR pthread_mutex_unlock (& mutex_minor)
#define LOCK_ALL do { LOCK_MAJOR; LOCK_MINOR; } while (0)
#define UNLOCK_ALL do { UNLOCK_MINOR; UNLOCK_MAJOR; } while (0)

/* State variables.  State changes that are allowed between LOCK_MINOR and
 * UNLOCK_MINOR (all others must take place between LOCK_ALL and UNLOCK_ALL):
 * s_paused -> true or false, s_flushed -> true, s_resetting -> true */

static bool s_input; /* input plugin connected */
static bool s_output; /* output plugin connected */
static bool s_gain; /* replay gain info set */
static bool s_paused; /* paused */
static bool s_flushed; /* flushed, writes ignored until resume */
static bool s_resetting; /* resetting output system */

/* Condition variable linked to LOCK_MINOR.
 * The input thread will wait if the following is true:
 *   ((! s_output || s_resetting) && ! s_flushed)
 * Hence you must signal if you cause the inverse to be true:
 *   ((s_output && ! s_resetting) || s_flushed) */

static pthread_cond_t cond_minor = PTHREAD_COND_INITIALIZER;

#define SIGNAL_MINOR pthread_cond_broadcast (& cond_minor)
#define WAIT_MINOR pthread_cond_wait (& cond_minor, & mutex_minor)

static OutputPlugin * cop;
static int seek_time;
static String in_filename;
static Tuple in_tuple;
static int in_format, in_channels, in_rate;
static int out_format, out_channels, out_rate;
static int out_bytes_per_sec, out_bytes_held;
static int64_t in_frames, out_bytes_written;
static ReplayGainInfo gain_info;

static bool change_op;
static OutputPlugin * new_op;

static Index<float> buffer1;
static Index<char> buffer2;

static inline int get_format ()
{
    switch (aud_get_int (0, "output_bit_depth"))
    {
        case 16: return FMT_S16_NE;
        case 24: return FMT_S24_NE;
        case 32: return FMT_S32_NE;
        default: return FMT_FLOAT;
    }
}

/* assumes LOCK_ALL, s_output */
static void cleanup_output ()
{
    if (! s_paused && ! s_flushed && ! s_resetting)
    {
        UNLOCK_MINOR;
        cop->drain ();
        LOCK_MINOR;
    }

    s_output = false;

    buffer1.clear ();
    buffer2.clear ();

    effect_flush (true);
    cop->close_audio ();
    vis_runner_start_stop (false, false);
}

/* assumes LOCK_ALL, s_output */
static void apply_pause ()
{
    cop->pause (s_paused);
    vis_runner_start_stop (true, s_paused);
}

/* assumes LOCK_ALL, s_input */
static void setup_output ()
{
    int format = get_format ();
    int channels = in_channels;
    int rate = in_rate;

    effect_start (channels, rate);
    eq_set_format (channels, rate);

    if (s_output && format == out_format && channels == out_channels && rate ==
     out_rate && ! cop->force_reopen)
    {
        AUDINFO ("Reusing output stream, format %d, %d channels, %d Hz.\n", format, channels, rate);
        return;
    }

    if (s_output)
        cleanup_output ();

    if (! cop)
        return;

    cop->set_info (in_filename, in_tuple);
    if (! cop->open_audio (format, rate, channels))
        return;

    AUDINFO ("Opened output stream, format %d, %d channels, %d Hz.\n", format, channels, rate);

    s_output = true;

    out_format = format;
    out_channels = channels;
    out_rate = rate;
    out_bytes_per_sec = FMT_SIZEOF (format) * channels * rate;
    out_bytes_held = 0;
    out_bytes_written = 0;

    apply_pause ();

    if (! s_flushed && ! s_resetting)
        SIGNAL_MINOR;
}

/* assumes LOCK_MINOR, s_output */
static bool flush_output (bool force)
{
    if (! effect_flush (force))
        return false;

    out_bytes_held = 0;
    out_bytes_written = 0;

    cop->flush ();
    vis_runner_flush ();
    return true;
}

static void apply_replay_gain (Index<float> & data)
{
    if (! aud_get_bool (0, "enable_replay_gain"))
        return;

    float factor = powf (10, aud_get_double (0, "replay_gain_preamp") / 20);

    if (s_gain)
    {
        float peak;

        if (aud_get_bool (0, "replay_gain_album"))
        {
            factor *= powf (10, gain_info.album_gain / 20);
            peak = gain_info.album_peak;
        }
        else
        {
            factor *= powf (10, gain_info.track_gain / 20);
            peak = gain_info.track_peak;
        }

        if (aud_get_bool (0, "enable_clipping_prevention") && peak * factor > 1)
            factor = 1 / peak;
    }
    else
        factor *= powf (10, aud_get_double (0, "default_gain") / 20);

    if (factor < 0.99 || factor > 1.01)
        audio_amplify (data.begin (), 1, data.len (), & factor);
}

/* assumes LOCK_ALL, s_output */
static void write_output_raw (Index<float> & data)
{
    if (! data.len ())
        return;

    int out_time = aud::rescale<int64_t> (out_bytes_written, out_bytes_per_sec, 1000);
    vis_runner_pass_audio (out_time, data, out_channels, out_rate);

    eq_filter (data.begin (), data.len ());

    if (aud_get_bool (0, "software_volume_control"))
    {
        StereoVolume v = {aud_get_int (0, "sw_volume_left"), aud_get_int (0, "sw_volume_right")};
        audio_amplify (data.begin (), out_channels, data.len () / out_channels, v);
    }

    if (aud_get_bool (0, "soft_clipping"))
        audio_soft_clip (data.begin (), data.len ());

    const void * out_data = data.begin ();

    if (out_format != FMT_FLOAT)
    {
        buffer2.resize (FMT_SIZEOF (out_format) * data.len ());
        audio_to_int (data.begin (), buffer2.begin (), out_format, data.len ());
        out_data = buffer2.begin ();
    }

    out_bytes_held = FMT_SIZEOF (out_format) * data.len ();

    while (! s_flushed && ! s_resetting)
    {
        int written = cop->write_audio (out_data, out_bytes_held);

        out_data = (char *) out_data + written;
        out_bytes_held -= written;
        out_bytes_written += written;

        if (! out_bytes_held)
            break;

        UNLOCK_MINOR;
        cop->period_wait ();
        LOCK_MINOR;
    }
}

/* assumes LOCK_ALL, s_input, s_output */
static bool write_output (const void * data, int size, int stop_time)
{
    int samples = size / FMT_SIZEOF (in_format);
    bool stopped = false;

    if (stop_time != -1)
    {
        int64_t frames_left = aud::rescale<int64_t> (stop_time - seek_time, 1000, in_rate) - in_frames;
        int64_t samples_left = in_channels * aud::max ((int64_t) 0, frames_left);

        if (samples >= samples_left)
        {
            samples = samples_left;
            stopped = true;
        }
    }

    in_frames += samples / in_channels;

    buffer1.resize (samples);

    if (in_format == FMT_FLOAT)
        memcpy (buffer1.begin (), data, sizeof (float) * samples);
    else
        audio_from_int (data, in_format, buffer1.begin (), samples);

    apply_replay_gain (buffer1);
    write_output_raw (effect_process (buffer1));

    return ! stopped;
}

/* assumes LOCK_ALL, s_output */
static void finish_effects (bool end_of_playlist)
{
    buffer1.resize (0);
    write_output_raw (effect_finish (buffer1, end_of_playlist));
}

bool output_open_audio (const String & filename, const Tuple & tuple,
 int format, int rate, int channels, int start_time)
{
    /* prevent division by zero */
    if (rate < 1 || channels < 1 || channels > AUD_MAX_CHANNELS)
        return false;

    LOCK_ALL;

    if (s_output && s_paused)
    {
        if (! s_flushed && ! s_resetting)
            flush_output (true);

        s_paused = false;
        apply_pause ();
    }

    s_input = true;
    s_gain = s_paused = s_flushed = false;
    seek_time = start_time;

    in_filename = filename;
    in_tuple = tuple.ref ();
    in_format = format;
    in_channels = channels;
    in_rate = rate;
    in_frames = 0;

    setup_output ();

    UNLOCK_ALL;
    return true;
}

void output_set_replay_gain (const ReplayGainInfo & info)
{
    LOCK_ALL;

    if (s_input)
    {
        gain_info = info;
        s_gain = true;

        AUDINFO ("Replay Gain info:\n");
        AUDINFO (" album gain: %f dB\n", info.album_gain);
        AUDINFO (" album peak: %f\n", info.album_peak);
        AUDINFO (" track gain: %f dB\n", info.track_gain);
        AUDINFO (" track peak: %f\n", info.track_peak);
    }

    UNLOCK_ALL;
}

/* returns false if stop_time is reached */
bool output_write_audio (const void * data, int size, int stop_time)
{
RETRY:
    LOCK_ALL;
    bool good = false;

    if (s_input && ! s_flushed)
    {
        if (! s_output || s_resetting)
        {
            UNLOCK_MAJOR;
            WAIT_MINOR;
            UNLOCK_MINOR;
            goto RETRY;
        }

        good = write_output (data, size, stop_time);
    }

    UNLOCK_ALL;
    return good;
}

void output_flush (int time, bool force)
{
    LOCK_MINOR;

    if (s_input && ! s_flushed)
    {
        if (s_output && ! s_resetting)
        {
            // allow effect plugins to prevent the flush, but
            // always flush if paused to prevent locking up
            s_flushed = flush_output (s_paused || force);
        }
        else
        {
            s_flushed = true;
            SIGNAL_MINOR;
        }
    }

    if (s_input)
    {
        seek_time = time;
        in_frames = 0;
    }

    UNLOCK_MINOR;
}

void output_resume ()
{
    LOCK_ALL;

    if (s_input)
        s_flushed = false;

    UNLOCK_ALL;
}

void output_pause (bool pause)
{
    LOCK_MINOR;

    if (s_input)
    {
        s_paused = pause;

        if (s_output)
            apply_pause ();
    }

    UNLOCK_MINOR;
}

int output_get_time ()
{
    LOCK_MINOR;
    int time = 0, delay = 0;

    if (s_input)
    {
        if (s_output)
        {
            delay = cop->get_delay ();
            delay += aud::rescale<int64_t> (out_bytes_held, out_bytes_per_sec, 1000);
        }

        delay = effect_adjust_delay (delay);
        time = aud::rescale<int64_t> (in_frames, in_rate, 1000);
        time = seek_time + aud::max (time - delay, 0);
    }

    UNLOCK_MINOR;
    return time;
}

int output_get_raw_time ()
{
    LOCK_MINOR;
    int time = 0;

    if (s_output)
    {
        time = aud::rescale<int64_t> (out_bytes_written, out_bytes_per_sec, 1000);
        time = aud::max (time - cop->get_delay (), 0);
    }

    UNLOCK_MINOR;
    return time;
}

void output_close_audio ()
{
    LOCK_ALL;

    if (s_input)
    {
        s_input = false;
        in_filename = String ();
        in_tuple = Tuple ();

        if (s_output && ! (s_paused || s_flushed || s_resetting))
            finish_effects (false); /* first time for end of song */
    }

    UNLOCK_ALL;
}

void output_drain ()
{
    LOCK_ALL;

    if (! s_input && s_output)
    {
        finish_effects (true); /* second time for end of playlist */
        cleanup_output ();
    }

    UNLOCK_ALL;
}

EXPORT void aud_output_reset (OutputReset type)
{
    LOCK_MINOR;

    s_resetting = true;

    if (s_output && ! s_flushed)
        flush_output (true);

    UNLOCK_MINOR;
    LOCK_ALL;

    if (s_output && type != OutputReset::EffectsOnly)
        cleanup_output ();

    if (type == OutputReset::ResetPlugin)
    {
        if (cop)
            cop->cleanup ();

        if (change_op)
            cop = new_op;

        if (cop && ! cop->init ())
            cop = nullptr;
    }

    if (s_input)
        setup_output ();

    s_resetting = false;

    if (s_output && ! s_flushed)
        SIGNAL_MINOR;

    UNLOCK_ALL;
}

EXPORT StereoVolume aud_drct_get_volume ()
{
    StereoVolume volume = {0, 0};
    LOCK_MINOR;

    if (aud_get_bool (0, "software_volume_control"))
        volume = {aud_get_int (0, "sw_volume_left"), aud_get_int (0, "sw_volume_right")};
    else if (cop)
        volume = cop->get_volume ();

    UNLOCK_MINOR;
    return volume;
}

EXPORT void aud_drct_set_volume (StereoVolume volume)
{
    LOCK_MINOR;

    volume.left = aud::clamp (volume.left, 0, 100);
    volume.right = aud::clamp (volume.right, 0, 100);

    if (aud_get_bool (0, "software_volume_control"))
    {
        aud_set_int (0, "sw_volume_left", volume.left);
        aud_set_int (0, "sw_volume_right", volume.right);
    }
    else if (cop)
        cop->set_volume (volume);

    UNLOCK_MINOR;
}

PluginHandle * output_plugin_get_current ()
{
    return cop ? aud_plugin_by_header (cop) : nullptr;
}

bool output_plugin_set_current (PluginHandle * plugin)
{
    change_op = true;
    new_op = plugin ? (OutputPlugin *) aud_plugin_get_header (plugin) : nullptr;
    aud_output_reset (OutputReset::ResetPlugin);

    bool success = (! plugin || (new_op && cop == new_op));
    change_op = false;
    new_op = nullptr;

    return success;
}
