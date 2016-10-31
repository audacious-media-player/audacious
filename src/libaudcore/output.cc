/*
 * output.c
 * Copyright 2009-2015 John Lindgren
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

#include "output.h"

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "equalizer.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "internal.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

/* With Audacious 3.7, there is some support for secondary output plugins.
 * Notes and limitations:
 *  - Only one secondary output can be in use at a time.
 *  - A reduced API is used, consisting of only open_audio(), close_audio(), and
 *    write_audio().
 *  - The primary and secondary outputs are run from the same thread, with
 *    timing controlled by the primary's period_wait().  To avoid dropouts in
 *    the primary output, the secondary's write_audio() must be able to process
 *    audio faster than realtime.
 *  - The secondary's write_audio() is called in a tight loop until it has
 *    caught up to the primary, and should never return a zero byte count. */

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
 * s_paused -> true or false, s_flushed -> true, s_resetting -> true,
 * s_secondary -> true or false */

static bool s_input; /* input plugin connected */
static bool s_output; /* primary output plugin connected */
static bool s_secondary; /* secondary output plugin connected */
static bool s_gain; /* replay gain info set */
static bool s_paused; /* paused */
static bool s_flushed; /* flushed, writes ignored until resume */
static bool s_resetting; /* resetting output system */

/* Condition variable linked to LOCK_MINOR.
 * The input thread will wait if the following is true:
 *   ((! s_output || s_paused || s_resetting) && ! s_flushed)
 * Hence you must signal if you cause the inverse to be true:
 *   ((s_output && ! s_paused && ! s_resetting) || s_flushed) */

static pthread_cond_t cond_minor = PTHREAD_COND_INITIALIZER;

#define SIGNAL_MINOR pthread_cond_broadcast (& cond_minor)
#define WAIT_MINOR pthread_cond_wait (& cond_minor, & mutex_minor)

static OutputPlugin * cop; /* current (primary) output plugin */
static OutputPlugin * sop; /* secondary output plugin */

static OutputStream record_stream;

static int seek_time;
static String in_filename;
static Tuple in_tuple;
static int in_format, in_channels, in_rate;
static int effect_channels, effect_rate;
static int sec_channels, sec_rate;
static int out_format, out_channels, out_rate;
static int out_bytes_per_sec, out_bytes_held;
static int64_t in_frames, out_bytes_written;
static ReplayGainInfo gain_info;

static Index<float> buffer1;
static Index<char> buffer2;

static inline int get_format (bool & automatic)
{
    automatic = false;

    switch (aud_get_int (0, "output_bit_depth"))
    {
        case 16: return FMT_S16_NE;
        case 24: return FMT_S24_3NE;
        case 32: return FMT_S32_NE;

        // return FMT_FLOAT for "auto" as well
        case -1: automatic = true;
        default: return FMT_FLOAT;
    }
}

/* assumes LOCK_ALL, s_input */
static void setup_effects ()
{
    effect_channels = in_channels;
    effect_rate = in_rate;

    effect_start (effect_channels, effect_rate);
    eq_set_format (effect_channels, effect_rate);
}

/* assumes LOCK_ALL */
static void cleanup_output ()
{
    if (! s_output)
        return;

    if (! s_paused && ! s_flushed && ! s_resetting)
    {
        UNLOCK_MINOR;
        cop->drain ();
        LOCK_MINOR;
    }

    s_output = false;

    buffer1.clear ();
    buffer2.clear ();

    cop->close_audio ();
    vis_runner_start_stop (false, false);
}

/* assumes LOCK_MINOR */
static void cleanup_secondary ()
{
    if (! s_secondary)
        return;

    s_secondary = false;
    sop->close_audio ();
}

/* assumes LOCK_MINOR, s_output */
static void apply_pause ()
{
    cop->pause (s_paused);
    vis_runner_start_stop (true, s_paused);
}

