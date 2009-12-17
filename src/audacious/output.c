/*
 * output.c
 * Copyright 2009 John Lindgren
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

#include "config.h"

#include "audio.h"
#include "audconfig.h"
#include "effect.h"
#include "equalizer_flow.h"
#include "flow.h"
#include "output.h"
#include "playback.h"
#include "pluginenum.h"
#include "plugin-registry.h"
#include "vis_runner.h"

#ifdef USE_SAMPLERATE
#include "src_flow.h"
#endif

#define SW_VOLUME_RANGE 40 /* decibels */

#define IS_S16_NE(a) ((a) == FMT_S16_NE || (G_BYTE_ORDER == G_LITTLE_ENDIAN && \
 (a) == FMT_S16_LE) || (G_BYTE_ORDER == G_BIG_ENDIAN && (a) == FMT_S16_BE))

OutputPlugin * current_output_plugin = NULL;
#define COP current_output_plugin

static gboolean plugin_list_func (void * plugin, void * data)
{
    GList * * list = data;

    * list = g_list_prepend (* list, plugin);
    return TRUE;
}

GList * get_output_list (void)
{
    static GList * list = NULL;

    if (list == NULL)
        plugin_for_each (PLUGIN_TYPE_OUTPUT, plugin_list_func, & list);

    return list;
}

void output_get_volume (gint * l, gint * r)
{
    if (cfg.software_volume_control)
    {
        * l = cfg.sw_volume_left;
        * r = cfg.sw_volume_right;
    }
    else if (COP != NULL && COP->get_volume != NULL)
        COP->get_volume (l, r);
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
    else if (COP != NULL && COP->set_volume != NULL)
        COP->set_volume (l, r);
}

static GMutex * output_mutex;
static gboolean output_opened, output_leave_open, output_paused;
static gint output_close_source;

static AFormat decoder_format, output_format;
static gint output_channels, decoder_rate, output_rate;
static gboolean bypass_dsp, resampling, have_replay_gain;
static ReplayGainInfo replay_gain_info;

#define REMOVE_SOURCE(s) \
do { \
    if (s != 0) { \
        g_source_remove (s); \
        s = 0; \
    } \
} while (0)

#define LOCK g_mutex_lock (output_mutex)
#define UNLOCK g_mutex_unlock (output_mutex)

/* output_mutex must be locked */
static void drain (void)
{
    if (output_opened || ! output_leave_open)
        return;

    REMOVE_SOURCE (output_close_source);

    while (COP->buffer_playing ())
    {
        UNLOCK;
        g_usleep (30000);
        LOCK;

        if (output_opened || ! output_leave_open)
            return;
    }

    COP->close_audio ();
    vis_runner_start_stop (FALSE, FALSE);
    output_leave_open = FALSE;
}

void output_init (void)
{
    output_mutex = g_mutex_new ();
    output_opened = FALSE;
    output_leave_open = FALSE;
    vis_runner_init ();
}

void output_cleanup (void)
{
    if (playback_get_playing ())
        playback_stop ();

    LOCK;
    drain ();
    UNLOCK;

    g_mutex_free (output_mutex);
}

static gboolean output_open_audio (AFormat format, gint rate, gint channels)
{
    if (COP == NULL)
    {
        fprintf (stderr, "No output plugin selected.\n");
        return FALSE;
    }

    LOCK;

    if (output_leave_open)
    {
        if (channels == output_channels && (resampling || rate == output_rate)
         && COP->set_written_time != NULL)
        {
            REMOVE_SOURCE (output_close_source);
            vis_runner_time_offset (- COP->written_time ());
            COP->set_written_time (0);
            output_opened = TRUE;
        }
        else
            drain ();
    }

    decoder_format = format;
    decoder_rate = rate;
    output_leave_open = FALSE;
    output_paused = FALSE;

    if (! output_opened)
    {
        bypass_dsp = cfg.bypass_dsp;
        output_format = bypass_dsp ? format : cfg.output_bit_depth == 32 ?
         FMT_S32_NE : cfg.output_bit_depth == 24 ? FMT_S24_NE :
         cfg.output_bit_depth == 16 ? FMT_S16_NE : FMT_FLOAT;
        output_channels = channels;

#ifdef USE_SAMPLERATE
        if (cfg.enable_src && ! bypass_dsp)
        {
            src_flow_init (rate, channels);
            resampling = TRUE;
            output_rate = cfg.src_rate;
        }
        else
        {
            src_flow_free ();
            resampling = FALSE;
            output_rate = rate;
        }
#else
        resampling = FALSE;
        output_rate = rate;
#endif

        if (COP->open_audio (output_format, output_rate, output_channels) > 0)
        {
            vis_runner_start_stop (TRUE, FALSE);
            output_opened = TRUE;
        }
        else
        {
            UNLOCK;
            return FALSE;
        }
    }

    UNLOCK;
    return TRUE;
}

