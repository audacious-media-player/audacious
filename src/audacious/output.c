/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious team
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

#include "output.h"
#include "iir.h"
#include "main.h"
#include "input.h"
#include "playback.h"

#include "playlist.h"
#include "configdb.h"

#include "flow.h"

#include "pluginenum.h"

#include "effect.h"
#include "volumecontrol.h"
#include "visualization.h"

#include "libSAD.h"
#include "util.h"

#include <math.h>

#ifdef USE_SRC
# include "src_flow.h"
#endif

#define FMT_FRACBITS(a) ( (a) == FMT_FIXED32 ? __AUDACIOUS_ASSUMED_MAD_F_FRACBITS__ : 0 )

OutputPluginData op_data = {
    NULL,
    NULL
};

OutputPluginState op_state = {
    0,
    0,
    0
};

static gint decoder_srate = 0;

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

static void apply_replaygain_info (ReplayGainInfo *rg_info);

OutputPlugin *
get_current_output_plugin(void)
{
    return op_data.current_output_plugin;
}

void
set_current_output_plugin(gint i)
{
    gboolean playing;
    OutputPlugin *op = get_current_output_plugin();

    GList *node = g_list_nth(op_data.output_list, i);
    if (!node) {
        op_data.current_output_plugin = NULL;
        return;
    }

    op_data.current_output_plugin = node->data;

    playing = playback_get_playing();

    if (playing == TRUE)
    {
        guint time, pos;
        PlaylistEntry *entry;
	
	plugin_set_current((Plugin *)op);

        /* don't stop yet, get the seek time and playlist position first */
        pos = playlist_get_position(playlist_get_active());
        time = op->output_time();

        /* reset the audio system */
        mainwin_stop_pushed();
        op->close_audio();

        g_usleep(300000);

        /* wait for the playback thread to come online */
        while (playback_get_playing())
            g_message("waiting for audio system shutdown...");

        /* wait for the playback thread to come online */
        playlist_set_position(playlist_get_active(), pos);
        entry = playlist_get_entry_to_play(playlist_get_active());
        playback_play_file(entry);

        while (!playback_get_playing())
        {
            gtk_main_iteration();
                g_message("waiting for audio system startup...");
        }

        /* and signal a reseek */
        if (playlist_get_current_length(playlist_get_active()) > -1 &&
            time <= (playlist_get_current_length(playlist_get_active())))
            playback_seek(time / 1000);
    }
}

GList *
get_output_list(void)
{
    return op_data.output_list;
}

void
output_about(gint i)
{
    OutputPlugin *out = g_list_nth(op_data.output_list, i)->data;
    if (out && out->about)
    {
	plugin_set_current((Plugin *)out);
        out->about();
    }
}

void
output_configure(gint i)
{
    OutputPlugin *out = g_list_nth(op_data.output_list, i)->data;
    if (out && out->configure)
    {
	plugin_set_current((Plugin *)out);
        out->configure();
    }
}

void
output_get_volume(gint * l, gint * r)
{
    *l = *r = -1;

    if (!op_data.current_output_plugin)
        return;

    if (!op_data.current_output_plugin->get_volume)
        return;

    if (cfg.software_volume_control)
        volumecontrol_get_volume_state(l, r);
    else
    {
        plugin_set_current((Plugin *)op_data.current_output_plugin);
        op_data.current_output_plugin->get_volume(l, r);
    }
}

void
output_set_volume(gint l, gint r)
{
    if (!op_data.current_output_plugin)
        return;

    if (!op_data.current_output_plugin->set_volume)
        return;

    if (cfg.software_volume_control)
        volumecontrol_set_volume_state(l, r);
    else
    {
        plugin_set_current((Plugin *)op_data.current_output_plugin);
        op_data.current_output_plugin->set_volume(l, r);
    }
}

void
output_set_eq(gboolean active, gfloat pre, gfloat * bands)
{
    int i;
    preamp[0] = 1.0 + 0.0932471 * pre + 0.00279033 * pre * pre;
    preamp[1] = 1.0 + 0.0932471 * pre + 0.00279033 * pre * pre;

    for (i = 0; i < 10; ++i)
    {
        set_gain(i, 0, 0.03 * bands[i] + 0.000999999 * bands[i] * bands[i]);
        set_gain(i, 1, 0.03 * bands[i] + 0.000999999 * bands[i] * bands[i]);
    }
}

