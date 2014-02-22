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

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h> /* for g_usleep */

#include "debug.h"
#include "effect.h"
#include "equalizer.h"
#include "misc.h"
#include "output.h"
#include "plugin.h"
#include "plugins.h"
#include "vis_runner.h"

#define SW_VOLUME_RANGE 40 /* decibels */

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
 * s_paused -> TRUE or FALSE, s_aborted -> TRUE, s_resetting -> TRUE */

static bool_t s_input; /* input plugin connected */
static bool_t s_output; /* output plugin connected */
static bool_t s_gain; /* replay gain info set */
static bool_t s_paused; /* paused */
static bool_t s_aborted; /* writes aborted */
static bool_t s_resetting; /* resetting output system */

static OutputPlugin * cop;
static int seek_time;
static int in_format, in_channels, in_rate;
static int out_format, out_channels, out_rate;
static int64_t in_frames, out_frames;
static ReplayGainInfo gain_info;

static bool_t change_op;
static OutputPlugin * new_op;

static void * buffer1, * buffer2;
static int buffer1_size, buffer2_size;

static inline int FR2MS (int64_t f, int r)
 { return (f > 0) ? (f * 1000 + r / 2) / r : (f * 1000 - r / 2) / r; }
static inline int MS2FR (int64_t ms, int r)
 { return (ms > 0) ? (ms * r + 500) / 1000 : (ms * r - 500) / 1000; }

static inline int get_format (void)
{
    switch (get_int (NULL, "output_bit_depth"))
    {
        case 16: return FMT_S16_NE;
        case 24: return FMT_S24_NE;
        case 32: return FMT_S32_NE;
        default: return FMT_FLOAT;
    }
}

static void ensure_buffer (void * * buffer, int * size, int newsize)
{
    if (newsize > * size)
    {
        g_free (* buffer);
        * buffer = g_malloc (newsize);
        * size = newsize;
    }
}

/* assumes LOCK_ALL, s_output */
static void cleanup_output (void)
{
    if (! (s_paused || s_aborted) && PLUGIN_HAS_FUNC (cop, drain))
    {
        UNLOCK_MINOR;
        cop->drain ();
        LOCK_MINOR;
    }

    s_output = FALSE;

    g_free (buffer1);
    g_free (buffer2);
    buffer1 = NULL;
    buffer2 = NULL;
    buffer1_size = 0;
    buffer2_size = 0;

    if (PLUGIN_HAS_FUNC (cop, close_audio))
        cop->close_audio ();

    effect_flush ();
    vis_runner_start_stop (FALSE, FALSE);
}

/* assumes LOCK_ALL, s_output */
static void apply_pause (void)
{
    if (PLUGIN_HAS_FUNC (cop, pause))
        cop->pause (s_paused);

    vis_runner_start_stop (TRUE, s_paused);
}

/* assumes LOCK_ALL, s_input */
static void setup_output (void)
{
    int format = get_format ();
    int channels = in_channels;
    int rate = in_rate;

    effect_start (& channels, & rate);
    eq_set_format (channels, rate);

    if (s_output && format == out_format && channels == out_channels && rate ==
     out_rate && ! PLUGIN_HAS_FUNC (cop, force_reopen))
        return;

    if (s_output)
        cleanup_output ();

    if (! cop || ! PLUGIN_HAS_FUNC (cop, open_audio) || ! cop->open_audio (format, rate, channels))
        return;

    s_output = TRUE;

    out_format = format;
    out_channels = channels;
    out_rate = rate;
    out_frames = 0;

    apply_pause ();
}

/* assumes LOCK_MINOR, s_output */
static void flush_output (void)
{
    if (PLUGIN_HAS_FUNC (cop, flush))
    {
        cop->flush (0);
        out_frames = 0;
    }

    effect_flush ();
    vis_runner_flush ();
}

static void apply_replay_gain (float * data, int samples)
{
    if (! get_bool (NULL, "enable_replay_gain"))
        return;

    float factor = powf (10, get_double (NULL, "replay_gain_preamp") / 20);

    if (s_gain)
    {
        float peak;

        if (get_bool (NULL, "replay_gain_album"))
        {
            factor *= powf (10, gain_info.album_gain / 20);
            peak = gain_info.album_peak;
        }
        else
        {
            factor *= powf (10, gain_info.track_gain / 20);
            peak = gain_info.track_peak;
        }

        if (get_bool (NULL, "enable_clipping_prevention") && peak * factor > 1)
            factor = 1 / peak;
    }
    else
        factor *= powf (10, get_double (NULL, "default_gain") / 20);

    if (factor < 0.99 || factor > 1.01)
        audio_amplify (data, 1, samples, & factor);
}

