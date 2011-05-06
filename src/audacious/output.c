/*
 * output.c
 * Copyright 2009-2010 John Lindgren
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

#include <math.h>
#include <libaudcore/audio.h>

#include "audconfig.h"
#include "debug.h"
#include "effect.h"
#include "equalizer.h"
#include "output.h"
#include "playback.h"
#include "plugins.h"
#include "vis_runner.h"

#define SW_VOLUME_RANGE 40 /* decibels */

static OutputPlugin * cop = NULL;

void output_get_volume (gint * l, gint * r)
{
    if (cfg.software_volume_control)
    {
        * l = cfg.sw_volume_left;
        * r = cfg.sw_volume_right;
    }
    else if (cop != NULL && cop->get_volume != NULL)
        cop->get_volume (l, r);
    else
    {
        * l = 0;
        * r = 0;
    }
}

void output_set_volume (gint l, gint r)
{
    if (cfg.software_volume_control)
    {
        cfg.sw_volume_left = l;
        cfg.sw_volume_right = r;
    }
    else if (cop != NULL && cop->set_volume != NULL)
        cop->set_volume (l, r);
}

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static gboolean locked = FALSE;

#define LOCK do {g_static_mutex_lock (& mutex); locked = TRUE;} while (0)
#define UNLOCK do {locked = FALSE; g_static_mutex_unlock (& mutex);} while (0)
#define LOCKED g_return_if_fail (locked)
#define LOCKED_RET(a) g_return_val_if_fail (locked, a)
#define LOCK_VIS do {vis_runner_lock (); LOCK;} while (0)
#define UNLOCK_VIS do {UNLOCK; vis_runner_unlock ();} while (0)
#define LOCKED_VIS g_return_if_fail (locked && vis_runner_locked ())
#define LOCKED_VIS_RET(a) g_return_val_if_fail (locked && vis_runner_locked (), a)

static gboolean opened = FALSE;
static gboolean leave_open = FALSE;

static gboolean waiting, aborted, paused;
static gint decoder_format, decoder_channels, decoder_rate, effect_channels,
 effect_rate, output_format, output_channels, output_rate;
static gint64 frames_written;
static gboolean have_replay_gain;
static ReplayGainInfo replay_gain_info;

static void reset_time (void)
{
    LOCKED_VIS;
    g_return_if_fail (cop->set_written_time != NULL);
    vis_runner_time_offset (- cop->written_time ());
    cop->set_written_time (0);
}

static void drain (void)
{
    LOCKED;
    g_return_if_fail (cop->drain != NULL);
    cop->drain ();
}

static void real_close (void)
{
    LOCKED_VIS;
    vis_runner_start_stop (FALSE, FALSE);
    cop->close_audio ();
    opened = FALSE;
    leave_open = FALSE;
}

static gboolean open_audio (gint format, gint rate, gint channels)
{
    LOCKED_VIS_RET (FALSE);
    g_return_val_if_fail (! opened, FALSE);

    decoder_format = format;
    decoder_channels = channels;
    decoder_rate = rate;
    effect_channels = channels;
    effect_rate = rate;
    effect_start (& effect_channels, & effect_rate);
    eq_set_format (effect_channels, effect_rate);

    if (leave_open && effect_channels == output_channels && effect_rate ==
     output_rate)
    {
        reset_time ();
        opened = TRUE;
    }
    else
    {
        if (leave_open)
        {
            drain ();
            real_close ();
        }

        output_format = cfg.output_bit_depth == 32 ? FMT_S32_NE :
         cfg.output_bit_depth == 24 ? FMT_S24_NE : cfg.output_bit_depth == 16 ?
         FMT_S16_NE : FMT_FLOAT;
        output_channels = effect_channels;
        output_rate = effect_rate;

        if (cop->open_audio (output_format, output_rate, output_channels))
        {
            vis_runner_start_stop (TRUE, FALSE);
            opened = TRUE;
        }
    }

    leave_open = FALSE;
    waiting = FALSE;
    aborted = FALSE;
    paused = FALSE;
    frames_written = 0;
    have_replay_gain = FALSE;

    return opened;
}

static gboolean output_open_audio (gint format, gint rate, gint channels)
{
    g_return_val_if_fail (cop != NULL, FALSE);
    LOCK_VIS;
    gboolean success = open_audio (format, rate, channels);
    UNLOCK_VIS;
    return success;
}

static void set_gain (ReplayGainInfo * info)
{
    LOCKED;
    g_return_if_fail (opened && ! waiting);

    AUDDBG ("Replay Gain info:\n");
    AUDDBG (" album gain: %f dB\n", info->album_gain);
    AUDDBG (" album peak: %f\n", info->album_peak);
    AUDDBG (" track gain: %f dB\n", info->track_gain);
    AUDDBG (" track peak: %f\n", info->track_peak);

    have_replay_gain = TRUE;
    memcpy (& replay_gain_info, info, sizeof (ReplayGainInfo));
}