/* assumes LOCK_ALL, s_input */
static void setup_output (bool new_input)
{
    if (! cop)
        return;

    bool automatic;
    int format = get_format (automatic);

    AUDINFO ("Setup output, format %d, %d channels, %d Hz.\n", format, effect_channels, effect_rate);

    if (s_output && format == out_format && effect_channels == out_channels &&
     effect_rate == out_rate && ! (new_input && cop->force_reopen))
        return;

    cleanup_output ();
    cop->set_info (in_filename, in_tuple);

    String error;
    while (! cop->open_audio (format, effect_rate, effect_channels, error))
    {
        if (automatic && format == FMT_FLOAT)
            format = FMT_S32_NE;
        else if (automatic && format == FMT_S32_NE)
            format = FMT_S16_NE;
        else if (format == FMT_S24_3NE)
            format = FMT_S24_NE; /* some output plugins support only padded 24-bit */
        else
        {
            aud_ui_show_error (error ? (const char *) error : _("Error opening output stream"));
            return;
        }

        AUDINFO ("Falling back to format %d.\n", format);
    }

    s_output = true;

    out_format = format;
    out_channels = effect_channels;
    out_rate = effect_rate;

    out_bytes_per_sec = FMT_SIZEOF (format) * out_channels * out_rate;
    out_bytes_held = 0;
    out_bytes_written = 0;

    apply_pause ();

    if (! s_paused && ! s_flushed && ! s_resetting)
        SIGNAL_MINOR;
}

/* assumes LOCK_MINOR, s_input */
static void setup_secondary (bool new_input)
{
    if (! sop)
        return;

    int rate, channels;
    record_stream = (OutputStream) aud_get_int (0, "record_stream");

    if (record_stream < OutputStream::AfterEffects)
    {
        rate = in_rate;
        channels = in_channels;
    }
    else
    {
        rate = effect_rate;
        channels = effect_channels;
    }

    if (s_secondary && channels == sec_channels && rate == sec_rate &&
     ! (new_input && sop->force_reopen))
        return;

    cleanup_secondary ();
    sop->set_info (in_filename, in_tuple);

    String error;
    if (! sop->open_audio (FMT_FLOAT, rate, channels, error))
    {
        aud_ui_show_error (error ? (const char *) error : _("Error opening output stream"));
        return;
    }

    s_secondary = true;

    sec_channels = channels;
    sec_rate = rate;
}

/* assumes LOCK_MINOR, s_output */
static void flush_output ()
{
    out_bytes_held = 0;
    out_bytes_written = 0;

    cop->flush ();
    vis_runner_flush ();
}