static gboolean close_cb (void * unused)
{
    gboolean playing;

    LOCK;
    playing = COP->buffer_playing ();

    if (! playing)
    {
        output_close_source = 0;
        COP->close_audio ();
        vis_runner_start_stop (FALSE, FALSE);
        output_leave_open = FALSE;
    }

    UNLOCK;
    return playing;
}

static void output_close_audio (void)
{
    LOCK;

    if (output_paused)
        output_leave_open = FALSE;

    if (output_leave_open)
        output_close_source = g_timeout_add (30, close_cb, NULL);
    else
    {
        COP->close_audio ();
        vis_runner_start_stop (FALSE, FALSE);
    }

    output_opened = FALSE;
    UNLOCK;
}

static void output_flush (gint time)
{
    LOCK;
    COP->flush (time);
    vis_runner_flush ();
    UNLOCK;
}

static void output_pause (gboolean pause)
{
    LOCK;
    COP->pause (pause);
    vis_runner_start_stop (TRUE, pause);
    output_paused = pause;
    UNLOCK;
}

static gint get_written_time (void)
{
    gint time = 0;

    LOCK;

    if (output_opened)
        time = COP->written_time ();

    UNLOCK;
    return time;
}

static gboolean output_buffer_playing (void)
{
    LOCK;
    output_leave_open = TRUE;
    UNLOCK;
    return FALSE;
}

static Flow * get_postproc_flow (void)
{
    static Flow * flow = NULL;

    if (flow == NULL)
    {
        flow = flow_new ();

#ifdef USE_SAMPLERATE
            flow_link_element (flow, src_flow);
#endif

        flow_link_element (flow, equalizer_flow);
    }

    return flow;
}

static Flow * get_legacy_flow (void)
{
    static Flow * flow = NULL;

    if (flow == NULL)
    {
        flow = flow_new ();
        flow_link_element (flow, effect_flow);
    }

    return flow;
}

static void output_set_replaygain_info (ReplayGainInfo * info)
{
    AUDDBG ("Replay Gain info:\n");
    AUDDBG (" album gain: %f dB\n", info->album_gain);
    AUDDBG (" album peak: %f\n", info->album_peak);
    AUDDBG (" track gain: %f dB\n", info->track_gain);
    AUDDBG (" track peak: %f\n", info->track_peak);

    have_replay_gain = TRUE;
    memcpy (& replay_gain_info, info, sizeof (ReplayGainInfo));
}

static void adjust_volume (gfloat * data, gint channels, gint frames)
{
    gboolean amplify = FALSE;
    gfloat factors[channels];
    gint channel;

    for (channel = 0; channel < channels; channel ++)
        factors[channel] = 1;

    if (cfg.enable_replay_gain)
    {
        gfloat factor = powf (10, (gfloat) cfg.replay_gain_preamp / 20);

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
        {
            amplify = TRUE;

            for (channel = 0; channel < channels; channel ++)
                factors[channel] *= factor;
        }
    }

    if (cfg.software_volume_control && (cfg.sw_volume_left != 100 ||
     cfg.sw_volume_right != 100))
    {
        gfloat left_factor = powf (10, (gfloat) SW_VOLUME_RANGE *
         (cfg.sw_volume_left - 100) / 100 / 20);
        gfloat right_factor = powf (10, (gfloat) SW_VOLUME_RANGE *
         (cfg.sw_volume_right - 100) / 100 / 20);

        amplify = TRUE;

        if (channels == 2)
        {
            factors[0] *= left_factor;
            factors[1] *= right_factor;
        }
        else
        {
            for (channel = 0; channel < channels; channel ++)
                factors[channel] *= MAX (left_factor, right_factor);
        }
    }

    if (amplify)
        audio_amplify (data, channels, frames, factors);
}