static void output_set_replaygain_info (ReplayGainInfo * info)
{
    g_return_if_fail (cop != NULL);
    LOCK;
    set_gain (info);
    UNLOCK;
}

static void apply_replay_gain (gfloat * data, gint samples)
{
    gfloat factor = powf (10, (gfloat) cfg.replay_gain_preamp / 20);

    if (! cfg.enable_replay_gain)
        return;

    if (have_replay_gain)
    {
        if (cfg.replay_gain_album)
        {
            factor *= powf (10, replay_gain_info.album_gain / 20);

            if (cfg.enable_clipping_prevention &&
             replay_gain_info.album_peak * factor > 1)
                factor = 1 / replay_gain_info.album_peak;
        }
        else
        {
            factor *= powf (10, replay_gain_info.track_gain / 20);

            if (cfg.enable_clipping_prevention &&
             replay_gain_info.track_peak * factor > 1)
                factor = 1 / replay_gain_info.track_peak;
        }
    }
    else
        factor *= powf (10, (gfloat) cfg.default_gain / 20);

    if (factor < 0.99 || factor > 1.01)
        audio_amplify (data, 1, samples, & factor);
}

static void apply_software_volume (gfloat * data, gint channels, gint frames)
{
    gfloat left_factor, right_factor;
    gfloat factors[channels];
    gint channel;

    if (! cfg.software_volume_control || (cfg.sw_volume_left == 100 &&
     cfg.sw_volume_right == 100))
        return;

    left_factor = (cfg.sw_volume_left == 0) ? 0 : powf (10, (gfloat)
     SW_VOLUME_RANGE * (cfg.sw_volume_left - 100) / 100 / 20);
    right_factor = (cfg.sw_volume_right == 0) ? 0 : powf (10, (gfloat)
     SW_VOLUME_RANGE * (cfg.sw_volume_right - 100) / 100 / 20);

    if (channels == 2)
    {
        factors[0] = left_factor;
        factors[1] = right_factor;
    }
    else
    {
        for (channel = 0; channel < channels; channel ++)
            factors[channel] = MAX (left_factor, right_factor);
    }

    audio_amplify (data, channels, frames, factors);
}

static void write_processed (void * data, gint samples)
{
    LOCKED_VIS;

    if (! samples)
        return;

    vis_runner_pass_audio (cop->written_time (), data, samples, output_channels,
     output_rate);
    eq_filter (data, samples);
    apply_software_volume (data, output_channels, samples / output_channels);

    void * allocated = NULL;

    if (output_format != FMT_FLOAT)
    {
        void * new = g_malloc (FMT_SIZEOF (output_format) * samples);
        audio_to_int (data, new, output_format, samples);
        data = new;
        g_free (allocated);
        allocated = new;
    }

    while (! aborted)
    {
        gint ready = (cop->buffer_free != NULL) ? cop->buffer_free () /
         FMT_SIZEOF (output_format) : output_channels * (output_rate / 50);
        ready = MIN (ready, samples);
        cop->write_audio (data, FMT_SIZEOF (output_format) * ready);
        data = (char *) data + FMT_SIZEOF (output_format) * ready;
        samples -= ready;

        if (! samples)
            break;

        waiting = TRUE;
        UNLOCK_VIS;

        if (cop->period_wait != NULL)
            cop->period_wait ();
        else if (cop->buffer_free != NULL)
            g_usleep (20000);

        LOCK_VIS;
        waiting = FALSE;
    }

    g_free (allocated);
}

static void write_audio (void * data, gint size)
{
    LOCKED;
    g_return_if_fail (opened && ! waiting);

    gint samples = size / FMT_SIZEOF (decoder_format);
    frames_written += samples / decoder_channels;

    void * allocated = NULL;

    if (decoder_format != FMT_FLOAT)
    {
        gfloat * new = g_malloc (sizeof (gfloat) * samples);
        audio_from_int (data, decoder_format, new, samples);
        data = new;
        g_free (allocated);
        allocated = new;
    }

    apply_replay_gain (data, samples);
    gfloat * fdata = data;
    effect_process (& fdata, & samples);
    data = fdata;

    if (data != allocated)
    {
        g_free (allocated);
        allocated = NULL;
    }

    write_processed (data, samples);
    g_free (allocated);
}

static void output_write_audio (void * data, gint size)
{
    g_return_if_fail (cop != NULL);
    LOCK_VIS;
    write_audio (data, size);
    UNLOCK_VIS;
}

static void close_audio (void)
{
    LOCKED;
    g_return_if_fail (opened && ! waiting);
    opened = FALSE;

    if (! leave_open)
    {
        effect_flush ();
        real_close ();
    }
}

static void output_close_audio (void)
{
    g_return_if_fail (cop != NULL);
    LOCK_VIS;
    close_audio ();
    UNLOCK_VIS;
}