static void apply_replay_gain (Index<float> & data)
{
    if (! aud_get_bool (0, "enable_replay_gain"))
        return;

    float factor = powf (10, aud_get_double (0, "replay_gain_preamp") / 20);

    if (s_gain)
    {
        float peak;

        auto mode = (ReplayGainMode) aud_get_int (0, "replay_gain_mode");
        if ((mode == ReplayGainMode::Album) ||
            (mode == ReplayGainMode::Automatic &&
             (! aud_get_bool (0, "shuffle") || aud_get_bool (0, "album_shuffle"))))
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

/* assumes LOCK_MINOR, s_secondary */
static void write_secondary (const Index<float> & data)
{
    auto begin = (const char *) data.begin ();
    auto end = (const char *) data.end ();

    while (begin < end)
        begin += sop->write_audio (begin, end - begin);
}

/* assumes LOCK_ALL, s_output */
static void write_output (Index<float> & data)
{
    if (! data.len ())
        return;

    if (s_secondary && record_stream == OutputStream::AfterEffects)
        write_secondary (data);

    int out_time = aud::rescale<int64_t> (out_bytes_written, out_bytes_per_sec, 1000);
    vis_runner_pass_audio (out_time, data, out_channels, out_rate);

    eq_filter (data.begin (), data.len ());

    if (s_secondary && record_stream == OutputStream::AfterEqualizer)
        write_secondary (data);

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

    while (! s_paused && ! s_flushed && ! s_resetting)
    {
        int written = cop->write_audio (out_data, out_bytes_held);

        out_data = (const char *) out_data + written;
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
static bool process_audio (const void * data, int size, int stop_time)
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

    if (s_secondary && record_stream == OutputStream::AsDecoded)
        write_secondary (buffer1);

    apply_replay_gain (buffer1);

    if (s_secondary && record_stream == OutputStream::AfterReplayGain)
        write_secondary (buffer1);

    write_output (effect_process (buffer1));

    return ! stopped;
}

/* assumes LOCK_ALL, s_output */
static void finish_effects (bool end_of_playlist)
{
    buffer1.resize (0);
    write_output (effect_finish (buffer1, end_of_playlist));
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
        effect_flush (true);
        cleanup_output ();
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

    setup_effects ();
    setup_output (true);
    setup_secondary (true);

    UNLOCK_ALL;
    return true;
}

void output_set_tuple (const Tuple & tuple)
{
    LOCK_ALL;

    if (s_input)
        in_tuple = tuple.ref ();

    UNLOCK_ALL;
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
        if (! s_output || s_paused || s_resetting)
        {
            UNLOCK_MAJOR;
            WAIT_MINOR;
            UNLOCK_MINOR;
            goto RETRY;
        }

        good = process_audio (data, size, stop_time);
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
            if (effect_flush (s_paused || force))
            {
                flush_output ();
                s_flushed = true;
                if (s_paused)
                    SIGNAL_MINOR;
            }
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

    if (s_input && s_paused != pause)
    {
        s_paused = pause;

        if (s_output)
        {
            apply_pause ();
            if (! s_paused && ! s_flushed && ! s_resetting)
                SIGNAL_MINOR;
        }
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

    if (! s_input)
    {
        if (s_output)
            finish_effects (true); /* second time for end of playlist */

        cleanup_output ();
        cleanup_secondary ();
    }

    UNLOCK_ALL;
}

static void output_reset (OutputReset type, OutputPlugin * op)
{
    LOCK_MINOR;

    s_resetting = true;

    if (s_output && ! s_flushed)
        flush_output ();

    UNLOCK_MINOR;
    LOCK_ALL;

    if (type != OutputReset::EffectsOnly)
        cleanup_output ();

    /* this does not reset the secondary plugin */
    if (type == OutputReset::ResetPlugin)
    {
        if (cop)
            cop->cleanup ();

        if (op)
        {
            /* secondary plugin may become primary */
            if (op == sop)
            {
                cleanup_secondary ();
                sop = nullptr;
            }
            else if (! op->init ())
                op = nullptr;
        }

        cop = op;
    }

    if (s_input)
    {
        if (type == OutputReset::EffectsOnly)
            setup_effects ();

        setup_output (false);
        setup_secondary (false);
    }

    s_resetting = false;

    if (s_output && ! s_paused && ! s_flushed)
        SIGNAL_MINOR;

    UNLOCK_ALL;
}

EXPORT void aud_output_reset (OutputReset type)
{
    output_reset (type, cop);
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

PluginHandle * output_plugin_get_secondary ()
{
    return sop ? aud_plugin_by_header (sop) : nullptr;
}

bool output_plugin_set_current (PluginHandle * plugin)
{
    output_reset (OutputReset::ResetPlugin, plugin ?
     (OutputPlugin *) aud_plugin_get_header (plugin) : nullptr);
    return (! plugin || cop);
}

bool output_plugin_set_secondary (PluginHandle * plugin)
{
    LOCK_MINOR;

    cleanup_secondary ();
    if (sop)
        sop->cleanup ();

    sop = plugin ? (OutputPlugin *) aud_plugin_get_header (plugin) : nullptr;
    if (sop && ! sop->init ())
        sop = nullptr;

    if (s_input)
        setup_secondary (false);

    UNLOCK_MINOR;
    return (! plugin || sop);
}

static void record_stream_changed (void *, void *)
{
    LOCK_MINOR;

    if (s_input)
        setup_secondary (false);

    UNLOCK_MINOR;
}

void output_init ()
{
    hook_associate ("set record_stream", record_stream_changed, nullptr);
}

void output_cleanup ()
{
    hook_dissociate ("set record_stream", record_stream_changed, nullptr);
}
