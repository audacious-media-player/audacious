/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

/*#define AUD_DEBUG*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "audio.h"
#include "audconfig.h"
#include "equalizer.h"
#include "output.h"
#include "main.h"
#include "input.h"
#include "playback.h"
#include "configdb.h"

#include "flow.h"

#include "pluginenum.h"
#include "plugin-registry.h"

#include "effect.h"
#include "vis_runner.h"

#include "util.h"
#include "equalizer_flow.h"

#include <math.h>

#ifdef USE_SAMPLERATE
# include "src_flow.h"
#endif

#define SW_VOLUME_RANGE 40 /* decibels */

#define IS_S16_NE(a) ((a) == FMT_S16_NE || (G_BYTE_ORDER == G_LITTLE_ENDIAN && \
 (a) == FMT_S16_LE) || (G_BYTE_ORDER == G_BIG_ENDIAN && (a) == FMT_S16_BE))

OutputPlugin * current_output_plugin = NULL;

OutputPluginState op_state = {
    0,
    0,
    0
};

static gint decoder_srate = 0;
static gboolean bypass_dsp = FALSE;
static gboolean have_replay_gain;
static ReplayGainInfo replay_gain_info;

OutputPlugin psuedo_output_plugin = {
    .description = "XMMS reverse compatibility output plugin",
    .get_volume = output_get_volume,
    .set_volume = output_set_volume,

    .open_audio = output_open_audio,
    .write_audio = output_write_audio,
    .close_audio = output_close_audio,

    .flush = output_flush,
    .pause = output_pause,

    .buffer_free = output_buffer_free,
    .buffer_playing = output_buffer_playing,
    .output_time = get_output_time,
    .written_time = get_written_time,
};

