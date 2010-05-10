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

#include "config.h"

#include "audio.h"
#include "audconfig.h"
#include "effect.h"
#include "equalizer.h"
#include "flow.h"
#include "output.h"
#include "playback.h"
#include "pluginenum.h"
#include "plugin-registry.h"
#include "vis_runner.h"

#define SW_VOLUME_RANGE 40 /* decibels */

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

static AFormat decoder_format, output_format;
static gint decoder_channels, decoder_rate, effect_channels, effect_rate,
 output_channels, output_rate;
static gint64 frames_written;
static gboolean have_replay_gain;
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

static void write_buffers (void);
static void drain (void);

/* output_mutex must be locked */
static void real_close (void)
{
    vis_runner_start_stop (FALSE, FALSE);
    COP->close_audio ();
    output_opened = FALSE;
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
    LOCK;

    if (output_leave_open)
        real_close ();

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

    if (output_leave_open && COP->set_written_time != NULL)
    {
        vis_runner_time_offset (- frames_written * (gint64) 1000 / decoder_rate);
        COP->set_written_time (0);
    }

    decoder_format = format;
    decoder_channels = channels;
    decoder_rate = rate;
    frames_written = 0;

    effect_channels = channels;
    effect_rate = rate;
    new_effect_start (& effect_channels, & effect_rate);
    eq_set_format (effect_channels, effect_rate);

    if (output_leave_open && COP->set_written_time != NULL && effect_channels ==
     output_channels && effect_rate == output_rate)
        output_opened = TRUE;
    else
    {
        if (output_leave_open)
        {
            UNLOCK;
            drain ();
            LOCK;
            real_close ();
        }

        output_format = cfg.output_bit_depth == 32 ? FMT_S32_NE :
         cfg.output_bit_depth == 24 ? FMT_S24_NE : cfg.output_bit_depth == 16 ?
         FMT_S16_NE : FMT_FLOAT;
        output_channels = effect_channels;
        output_rate = effect_rate;

        if (COP->open_audio (output_format, output_rate, output_channels) > 0)
        {
            vis_runner_start_stop (TRUE, FALSE);
            output_opened = TRUE;
        }
    }

    output_leave_open = FALSE;
    output_paused = FALSE;

    UNLOCK;
    return output_opened;
}

static void output_close_audio (void)
{
    LOCK;

    output_opened = FALSE;

    if (! output_leave_open)
    {
        new_effect_flush ();
        real_close ();
    }

    UNLOCK;
}

static void output_flush (gint time)
{
    LOCK;
    frames_written = time * (gint64) decoder_rate / 1000;
    vis_runner_flush ();
    new_effect_flush ();
    COP->flush (new_effect_decoder_to_output_time (time));
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
        time = frames_written * (gint64) 1000 / decoder_rate;

    UNLOCK;
    return time;
}

static gboolean output_buffer_playing (void)
{
    LOCK;

    if (! output_paused)
    {
        UNLOCK;
        write_buffers ();
        LOCK;
        output_leave_open = TRUE;
    }

    UNLOCK;
    return FALSE;
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

static void do_write (void * data, gint samples)
{
    void * allocated = NULL;

    eq_filter (data, samples);
    apply_software_volume (data, output_channels, samples / output_channels);

    if (output_format != FMT_FLOAT)
    {
        void * new = g_malloc (FMT_SIZEOF (output_format) * samples);

        audio_to_int (data, new, output_format, samples);

        data = new;
        g_free (allocated);
        allocated = new;
    }

    if (output_format == FMT_S16_NE)
    {
        samples = flow_execute (get_legacy_flow (), 0, & data, 2 * samples,
         output_format, output_rate, output_channels) / 2;

        if (data != allocated)
        {
            g_free (allocated);
            allocated = NULL;
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

static void output_write_audio (void * data, gint size)
{
    gint samples = size / FMT_SIZEOF (decoder_format);
    void * allocated = NULL;

    LOCK;
    frames_written += samples / decoder_channels;
    UNLOCK;

    if (decoder_format != FMT_FLOAT)
    {
        gfloat * new = g_malloc (sizeof (gfloat) * samples);

        audio_from_int (data, decoder_format, new, samples);

        data = new;
        g_free (allocated);
        allocated = new;
    }

    apply_replay_gain (data, samples);
    vis_runner_pass_audio (frames_written * (gint64) 1000 / decoder_rate, data,
     samples, decoder_channels);
    new_effect_process ((gfloat * *) & data, & samples);

    if (data != allocated)
    {
        g_free (allocated);
        allocated = NULL;
    }

    do_write (data, samples);
    g_free (allocated);
}

static void write_buffers (void)
{
    gfloat * data = NULL;
    gint samples = 0;

    new_effect_finish (& data, & samples);
    do_write (data, samples);
}

static void drain (void)
{
    if (COP->buffer_playing != NULL)
    {
        while (COP->buffer_playing ())
            g_usleep (30000);
    }
    else
        COP->drain ();
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
        time = new_effect_output_to_decoder_time (COP->output_time ());
        time = MAX (0, time);
    }

    UNLOCK;
    return time;
}

void output_drain (void)
{
    LOCK;

    if (output_leave_open)
    {
        UNLOCK;
        write_buffers (); /* tell effect plugins this is the last song */
        drain ();
        LOCK;
        real_close ();
    }

    UNLOCK;
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

    /* This function is also used to restart playback (for example, when
     resampling is switched on or off), in which case we don't need to do an
     init cycle. -jlindgren */
    if (plugin != COP)
    {
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
    }

    if (playing)
    {
        playback_initiate ();

        if (paused)
            playback_pause ();

        playback_seek (time);
    }
}
