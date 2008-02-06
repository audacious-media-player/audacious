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

/* #define AUD_DEBUG */

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

#include "effect.h"
#include "volumecontrol.h"
#include "visualization.h"

#include "libSAD.h"

#include <math.h>

#ifdef USE_SRC
#include <samplerate.h>
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

static const struct {
    AFormat afmt;
    SAD_sample_format sadfmt;
} format_table[] = {
    {FMT_U8, SAD_SAMPLE_U8},
    {FMT_S8, SAD_SAMPLE_S8},

    {FMT_S16_LE, SAD_SAMPLE_S16_LE},
    {FMT_S16_BE, SAD_SAMPLE_S16_BE},
    {FMT_S16_NE, SAD_SAMPLE_S16},

    {FMT_U16_LE, SAD_SAMPLE_U16_LE},
    {FMT_U16_BE, SAD_SAMPLE_U16_BE},
    {FMT_U16_NE, SAD_SAMPLE_U16},

    {FMT_S24_LE, SAD_SAMPLE_S24_LE},
    {FMT_S24_BE, SAD_SAMPLE_S24_BE},
    {FMT_S24_NE, SAD_SAMPLE_S24},
    
    {FMT_U24_LE, SAD_SAMPLE_U24_LE},
    {FMT_U24_BE, SAD_SAMPLE_U24_BE},
    {FMT_U24_NE, SAD_SAMPLE_U24},

    {FMT_S32_LE, SAD_SAMPLE_S32_LE},
    {FMT_S32_BE, SAD_SAMPLE_S32_BE},
    {FMT_S32_NE, SAD_SAMPLE_S32},
    
    {FMT_U32_LE, SAD_SAMPLE_U32_LE},
    {FMT_U32_BE, SAD_SAMPLE_U32_BE},
    {FMT_U32_NE, SAD_SAMPLE_U32},
    
    {FMT_FLOAT, SAD_SAMPLE_FLOAT},
    {FMT_FIXED32, SAD_SAMPLE_FIXED32},
};

static void apply_replaygain_info (ReplayGainInfo *rg_info);

static inline unsigned sample_size(AFormat fmt) {
  switch(fmt) {
    case FMT_S8:
    case FMT_U8: return sizeof(gint8);
    case FMT_S16_NE:
    case FMT_S16_LE:
    case FMT_S16_BE:
    case FMT_U16_NE:
    case FMT_U16_LE:
    case FMT_U16_BE: return sizeof(gint16);
    case FMT_S24_NE:
    case FMT_S24_LE:
    case FMT_S24_BE:
    case FMT_U24_NE:
    case FMT_U24_LE:
    case FMT_U24_BE:
    case FMT_S32_NE:
    case FMT_S32_LE:
    case FMT_S32_BE:
    case FMT_U32_NE:
    case FMT_U32_LE:
    case FMT_U32_BE:
    case FMT_FIXED32: return sizeof(gint32);
    case FMT_FLOAT: return sizeof(float);
    default: return 0;
  }
}

static SAD_sample_format
sadfmt_from_afmt(AFormat fmt)
{
    int i;
    for (i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++) {
        if (format_table[i].afmt == fmt) return format_table[i].sadfmt;
    }

    return -1;
}

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
        out->about();
}

void
output_configure(gint i)
{
    OutputPlugin *out = g_list_nth(op_data.output_list, i)->data;
    if (out && out->configure)
        out->configure();
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
        op_data.current_output_plugin->get_volume(l, r);
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
        op_data.current_output_plugin->set_volume(l, r);
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

    return op->written_time();
}

/* called by input plugin to peek at the output plugin's output progress */
gint
get_output_time(void)
{
    OutputPlugin *op = get_current_output_plugin();

    return op->output_time();
}

#ifdef USE_SRC

static SRC_STATE *src_state;
static SRC_DATA src_data;
static int overSamplingFs = 96000;
static int converter_type = SRC_SINC_BEST_QUALITY;
static int srcError = 0;

static float *srcIn = NULL, *srcOut = NULL;
static short int *wOut = NULL;
static int lengthOfSrcIn = 0;
static int lengthOfSrcOut = 0;

static void freeSRC()
{
  if(src_state != NULL)
    src_state = src_delete(src_state);
  free(srcIn);
  free(srcOut);
  free(wOut);
  srcIn = NULL;
  srcOut = NULL;
  wOut = NULL;
  lengthOfSrcIn = 0;
  lengthOfSrcOut = 0;
}

#endif