/* called by input plugin to peek at the output plugin's write progress */
gint
get_written_time(void)
{
    OutputPlugin *op = get_current_output_plugin();

    plugin_set_current((Plugin *)op);
    return op->written_time();
}

/* called by input plugin to peek at the output plugin's output progress */
gint
get_output_time(void)
{
    OutputPlugin *op = get_current_output_plugin();

    plugin_set_current((Plugin *)op);
    return op->output_time();
}

static SAD_dither_t *sad_state_to_float = NULL;
static SAD_dither_t *sad_state_from_float = NULL;
static float *sad_float_buf = NULL;
static void *sad_out_buf = NULL;
static guint sad_float_buf_length = 0;
static guint sad_out_buf_length = 0;
static ReplayGainInfo replay_gain_info = {
    .track_gain = 0.0,
    .track_peak = 0.0,
    .album_gain = 0.0,
    .album_peak = 0.0,
};

static void
freeSAD()
{
    if (sad_state_from_float != NULL) {SAD_dither_free(sad_state_from_float); sad_state_from_float = NULL;}
    if (sad_state_to_float != NULL)   {SAD_dither_free(sad_state_to_float);   sad_state_to_float = NULL;}
}

gint
output_open_audio(AFormat fmt, gint rate, gint nch)
{
    gint ret;
    OutputPlugin *op;
    AUDDBG("requested: fmt=%d, rate=%d, nch=%d\n", fmt, rate, nch);

    AFormat output_fmt;
    int bit_depth;
    SAD_buffer_format input_sad_fmt;
    SAD_buffer_format output_sad_fmt;

    decoder_srate = rate;

#ifdef USE_SRC
    if(cfg.enable_src) rate = cfg.src_rate;
#endif
    
    bit_depth = cfg.output_bit_depth;

    AUDDBG("bit depth: %d\n", bit_depth);
    output_fmt = (bit_depth == 24) ? FMT_S24_NE : FMT_S16_NE;
    
    freeSAD();

    AUDDBG("initializing dithering engine for 2 stage conversion: fmt%d --> float -->fmt%d\n", fmt, output_fmt);
    input_sad_fmt.sample_format = sadfmt_from_afmt(fmt);
    if (input_sad_fmt.sample_format < 0) return FALSE;
    input_sad_fmt.fracbits = FMT_FRACBITS(fmt);
    input_sad_fmt.channels = nch;
    input_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
    input_sad_fmt.samplerate = 0;
    
    output_sad_fmt.sample_format = SAD_SAMPLE_FLOAT;
    output_sad_fmt.fracbits = 0;
    output_sad_fmt.channels = nch;
    output_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
    output_sad_fmt.samplerate = 0;
    
    sad_state_to_float = SAD_dither_init(&input_sad_fmt, &output_sad_fmt, &ret);
    if (sad_state_to_float == NULL) {
        AUDDBG("ditherer init failed (decoder's native --> float)\n");
        return FALSE;
    }
    SAD_dither_set_dither (sad_state_to_float, FALSE);
    
    input_sad_fmt.sample_format = SAD_SAMPLE_FLOAT;
    input_sad_fmt.fracbits = 0;
    input_sad_fmt.channels = nch;
    input_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
    input_sad_fmt.samplerate = 0;
    
    output_sad_fmt.sample_format = sadfmt_from_afmt(output_fmt);
    if (output_sad_fmt.sample_format < 0) return FALSE;
    output_sad_fmt.fracbits = FMT_FRACBITS(output_fmt);
    output_sad_fmt.channels = nch;
    output_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
    output_sad_fmt.samplerate = 0;
    
    sad_state_from_float = SAD_dither_init(&input_sad_fmt, &output_sad_fmt, &ret);
    if (sad_state_from_float == NULL) {
        SAD_dither_free(sad_state_to_float);
        AUDDBG("ditherer init failed (float --> output)\n");
        return FALSE;
    }
    SAD_dither_set_dither (sad_state_from_float, TRUE);
    
    fmt = output_fmt;

    if(replay_gain_info.album_peak == 0.0 && replay_gain_info.track_peak == 0.0) {
        AUDDBG("RG info isn't set yet. Filling replay_gain_info with default values.\n");
        replay_gain_info.track_gain = cfg.default_gain;
        replay_gain_info.track_peak = 0.01;
        replay_gain_info.album_gain = cfg.default_gain;
        replay_gain_info.album_peak = 0.01;
    }
    apply_replaygain_info(&replay_gain_info);
    
    op = get_current_output_plugin();

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
    }

    plugin_set_current((Plugin *)op);
    ret = op->open_audio(fmt, rate, nch);

    if (ret == 1)            /* Success? */
    {
        AUDDBG("opened audio: fmt=%d, rate=%d, nch=%d\n", fmt, rate, nch);
        op_state.fmt = fmt;
        op_state.rate = rate;
        op_state.nch = nch;
    }

    return ret;
}