static void apply_software_volume (float * data, int channels, int samples)
{
    if (! get_bool (NULL, "software_volume_control"))
        return;

    int l = get_int (NULL, "sw_volume_left");
    int r = get_int (NULL, "sw_volume_right");

    if (l == 100 && r == 100)
        return;

    float lfactor = (l == 0) ? 0 : powf (10, (float) SW_VOLUME_RANGE * (l - 100) / 100 / 20);
    float rfactor = (r == 0) ? 0 : powf (10, (float) SW_VOLUME_RANGE * (r - 100) / 100 / 20);
    float factors[channels];

    if (channels == 2)
    {
        factors[0] = lfactor;
        factors[1] = rfactor;
    }
    else
    {
        for (int c = 0; c < channels; c ++)
            factors[c] = MAX (lfactor, rfactor);
    }

    audio_amplify (data, channels, samples / channels, factors);
}

/* assumes LOCK_ALL, s_output */
static void write_output_raw (void * data, int samples)
{
    vis_runner_pass_audio (FR2MS (out_frames, out_rate), data, samples,
     out_channels, out_rate);
    out_frames += samples / out_channels;

    eq_filter (data, samples);
    apply_software_volume (data, out_channels, samples);

    if (get_bool (NULL, "soft_clipping"))
        audio_soft_clip (data, samples);

    if (out_format != FMT_FLOAT)
    {
        ensure_buffer (& buffer2, & buffer2_size, FMT_SIZEOF (out_format) * samples);
        audio_to_int (data, buffer2, out_format, samples);
        data = buffer2;
    }

    while (! (s_aborted || s_resetting))
    {
        bool_t blocking = ! PLUGIN_HAS_FUNC (cop, buffer_free);
        int ready;

        if (blocking)
            ready = out_channels * (out_rate / 50);
        else
            ready = cop->buffer_free () / FMT_SIZEOF (out_format);

        ready = MIN (ready, samples);

        if (PLUGIN_HAS_FUNC (cop, write_audio))
        {
            cop->write_audio (data, FMT_SIZEOF (out_format) * ready);
            data = (char *) data + FMT_SIZEOF (out_format) * ready;
            samples -= ready;
        }

        if (samples == 0)
            break;

        UNLOCK_MINOR;

        if (! blocking)
        {
            if (PLUGIN_HAS_FUNC (cop, period_wait))
                cop->period_wait ();
            else
                g_usleep (20000);
        }

        LOCK_MINOR;
    }
}

/* assumes LOCK_ALL, s_input, s_output */
static bool_t write_output (void * data, int size, int stop_time)
{
    bool_t stopped = FALSE;

    int64_t cur_frame = in_frames;
    int samples = size / FMT_SIZEOF (in_format);

    /* always update in_frames, whether we use all the decoded frames or not */
    in_frames += samples / in_channels;

    if (stop_time != -1)
    {
        int64_t frames_left = MS2FR (stop_time - seek_time, in_rate) - cur_frame;
        int64_t samples_left = in_channels * MAX (0, frames_left);

        if (samples >= samples_left)
        {
            samples = samples_left;
            stopped = TRUE;
        }
    }

    if (s_aborted)
        return ! stopped;

    if (in_format != FMT_FLOAT)
    {
        ensure_buffer (& buffer1, & buffer1_size, sizeof (float) * samples);
        audio_from_int (data, in_format, buffer1, samples);
        data = buffer1;
    }

    float * fdata = data;
    apply_replay_gain (fdata, samples);
    effect_process (& fdata, & samples);
    write_output_raw (fdata, samples);

    return ! stopped;
}

/* assumes LOCK_ALL, s_output */
static void finish_effects (void)
{
    float * data = NULL;
    int samples = 0;

    effect_finish (& data, & samples);
    write_output_raw (data, samples);
}

bool_t output_open_audio (int format, int rate, int channels)
{
    /* prevent division by zero */
    if (rate < 1 || channels < 1)
        return FALSE;

    LOCK_ALL;

    if (s_output && s_paused)
    {
        flush_output ();
        s_paused = FALSE;
        apply_pause ();
    }

    s_input = TRUE;
    s_gain = s_paused = s_aborted = FALSE;
    seek_time = 0;

    in_format = format;
    in_channels = channels;
    in_rate = rate;
    in_frames = 0;

    setup_output ();

    UNLOCK_ALL;
    return TRUE;
}

void output_set_replaygain_info (const ReplayGainInfo * info)
{
    LOCK_ALL;

    if (s_input)
    {
        memcpy (& gain_info, info, sizeof (ReplayGainInfo));
        s_gain = TRUE;

        AUDDBG ("Replay Gain info:\n");
        AUDDBG (" album gain: %f dB\n", info->album_gain);
        AUDDBG (" album peak: %f\n", info->album_peak);
        AUDDBG (" track gain: %f dB\n", info->track_gain);
        AUDDBG (" track peak: %f\n", info->track_peak);
    }

    UNLOCK_ALL;
}