static SAD_dither_t *sad_state = NULL;
static SAD_dither_t *sad_state_to_float = NULL;
static SAD_dither_t *sad_state_from_float = NULL;
static void *sad_out_buf = NULL;
static int sad_out_buf_length = 0;
static ReplayGainInfo replay_gain_info = {
    .track_gain = 0.0,
    .track_peak = 0.0,
    .album_gain = 0.0,
    .album_peak = 0.0,
};

static void
freeSAD()
{
    if (sad_state != NULL)            {SAD_dither_free(sad_state);            sad_state = NULL;}
    if (sad_state_from_float != NULL) {SAD_dither_free(sad_state_from_float); sad_state_from_float = NULL;}
    if (sad_state_to_float != NULL)   {SAD_dither_free(sad_state_to_float);   sad_state_to_float = NULL;}
    if (sad_out_buf != NULL)          {free(sad_out_buf);                     sad_out_buf = NULL; sad_out_buf_length = 0;}
}

gint
output_open_audio(AFormat fmt, gint rate, gint nch)
{
    gint ret;
    OutputPlugin *op;
    AUDDBG("\n");

    AFormat output_fmt;
    int bit_depth;
    SAD_buffer_format input_sad_fmt;
    SAD_buffer_format output_sad_fmt;

#ifdef USE_SRC
    gboolean src_enabled;
    gint src_rate, src_type;
    ConfigDb *db;
    
    db = cfg_db_open();
    
    if (cfg_db_get_bool(db, NULL, "enable_src", &src_enabled) == FALSE)
      src_enabled = FALSE;

    if (cfg_db_get_int(db, NULL, "src_rate", &src_rate) == FALSE)
      overSamplingFs = 48000;
    else
      overSamplingFs = src_rate;

    /* don't resample if sampling rates are the same --nenolod */
    if (rate == overSamplingFs)
      src_enabled = FALSE;

    if (cfg_db_get_int(db, NULL, "src_type", &src_type) == FALSE)
      converter_type = SRC_SINC_BEST_QUALITY;
    else
      converter_type = src_type;
    
    freeSRC();
    
    if(src_enabled)
      {
	src_state = src_new(converter_type, nch, &srcError);
	if (src_state != NULL)
	  {
	    src_data.src_ratio = (float)overSamplingFs/(float)rate;
	    rate = overSamplingFs;
	  }
	else
        {
	  fprintf(stderr, "src_new(): %s\n\n", src_strerror(srcError));
          src_enabled = FALSE;
        }
      }
    
    cfg_db_close(db);
#endif
    
    /*if (cfg_db_get_int(db, NULL, "output_bit_depth", &bit_depth) == FALSE) bit_depth = 16;*/
    bit_depth = cfg.output_bit_depth;

    AUDDBG("bit depth: %d\n", bit_depth);
    output_fmt = (bit_depth == 24) ? FMT_S24_NE : FMT_S16_NE;
    /*output_fmt = (bit_depth == 24) ? FMT_S24_LE : FMT_S16_LE;*/ /* no reason to support other output formats --asphyx */
    
    freeSAD();

#ifdef USE_SRC
    if (src_enabled) {
        AUDDBG("initializing dithering engine for 2 stage conversion\n");
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
    } else
#endif /* USE_SRC */
    {   /* needed for RG processing !*/
        AUDDBG("initializing dithering engine for direct conversion\n");

        input_sad_fmt.sample_format = sadfmt_from_afmt(fmt);
        if (input_sad_fmt.sample_format < 0) return FALSE;
        input_sad_fmt.fracbits = FMT_FRACBITS(fmt);
        input_sad_fmt.channels = nch;
        input_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
        input_sad_fmt.samplerate = 0; /* resampling not implemented yet in libSAD */
        
        output_sad_fmt.sample_format = sadfmt_from_afmt(output_fmt);
        output_sad_fmt.fracbits = FMT_FRACBITS(output_fmt);
        output_sad_fmt.channels = nch;
        output_sad_fmt.channels_order = SAD_CHORDER_INTERLEAVED;
        output_sad_fmt.samplerate = 0;

        sad_state = SAD_dither_init(&input_sad_fmt, &output_sad_fmt, &ret);
        if (sad_state == NULL) {
            AUDDBG("ditherer init failed\n");
            return FALSE;
        }
        SAD_dither_set_dither (sad_state, TRUE);

        fmt = output_fmt;
    }

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
        op->flush(0);
        return TRUE;
    }
    else if (op_state.rate != 0 && op_state.nch != 0)
        op->close_audio();

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

    op->write_audio(ptr, length);
}