void
output_write_audio(gpointer ptr, gint length)
{
    OutputPlugin *op = get_current_output_plugin();

    /* Sanity check. */
    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->write_audio(ptr, length);
}

void
output_close_audio(void)
{
    OutputPlugin *op = get_current_output_plugin();

    freeSAD();
    
    AUDDBG("clearing RG settings\n");
    replay_gain_info.track_gain = 0.0;
    replay_gain_info.track_peak = 0.0;
    replay_gain_info.album_gain = 0.0;
    replay_gain_info.album_peak = 0.0;

    /* Do not close if there are still songs to play and the user has 
     * not requested a stop.  --nenolod
     */
    Playlist *pl = playlist_get_active();
    if (ip_data.stop == FALSE &&
       (playlist_get_position_nolock(pl) < playlist_get_length(pl) - 1)) {
            AUDDBG("leaving audio opened\n");
            return;
        }

    /* Sanity check. */
    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->close_audio();
    AUDDBG("done\n");
#ifdef USE_SRC
    src_flow_free();
#endif

    /* Reset the op_state. */
    op_state.fmt = op_state.rate = op_state.nch = 0;
}

void
output_flush(gint time)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->flush(time);
}

void
output_pause(gshort paused)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return;

    plugin_set_current((Plugin *)op);
    op->pause(paused);
}

gint
output_buffer_free(void)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return 0;

    plugin_set_current((Plugin *)op);
    return op->buffer_free();
}

gint
output_buffer_playing(void)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return 0;

    plugin_set_current((Plugin *)op);
    return op->buffer_playing();
}