/* returns FALSE if stop_time is reached */
bool_t output_write_audio (void * data, int size, int stop_time)
{
    LOCK_ALL;
    bool_t good = FALSE;

    if (s_input)
    {
        while ((! s_output || s_resetting) && ! s_aborted)
        {
            UNLOCK_ALL;
            g_usleep (20000);
            LOCK_ALL;
        }

        good = write_output (data, size, stop_time);
    }

    UNLOCK_ALL;
    return good;
}

void output_abort_write (void)
{
    LOCK_MINOR;

    if (s_input)
    {
        s_aborted = TRUE;

        if (s_output)
            flush_output ();
    }

    UNLOCK_MINOR;
}

void output_pause (bool_t pause)
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

int output_written_time (void)
{
    LOCK_MINOR;
    int time = 0;

    if (s_input)
        time = seek_time + FR2MS (in_frames, in_rate);

    UNLOCK_MINOR;
    return time;
}

void output_set_time (int time)
{
    LOCK_ALL;

    if (s_input)
    {
        s_aborted = FALSE;
        seek_time = time;
        in_frames = 0;
    }

    UNLOCK_ALL;
}

bool_t output_is_open (void)
{
    LOCK_MINOR;
    bool_t is_open = s_input;
    UNLOCK_MINOR;
    return is_open;
}

int output_get_time (void)
{
    LOCK_MINOR;
    int time = 0, delay = 0;

    if (s_input)
    {
        if (s_output && PLUGIN_HAS_FUNC (cop, output_time))
            delay = FR2MS (out_frames, out_rate) - cop->output_time ();

        delay = effect_adjust_delay (delay);
        time = FR2MS (in_frames, in_rate);
        time = seek_time + MAX (time - delay, 0);
    }

    UNLOCK_MINOR;
    return time;
}

int output_get_raw_time (void)
{
    LOCK_MINOR;
    int time = 0;

    if (s_output && PLUGIN_HAS_FUNC (cop, output_time))
        time = cop->output_time ();

    UNLOCK_MINOR;
    return time;
}

void output_close_audio (void)
{
    LOCK_ALL;

    if (s_input)
    {
        s_input = FALSE;

        if (s_output && ! (s_paused || s_aborted || s_resetting))
            finish_effects (); /* first time for end of song */
    }

    UNLOCK_ALL;
}

void output_drain (void)
{
    LOCK_ALL;

    if (! s_input && s_output)
    {
        finish_effects (); /* second time for end of playlist */
        cleanup_output ();
    }

    UNLOCK_ALL;
}

void output_reset (int type)
{
    LOCK_MINOR;

    s_resetting = TRUE;

    if (s_output)
        flush_output ();

    UNLOCK_MINOR;
    LOCK_ALL;

    if (s_output && type != OUTPUT_RESET_EFFECTS_ONLY)
        cleanup_output ();

    if (type == OUTPUT_RESET_HARD)
    {
        if (cop && PLUGIN_HAS_FUNC (cop, cleanup))
            cop->cleanup ();

        if (change_op)
            cop = new_op;

        if (cop && PLUGIN_HAS_FUNC (cop, init) && ! cop->init ())
            cop = NULL;
    }

    if (s_input)
        setup_output ();

    s_resetting = FALSE;

    UNLOCK_ALL;
}

void output_get_volume (int * left, int * right)
{
    LOCK_MINOR;

    * left = * right = 0;

    if (get_bool (NULL, "software_volume_control"))
    {
        * left = get_int (NULL, "sw_volume_left");
        * right = get_int (NULL, "sw_volume_right");
    }
    else if (cop && PLUGIN_HAS_FUNC (cop, get_volume))
        cop->get_volume (left, right);

    UNLOCK_MINOR;
}

void output_set_volume (int left, int right)
{
    LOCK_MINOR;

    if (get_bool (NULL, "software_volume_control"))
    {
        set_int (NULL, "sw_volume_left", left);
        set_int (NULL, "sw_volume_right", right);
    }
    else if (cop && PLUGIN_HAS_FUNC (cop, set_volume))
        cop->set_volume (left, right);

    UNLOCK_MINOR;
}

static bool_t probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    OutputPlugin * op = plugin_get_header (p);

    if (! op || (PLUGIN_HAS_FUNC (op, init) && ! op->init ()))
        return TRUE; /* keep searching */

    if (PLUGIN_HAS_FUNC (op, cleanup))
        op->cleanup ();

    * pp = p;
    return FALSE; /* stop searching */
}

PluginHandle * output_plugin_probe (void)
{
    PluginHandle * p = NULL;
    plugin_for_each (PLUGIN_TYPE_OUTPUT, (PluginForEachFunc) probe_cb, & p);
    return p;
}

PluginHandle * output_plugin_get_current (void)
{
    return cop ? plugin_by_header (cop) : NULL;
}

bool_t output_plugin_set_current (PluginHandle * plugin)
{
    change_op = TRUE;
    new_op = plugin ? plugin_get_header (plugin) : NULL;
    output_reset (OUTPUT_RESET_HARD);

    bool_t success = (cop == new_op);
    change_op = FALSE;
    new_op = NULL;

    return success;
}