static void do_pause (gboolean p)
{
    LOCKED_VIS;
    g_return_if_fail (opened);
    cop->pause (p);
    vis_runner_start_stop (TRUE, p);
    paused = p;
}

static void output_pause (gboolean p)
{
    g_return_if_fail (cop != NULL);
    LOCK_VIS;
    do_pause (p);
    UNLOCK_VIS;
}

static void flush (gint time)
{
    LOCKED_VIS;
    g_return_if_fail (opened);

    aborted = FALSE;

    /* When playback is started from the middle of a song, flush() is called
     * before any audio is actually written in order to set the time counter.
     * In this case, we do not want to cut off the end of the previous song, so
     * we do not actually flush. */
    if (! frames_written)
    {
        g_return_if_fail (cop->set_written_time != NULL);
        cop->set_written_time (time);
    }
    else
    {
        vis_runner_flush ();
        effect_flush ();
        cop->flush (effect_decoder_to_output_time (time));
    }

    frames_written = time * (gint64) decoder_rate / 1000;
}

static void output_flush (gint time)
{
    g_return_if_fail (cop != NULL);
    LOCK_VIS;
    flush (time);
    UNLOCK_VIS;
}

static gint written_time (void)
{
    LOCKED_RET (0);
    g_return_val_if_fail (opened && ! waiting, 0);
    return frames_written * (gint64) 1000 / decoder_rate;
}

static gint output_written_time (void)
{
    g_return_val_if_fail (cop != NULL, 0);
    LOCK;
    gint time = written_time ();
    UNLOCK;
    return time;
}

static void write_buffers (void)
{
    LOCKED;
    gfloat * data = NULL;
    gint samples = 0;
    effect_finish (& data, & samples);
    write_processed (data, samples);
}

static void set_leave_open (void)
{
    LOCKED;
    g_return_if_fail (opened && ! waiting);

    if (! paused)
    {
        write_buffers ();
        leave_open = TRUE;
    }
}

static gboolean output_buffer_playing (void)
{
    g_return_val_if_fail (cop != NULL, FALSE);
    LOCK_VIS;
    set_leave_open ();
    UNLOCK_VIS;
    return FALSE;
}

static void abort_write (void)
{
    LOCKED;
    g_return_if_fail (opened);
    aborted = TRUE;
    cop->flush (cop->output_time ());
}

static void output_abort_write (void)
{
    g_return_if_fail (cop != NULL);
    LOCK;
    abort_write ();
    UNLOCK;
}

const struct OutputAPI output_api =
{
    .open_audio = output_open_audio,
    .set_replaygain_info = output_set_replaygain_info,
    .write_audio = output_write_audio,
    .close_audio = output_close_audio,

    .pause = output_pause,
    .flush = output_flush,
    .written_time = output_written_time,
    .buffer_playing = output_buffer_playing,
    .abort_write = output_abort_write,
};

static gint output_time (void)
{
    LOCKED_RET (0);
    g_return_val_if_fail (opened || leave_open, 0);
    return cop->output_time ();
}

gint get_output_time (void)
{
    g_return_val_if_fail (cop != NULL, 0);
    LOCK;

    gint time = 0;
    if (opened)
    {
        time = effect_output_to_decoder_time (output_time ());
        time = MAX (0, time);
    }

    UNLOCK;
    return time;
}

gint get_raw_output_time (void)
{
    g_return_val_if_fail (cop != NULL, 0);
    LOCK;
    gint time = output_time ();
    UNLOCK;
    return time;
}

void output_drain (void)
{
    g_return_if_fail (cop != NULL);
    LOCK_VIS;

    if (leave_open)
    {
        write_buffers ();
        drain ();
        real_close ();
    }

    UNLOCK_VIS;
}

static gboolean probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    OutputPlugin * op = plugin_get_header (p);
    g_return_val_if_fail (op != NULL && op->init != NULL, TRUE);

    if (! op->init ())
        return TRUE;

    if (op->cleanup != NULL)
        op->cleanup ();

    * pp = p;
    return FALSE;
}

PluginHandle * output_plugin_probe (void)
{
    PluginHandle * p = NULL;
    plugin_for_each (PLUGIN_TYPE_OUTPUT, (PluginForEachFunc) probe_cb, & p);
    return p;
}

PluginHandle * output_plugin_get_current (void)
{
    return (cop != NULL) ? plugin_by_header (cop) : NULL;
}

gboolean output_plugin_set_current (PluginHandle * plugin)
{
    if (cop != NULL)
    {
        if (playback_get_playing ())
            playback_stop ();

        if (cop->cleanup != NULL)
            cop->cleanup ();

        cop = NULL;
    }

    if (plugin != NULL)
    {
        OutputPlugin * op = plugin_get_header (plugin);
        g_return_val_if_fail (op != NULL && op->init != NULL, FALSE);

        if (! op->init ())
            return FALSE;

        cop = op;
    }

    return TRUE;
}