void
output_close_audio(void)
{
    OutputPlugin *op = get_current_output_plugin();

    AUDDBG("\n");

#ifdef USE_SRC
    freeSRC();
#endif
    freeSAD();
    
    AUDDBG("clearing RG settings\n");
    replay_gain_info.track_gain = 0.0;
    replay_gain_info.track_peak = 0.0;
    replay_gain_info.album_gain = 0.0;
    replay_gain_info.album_peak = 0.0;

    /* Do not close if there are still songs to play and the user has 
     * not requested a stop.  --nenolod
     */
    if (ip_data.stop == FALSE && 
       (playlist_get_position_nolock(playlist_get_active()) < 
        playlist_get_length(playlist_get_active()) - 1))
        return;

    /* Sanity check. */
    if (op == NULL)
        return;

    op->close_audio();

    /* Reset the op_state. */
    op_state.fmt = op_state.rate = op_state.nch = 0;
}

void
output_flush(gint time)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return;

    op->flush(time);
}

void
output_pause(gshort paused)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return;

    op->pause(paused);
}

gint
output_buffer_free(void)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return 0;

    return op->buffer_free();
}

gint
output_buffer_playing(void)
{
    OutputPlugin *op = get_current_output_plugin();

    if (op == NULL)
        return 0;

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
    OutputPlugin *op = playback->output;
    gint writeoffs;

    if (length <= 0) return;
    gint time = playback->output->written_time();

    if (postproc_flow == NULL)
    {
        postproc_flow = flow_new();
        flow_link_element(postproc_flow, iir_flow);
        flow_link_element(postproc_flow, effect_flow);
        flow_link_element(postproc_flow, vis_flow);
        flow_link_element(postproc_flow, volumecontrol_flow);
    }

#ifdef USE_SRC
    if(src_state != NULL)
      {
        /*int lrLength = length / nch;*/
        int lrLength = length / sample_size(fmt);
        int overLrLength = (int)floor(lrLength*(src_data.src_ratio+1));
	if(lengthOfSrcIn < lrLength)
	  {
	    lengthOfSrcIn = lrLength;
	    free(srcIn);
	    srcIn = (float*)malloc(sizeof(float)*lrLength);
	  }
	if(lengthOfSrcOut < overLrLength)
	  {
	    lengthOfSrcOut = overLrLength;
	    free(srcOut);
	    free(wOut);
	    srcOut = (float*)malloc(sizeof(float)*overLrLength);
	    wOut = (short int*)malloc(sample_size(op_state.fmt) * overLrLength);
	  }
        /*src_short_to_float_array((short int*)ptr, srcIn, lrLength);*/
        SAD_dither_process_buffer(sad_state_to_float, ptr, srcIn, lrLength / nch);
        src_data.data_in = srcIn;
        src_data.data_out = srcOut;
        src_data.end_of_input = 0;
        src_data.input_frames = lrLength / nch;
        src_data.output_frames = overLrLength / nch;
        if ((srcError = src_process(src_state, &src_data)) > 0)
          {
            fprintf(stderr, "src_process(): %s\n", src_strerror(srcError));
          }
        else
          {
            /*src_float_to_short_array(srcOut, wOut, src_data.output_frames_gen*2);*/
            SAD_dither_process_buffer(sad_state_from_float, srcOut, wOut, src_data.output_frames_gen);
            ptr = wOut;
            length = src_data.output_frames_gen *  op_state.nch * sample_size(op_state.fmt);
          }
      } else
#endif
    if(sad_state != NULL) {
        int frames  = length / nch / sample_size(fmt);
        int len =  frames * op_state.nch * sample_size(op_state.fmt);
        if(sad_out_buf == NULL || sad_out_buf_length < len ) {
            if(sad_out_buf != NULL) free (sad_out_buf);
            sad_out_buf = malloc(len);
            sad_out_buf_length = len; 
        }
        SAD_dither_process_buffer(sad_state, ptr, sad_out_buf, frames);
        ptr = sad_out_buf;
        length = len;
    }
    
    if (op_state.fmt == FMT_S16_NE || (op_state.fmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) ||
                                      (op_state.fmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN)) {
        length = flow_execute(postproc_flow, time, &ptr, length, op_state.fmt, op_state.rate, op_state.nch);
    } else {
        AUDDBG("postproc_flow can deal only with S16_NE streams\n"); /*FIXME*/
    }

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
    /*ConfigDb *db;*/
    gboolean rg_enabled;
    gboolean album_mode;
    SAD_dither_t *active_state;


    if(sad_state == NULL && sad_state_from_float == NULL) {
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
    
    active_state = sad_state != NULL ? sad_state : sad_state_from_float;
    SAD_dither_apply_replaygain(active_state, &info, &mode);
}