/* called by input plugin when data is ready */
void
output_pass_audio(InputPlayback *playback,
              AFormat fmt,       /* output format        */
              gint nch,          /* channels             */
              gint length,       /* length of sample     */
              gpointer ptr,      /* data                 */
              int *going         /* 0 when time to stop  */
              )
{
    static Flow *postproc_flow = NULL;
    static Flow *legacy_flow = NULL;
    OutputPlugin *op = playback->output;
    gint writeoffs;
    gpointer float_ptr;

    if(length <= 0 || sad_state_from_float == NULL || sad_state_to_float == NULL) return;
    
    plugin_set_current((Plugin *)(playback->output));
    gint time = playback->output->written_time();

    if (legacy_flow == NULL)
    {
        legacy_flow = flow_new();
        flow_link_element(legacy_flow, iir_flow);
        flow_link_element(legacy_flow, effect_flow);
        flow_link_element(legacy_flow, volumecontrol_flow);
    }
    
    if (postproc_flow == NULL)
    {
        postproc_flow = flow_new();
        flow_link_element(postproc_flow, vis_flow);
#ifdef USE_SRC
        flow_link_element(postproc_flow, src_flow);
#endif
    }

    int frames = length / nch / FMT_SIZEOF(fmt);
    int len = frames * nch * sizeof(float);
    if(sad_float_buf == NULL || sad_float_buf_length < len) {
        sad_float_buf_length = len;
        sad_float_buf = smart_realloc(sad_float_buf, &sad_float_buf_length);
    }

    SAD_dither_process_buffer(sad_state_to_float, ptr, sad_float_buf, frames);
    float_ptr = sad_float_buf;
    
    length = flow_execute(postproc_flow,
                          time,
                          &float_ptr,
                          len,
                          FMT_FLOAT,
                          decoder_srate,
                          nch);
    
    frames = length / nch / sizeof(float);
    len = frames * nch * FMT_SIZEOF(op_state.fmt);
    if(sad_out_buf == NULL || sad_out_buf_length < len) {
        sad_out_buf_length = len;
        sad_out_buf = smart_realloc(sad_out_buf, &sad_out_buf_length);
    }

    SAD_dither_process_buffer(sad_state_from_float, float_ptr, sad_out_buf, frames);

    length = len;
    ptr = sad_out_buf;
    
    if (op_state.fmt == FMT_S16_NE || (op_state.fmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) ||
                                      (op_state.fmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN)) {
        length = flow_execute(legacy_flow, time, &ptr, length, op_state.fmt, op_state.rate, op_state.nch);
    } else {
        AUDDBG("legacy_flow can deal only with S16_NE streams\n"); /*FIXME*/
    }

    /**** write it out ****/

    writeoffs = 0;
    while (writeoffs < length)
    {
        int writable = length - writeoffs;

        if (writable > 2048)
            writable = 2048;

        if (writable == 0)
            return;

        while (op->buffer_free() < writable)   /* wait output buf */
        {
            GTimeVal pb_abs_time;

            g_get_current_time(&pb_abs_time);
            g_time_val_add(&pb_abs_time, 10000);

            if (going && !*going)              /* thread stopped? */
                return;                        /* so finish */

            if (ip_data.stop)                  /* has a stop been requested? */
                return;                        /* yes, so finish */

            /* else sleep for retry */
            g_mutex_lock(playback->pb_change_mutex);
            g_cond_timed_wait(playback->pb_change_cond, playback->pb_change_mutex, &pb_abs_time);
            g_mutex_unlock(playback->pb_change_mutex);
        }

        if (ip_data.stop)
            return;

        if (going && !*going)                  /* thread stopped? */
            return;                            /* so finish */

        /* do output */
	plugin_set_current((Plugin *)op);
        op->write_audio(((guint8 *) ptr) + writeoffs, writable);

        writeoffs += writable;
    }
}

/* called by input plugin when RG info available --asphyx */
void
output_set_replaygain_info (InputPlayback *pb, ReplayGainInfo *rg_info)
{
    replay_gain_info = *rg_info;
    apply_replaygain_info(rg_info);
}

static void
apply_replaygain_info (ReplayGainInfo *rg_info)
{
    SAD_replaygain_mode mode;
    SAD_replaygain_info info;
    gboolean rg_enabled;
    gboolean album_mode;

    if(sad_state_from_float == NULL) {
        AUDDBG("SAD not initialized!\n");
        return;
    }
    
    rg_enabled = cfg.enable_replay_gain;
    album_mode = cfg.replay_gain_album;
    mode.clipping_prevention = cfg.enable_clipping_prevention;
    mode.hard_limit = FALSE;
    mode.adaptive_scaler = cfg.enable_adaptive_scaler;
    
    if(!rg_enabled) return;

    mode.mode = album_mode ? SAD_RG_ALBUM : SAD_RG_TRACK;
    mode.preamp = cfg.replay_gain_preamp;

    info.present = TRUE;
    info.track_gain = rg_info->track_gain;
    info.track_peak = rg_info->track_peak;
    info.album_gain = rg_info->album_gain;
    info.album_peak = rg_info->album_peak;

    AUDDBG("Applying Replay Gain settings:\n");
    AUDDBG("* mode:                %s\n",     mode.mode == SAD_RG_ALBUM ? "album" : "track");
    AUDDBG("* clipping prevention: %s\n",     mode.clipping_prevention ? "yes" : "no");
    AUDDBG("* adaptive scaler      %s\n",     mode.adaptive_scaler ? "yes" : "no");
    AUDDBG("* preamp:              %+f dB\n", mode.preamp);
    AUDDBG("Replay Gain info for current track:\n");
    AUDDBG("* track gain:          %+f dB\n", info.track_gain);
    AUDDBG("* track peak:          %f\n",     info.track_peak);
    AUDDBG("* album gain:          %+f dB\n", info.album_gain);
    AUDDBG("* album peak:          %f\n",     info.album_peak);
    
    SAD_dither_apply_replaygain(sad_state_from_float, &info, &mode);
}