static void output_write_audio (void * data, gint size)
{
    gint samples = size / FMT_SIZEOF (decoder_format);
    void * allocated = NULL;
    gint time = COP->written_time ();

    if (! bypass_dsp)
    {
        if (decoder_format != FMT_FLOAT)
        {
            gfloat * new = g_malloc (sizeof (gfloat) * samples);

            audio_from_int (data, decoder_format, new, samples);

            data = new;
            g_free (allocated);
            allocated = new;
        }

        vis_runner_pass_audio (time, data, samples, output_channels);
        adjust_volume (data, output_channels, samples / output_channels);

        samples = flow_execute (get_postproc_flow (), time, & data, sizeof
         (gfloat) * samples, FMT_FLOAT, decoder_rate, output_channels) / sizeof
         (gfloat);

        if (data != allocated)
        {
            g_free (allocated);
            allocated = NULL;
        }

        if (output_format != FMT_FLOAT)
        {
            void * new = g_malloc (FMT_SIZEOF (output_format) * samples);

            audio_to_int (data, new, output_format, samples);

            data = new;
            g_free (allocated);
            allocated = new;
        }

        if (IS_S16_NE (output_format))
        {
            samples = flow_execute (get_legacy_flow (), time, & data, 2 *
             samples, output_format, output_rate, output_channels) / 2;

            if (data != allocated)
            {
                g_free (allocated);
                allocated = NULL;
            }
        }
    }

    if (COP->buffer_free == NULL)
        COP->write_audio (data, FMT_SIZEOF (output_format) * samples);
    else
    {
        while (1)
        {
            gint ready = COP->buffer_free () / FMT_SIZEOF (output_format);

            ready = MIN (ready, samples);
            COP->write_audio (data, FMT_SIZEOF (output_format) * ready);
            data = (char *) data + FMT_SIZEOF (output_format) * ready;
            samples -= ready;

            if (samples == 0)
                break;

            g_usleep (50000);
        }
    }

    g_free (allocated);
}

struct OutputAPI output_api =
{
    .open_audio = output_open_audio,
    .set_replaygain_info = output_set_replaygain_info,
    .write_audio = output_write_audio,
    .close_audio = output_close_audio,

    .pause = output_pause,
    .flush = output_flush,
    .written_time = get_written_time,
    .buffer_playing = output_buffer_playing,
};

gint get_output_time (void)
{
    gint time = 0;

    LOCK;

    if (output_opened)
    {
        time = COP->output_time ();
        time = MAX (0, time);
    }

    UNLOCK;
    return time;
}

void set_current_output_plugin (OutputPlugin * plugin)
{
    OutputPlugin * old = COP;
    gboolean playing = playback_get_playing ();
    gboolean paused = FALSE;
    gint time = 0;

    if (playing)
    {
        paused = playback_get_paused ();
        time = playback_get_time ();
        playback_stop ();
    }

    drain ();
    COP = NULL;

    if (old != NULL && old->cleanup != NULL)
        old->cleanup ();

    if (plugin->init () == OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        COP = plugin;
    else
    {
        fprintf (stderr, "Output plugin failed to load: %s\n",
         plugin->description);

        if (old == NULL || old->init () != OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
            return;

        fprintf (stderr, "Falling back to: %s\n", old->description);
        COP = old;
    }

    if (playing)
    {
        playback_initiate ();

        if (paused)
            playback_pause ();

        playback_seek (time);
    }
}