void set_current_output_plugin (OutputPlugin * plugin)
{
    OutputPlugin * old = current_output_plugin;
    gboolean playing = playback_get_playing ();
    gboolean paused;
    gint time;

    if (playing)
    {
        paused = playback_get_paused ();
        time = playback_get_time ();
        playback_stop ();
    }

    current_output_plugin = NULL;

    if (old != NULL && old->cleanup != NULL)
        old->cleanup ();

    if (plugin->init () == OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        current_output_plugin = plugin;
    else
    {
        fprintf (stderr, "Output failed to load: %s\n", plugin->description);

        if (old == NULL || old->init () != OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
            return;

        fprintf (stderr, "Falling back to: %s\n", old->description);
        current_output_plugin = old;
    }

    if (playing)
    {
        playback_initiate ();

        if (paused)
            playback_pause ();

        playback_seek (time);
    }
}

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

void output_about (OutputPlugin * plugin)
{
    if (plugin->about != NULL)
        plugin->about ();
}

void output_configure (OutputPlugin * plugin)
{
    if (plugin->configure != NULL)
        plugin->configure ();
}

void
output_get_volume(gint * l, gint * r)
{
    if (cfg.software_volume_control)
    {
        * l = cfg.sw_volume_left;
        * r = cfg.sw_volume_right;
    }
    else if (current_output_plugin != NULL && current_output_plugin->get_volume
     != NULL)
        current_output_plugin->get_volume (l, r);
    else
    {
        * l = -1;
        * r = -1;
    }
}

void
output_set_volume(gint l, gint r)
{
    if (cfg.software_volume_control)
    {
        cfg.sw_volume_left = l;
        cfg.sw_volume_right = r;
    }
    else if (current_output_plugin != NULL && current_output_plugin->set_volume
     != NULL)
        current_output_plugin->set_volume (l, r);
}

void
output_set_eq(gboolean active, gfloat pre, gfloat * bands)
{
    AUDDBG("preamp: %f, bands: %f:%f:%f:%f:%f:%f:%f:%f:%f:%f\n", pre, bands[0], bands[1], bands[2], bands[3], bands[4],
            bands[5], bands[6], bands[7], bands[8], bands[9]);

    equalizer_flow_set_bands(pre, bands);
}

gint get_written_time (void)
{
    return current_output_plugin->written_time ();
}

gint get_output_time (void)
{
    return current_output_plugin->output_time ();
}

static gboolean
reopen_audio(AFormat fmt, gint rate, gint nch)
{
    OutputPlugin * op = current_output_plugin;

    if (op == NULL)
        return FALSE;

    /* Is our output port already open? */
    if ((op_state.rate != 0 && op_state.nch != 0) &&
        (op_state.rate == rate && op_state.nch == nch && op_state.fmt == fmt))
    {
        /* Yes, and it's the correct sampling rate. Reset the counter and go. */
	AUDDBG("flushing output instead of reopening\n");
	plugin_set_current((Plugin *)op);
        op->flush(0);
        return TRUE;
    }
    else if (op_state.rate != 0 && op_state.nch != 0)
    {
        plugin_set_current((Plugin *)op);
        op->close_audio();
	op_state.fmt = 0;
	op_state.rate = 0;
	op_state.nch = 0;
    }

    plugin_set_current((Plugin *)op);
    gint ret = op->open_audio(fmt, rate, nch);

    if (ret == 1)            /* Success? */
    {
        AUDDBG("opened audio: fmt=%d, rate=%d, nch=%d\n", fmt, rate, nch);
        op_state.fmt = fmt;
        op_state.rate = rate;
        op_state.nch = nch;

        output_set_eq (cfg.equalizer_active, cfg.equalizer_preamp,
         cfg.equalizer_bands);
        return TRUE;
    } else {
        return FALSE;
    }
}

gint
output_open_audio(AFormat fmt, gint rate, gint nch)
{
    AUDDBG("requested: fmt=%d, rate=%d, nch=%d\n", fmt, rate, nch);

    AFormat output_fmt;
    int bit_depth;

    decoder_srate = rate;
    bypass_dsp = cfg.bypass_dsp;
    have_replay_gain = FALSE;

    if (bypass_dsp) {
        AUDDBG("trying to open audio in native format\n");
        bypass_dsp = reopen_audio(fmt, rate, nch);
        AUDDBG("opening in native fmt %s\n", bypass_dsp ? "succeeded" : "failed");
    }

    if (bypass_dsp) {
        return TRUE;
    } else {
#ifdef USE_SAMPLERATE
        rate = src_flow_init(rate, nch); /* returns sample rate unchanged if resampling switched off */
#endif

        bit_depth = cfg.output_bit_depth;

        AUDDBG("bit depth: %d\n", bit_depth);
        switch (bit_depth) {
            case 32:
                output_fmt = FMT_S32_NE;
                break;
            case 24:
                output_fmt = FMT_S24_NE;
                break;
            case 16:
                output_fmt = FMT_S16_NE;
                break;
            case 0:
                output_fmt = FMT_FLOAT;
                break;
            default:
                AUDDBG("unsupported bit depth requested %d\n", bit_depth);
                output_fmt = FMT_S16_NE;
                break;
        }

        fmt = output_fmt;

        return reopen_audio(fmt, rate, nch);
    } /* bypass_dsp */
}

void
output_write_audio(gpointer ptr, gint length)
{
    OutputPlugin * op = current_output_plugin;

    /* Sanity check. */
    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->write_audio(ptr, length);
}

void
output_close_audio(void)
{
    OutputPlugin * op = current_output_plugin;

    vis_runner_flush ();

#ifdef USE_SAMPLERATE
    src_flow_free();
#endif

    /* Sanity check. */
    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->close_audio();
    AUDDBG("done\n");

    /* Reset the op_state. */
    op_state.fmt = op_state.rate = op_state.nch = 0;
    equalizer_flow_free();
}

void
output_flush(gint time)
{
    OutputPlugin * op = current_output_plugin;

    vis_runner_flush ();

    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->flush(time);
}

void
output_pause(gshort paused)
{
    OutputPlugin * op = current_output_plugin;

    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->pause(paused);
}

gint
output_buffer_free(void)
{
    OutputPlugin * op = current_output_plugin;

    if (op == NULL)
        return 0;

    plugin_set_current((Plugin *)op);
    return op->buffer_free();
}

gint
output_buffer_playing(void)
{
    OutputPlugin * op = current_output_plugin;

    if (op == NULL)
        return 0;

    plugin_set_current((Plugin *)op);
    return op->buffer_playing();
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

void output_set_replaygain_info (InputPlayback * playback, ReplayGainInfo * info)
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

void output_pass_audio (InputPlayback * playback, AFormat fmt, gint channels,
 gint size, void * data, gint * going)
{
    gint samples = size / FMT_SIZEOF (fmt);
    void * allocated = NULL;
    gint time = playback->output->written_time ();

    if (! bypass_dsp)
    {
        if (fmt != FMT_FLOAT)
        {
            gfloat * new = g_malloc (sizeof (gfloat) * samples);

            audio_from_int (data, fmt, new, samples);

            data = new;
            g_free (allocated);
            allocated = new;
        }

        vis_runner_pass_audio (time, data, samples, channels);
        adjust_volume (data, channels, samples / channels);

        samples = flow_execute (get_postproc_flow (), time, & data, sizeof
         (gfloat) * samples, FMT_FLOAT, decoder_srate, channels) / sizeof
         (gfloat);

        if (data != allocated)
        {
            g_free (allocated);
            allocated = NULL;
        }

        if (op_state.fmt != FMT_FLOAT)
        {
            void * new = g_malloc (FMT_SIZEOF (op_state.fmt) * samples);

            audio_to_int (data, new, op_state.fmt, samples);

            data = new;
            g_free (allocated);
            allocated = new;
        }

        if (IS_S16_NE (op_state.fmt))
        {
            samples = flow_execute (get_legacy_flow (), time, & data, 2 *
             samples, op_state.fmt, op_state.rate, op_state.nch) / 2;

            if (data != allocated)
            {
                g_free (allocated);
                allocated = NULL;
            }
        }
    }

    for (;;)
    {
        gint ready = playback->output->buffer_free () / FMT_SIZEOF (op_state.fmt);

        ready = MIN (ready, samples);
        playback->output->write_audio (data, FMT_SIZEOF (op_state.fmt) * ready);
        data = (char *) data + FMT_SIZEOF (op_state.fmt) * ready;
        samples -= ready;

        if (! samples)
            break;

        g_usleep (50000);
    }

    g_free (allocated);
}
