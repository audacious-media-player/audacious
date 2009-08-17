/*
 * Scale & Dither library (libSAD)
 * High-precision bit depth converter with ReplayGain support
 *
 * Copyright (c) 2007-2008 Eugene Zagidullin (e.asphyx@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* #define CLIPPING_DEBUG
#define DEBUG
#define DITHER_DEBUG */

#include "common.h"
#include "dither_ops.h"
#include "noicegen.h"
#include "debug.h"

#include <assert.h>
#include <math.h>

/* Supported precision / format conversions:
 *
 *                    O U T P U T
 *   ,------------------+--------------------------------------.
 *   |             |S8 U8 S16 U16 S24 U24 S32 U32 FLOAT FIXPNT |
 *   +------------------+--------------------------------------+
 *   | S8          |X  X  X   X   X   X   X   X   -     -      |
 *   | U8          |X  X  X   X   X   X   X   X   -     -      |
 * I | S16         |X  X  X   X   X   X   X   X   -     -      |
 * N | U16         |X  X  X   X   X   X   X   X   -     -      |
 * P | S24         |X  X  X   X   X   X   X   X   -     -      |
 * U | U24         |X  X  X   X   X   X   X   X   -     -      |
 * T | S32         |X  X  X   X   X   X   X   X   -     -      |
 *   | U32         |X  X  X   X   X   X   X   X   -     -      |
 *   | FLOAT       |X  X  X   X   X   X   X   X   X     -      |
 *   | FIXED-POINT |X  X  X   X   X   X   X   X   X     -      |
 *   `------------------+--------------------------------------'
 */

#define SAD_SCALE(x,s) (s != 1.0 ? x * s : x)
#define SAD_MAXINT(a) ((1 << ((a) - 1)) - 1)
#define SAD_CLIP(x,m) (x > m-1 ? m-1 : (x < -m ? -m : x))

#define ADJUSTMENT_COEFFICIENT 0.1

/* private object */
typedef struct
{
    SAD_sample_format input_sample_format;
    SAD_sample_format output_sample_format;
    gint input_bits;
    gint input_fracbits;
    gint output_bits;
    gint output_fracbits;
    gint channels;
    SAD_channels_order input_chorder;
    SAD_channels_order output_chorder;
    SAD_get_sample_proc get_sample;
    SAD_put_sample_proc put_sample;
    gint dither;
    gint hardlimit;
    gdouble scale;
    gdouble rg_scale;
    gint adaptive_scaler;
} SAD_state_priv;

/* error code */

//static SAD_error SAD_last_error = SAD_ERROR_OK;

static inline gdouble compute_hardlimit(gdouble sample, gdouble scale)
{
    sample *= scale;
    const gdouble k = 0.5;      /* -6dBFS */
    if (sample > k)
    {
        return tanh((sample - k) / (1 - k)) * (1 - k) + k;
    }
    else if (sample < -k)
    {
        return tanh((sample + k) / (1 - k)) * (1 - k) - k;
    }
    return sample;
}

/*
 * Dither fixed-point normalized or integer sample to n-bits integer
 * samples < -1 and > 1 will be clipped
 */

static inline gint32 __dither_sample_fixed_to_int(gint32 sample, gint inbits, gint fracbits, gint outbits, gdouble * scale, gint dither, gint hardlimit, gint adaptive_scale)
{
    gint n_bits_to_loose, bitwidth, precision_loss;
    gint32 maxint = SAD_MAXINT(outbits);

    n_bits_to_loose = 0;
    bitwidth = inbits;
    precision_loss = FALSE;

/*#ifdef DEEP_DEBUG
  printf("f: __dither_sample_fixed_to_int\n");
#endif*/

    if (fracbits == 0)
    {
        if (inbits < 29)
        {
            /* convert to 4.28 fixed-point */
            n_bits_to_loose = 29 - inbits;
            sample <<= n_bits_to_loose;
            bitwidth += n_bits_to_loose;
        }

        n_bits_to_loose += inbits - outbits;

        if (inbits > outbits)
        {
            precision_loss = TRUE;
#ifdef PRECISION_DEBUG
            printf("Precision loss, reason: bitwidth loss %d --> %d\n", inbits, outbits);
#endif
        }
    }
    else
    {
        n_bits_to_loose = fracbits + 1 - outbits;
        bitwidth = fracbits;
        precision_loss = TRUE;
#ifdef PRECISION_DEBUG
        printf("Precision loss, reason: fixed-point input\n", inbits, outbits);
#endif
    }

    assert(n_bits_to_loose >= 0);

    /* adaptive scaler */
    if (adaptive_scale)
    {
        gint sam = sample >> n_bits_to_loose;
        gdouble d_sam = fabs((double)sam) / (double)(maxint - 1);
        if (d_sam * *scale > 1.0)
        {
#ifdef CLIPPING_DEBUG
            printf("sample val %d, scale factor adjusted %f --> ", sam, *scale);
#endif
            *scale -= (*scale - 1.0 / d_sam) * ADJUSTMENT_COEFFICIENT;
#ifdef CLIPPING_DEBUG
            printf("%f\n", *scale);
#endif
        }
        sample = SAD_SCALE(sample, *scale);
    }
    else
  /*****************/
    if (hardlimit)
    {
        sample = (gint32) (compute_hardlimit((double)sample / (double)SAD_MAXINT(bitwidth), *scale) * (double)SAD_MAXINT(bitwidth));
#ifdef PRECISION_DEBUG
        printf("Precision loss, reason: hard limiter\n", inbits, outbits);
#endif
        precision_loss = TRUE;
    }
    else
    {
        sample = SAD_SCALE(sample, *scale);
    }

    if (*scale != 1.0)
    {
        precision_loss = TRUE;
#ifdef PRECISION_DEBUG
        printf("Precision loss, reason: scale\n", inbits, outbits);
#endif
    }

    if (precision_loss && (n_bits_to_loose >= 1) && (inbits < 32 || fracbits != 0))
        sample += (1L << (n_bits_to_loose - 1));

#ifdef DITHER_DEBUG
    gint32 val_wo_dither = sample >> n_bits_to_loose;
    val_wo_dither = SAD_CLIP(val_wo_dither, maxint);
#endif
    if (dither && precision_loss && (n_bits_to_loose >= 1) && (inbits < 32 || fracbits != 0))
    {
        gint32 dither_num = triangular_dither_noise(n_bits_to_loose + 1);
        sample += dither_num;
    }

    sample >>= n_bits_to_loose;

    /* Clipping */
#ifdef CLIPPING_DEBUG
    gint32 val_wo_clip = sample;
#endif
    sample = SAD_CLIP(sample, maxint);
#ifdef CLIPPING_DEBUG
    if (val_wo_clip != sample)
    {
        printf("Clipping: %d --> %d\n", val_wo_clip, sample);
    }
#endif
#ifdef DITHER_DEBUG
    if (dither && precision_loss && (n_bits_to_loose >= 1))
        printf("%d --> %d, noise: %d\n", val_wo_dither, sample, sample - val_wo_dither);
#endif
    return sample;
}

/*
 * Dither floating-point normalized sample to n-bits integer
 * samples < -1 and > 1 will be clipped
 */
static inline gint32 __dither_sample_float_to_int(gfloat sample, gint nbits, gdouble * scale, gint dither, gint hardlimit, gint adaptive_scale)
{
#ifdef DEEP_DEBUG
    printf("f: __dither_sample_float_to_int\n");
#endif

    gint32 maxint = SAD_MAXINT(nbits);

    /* adaptive scaler */
    if (adaptive_scale)
    {
        if (fabs(sample) * *scale > 1.0)
        {
#ifdef CLIPPING_DEBUG
            printf("sample val %f, scale factor adjusted %f --> ", sample, *scale);
#endif
            *scale -= (*scale - 1.0 / sample) * ADJUSTMENT_COEFFICIENT;
#ifdef CLIPPING_DEBUG
            printf("%f\n", *scale);
#endif
        }
        sample = SAD_SCALE(sample, *scale);
    }
    else
  /*****************/
    if (hardlimit)
    {
        sample = compute_hardlimit((double)sample, *scale);
    }
    else
    {
        sample = SAD_SCALE(sample, *scale);
    }

    sample *= maxint;
    /* we want to round precisely */
    sample = (sample < 0 ? sample - 0.5 : sample + 0.5);

#ifdef DITHER_DEBUG
    gint32 val_wo_dither = (gint32) sample;
    val_wo_dither = SAD_CLIP(val_wo_dither, maxint);
#endif
    if (dither)
    {
        gdouble dither_num = triangular_dither_noise_f();
        sample += dither_num;
    }

    /* Round and clipping */
    gint32 value = (gint32) sample;
#ifdef CLIPPING_DEBUG
    gint32 val_wo_clip = value;
#endif
    value = SAD_CLIP(value, maxint);
#ifdef CLIPPING_DEBUG
    if (val_wo_clip != value)
    {
        printf("Clipping: %d --> %d\n", val_wo_clip, value);
    }
#endif

#ifdef DITHER_DEBUG
    printf("%d --> %d, noise: %d\n", val_wo_dither, value, value - val_wo_dither);
#endif
    return value;
}

static inline gfloat __dither_sample_float_to_float(gfloat sample, gdouble scale, gint hardlimit)
{
#ifdef DEEP_DEBUG
    printf("f: __dither_sample_float_to_float\n");
#endif
    if (hardlimit)
    {
        sample = compute_hardlimit((double)sample, scale);
    }
    else
    {
        sample = SAD_SCALE(sample, scale);
    }
    return sample;
}

static inline gfloat __dither_sample_fixed_to_float(gint32 sample, gint inbits, gint fracbits, gdouble scale, gint hardlimit)
{
    gfloat fsample;

#ifdef DEEP_DEBUG
    printf("f: __dither_sample_fixed_to_float\n");
#endif
    if (fracbits == 0)
    {
        fsample = (double)sample / (double)SAD_MAXINT(inbits);
    }
    else
    {
        fsample = (double)sample / (double)SAD_MAXINT(fracbits + 1);
    }
    return __dither_sample_float_to_float(fsample, scale, hardlimit);
}


SAD_dither_t *SAD_dither_init(SAD_buffer_format * inbuf_format, SAD_buffer_format * outbuf_format, gint * error)
{
    SAD_state_priv *priv;

    DEBUG_MSG("f: SAD_dither_init\n", 0);

    priv = g_new0(SAD_state_priv, 1);

    /* Check buffer formats and assign buffer ops */
    SAD_buffer_ops *inops = SAD_assign_buf_ops(inbuf_format);

    if (inbuf_format->sample_format != SAD_SAMPLE_FLOAT)
    {
        if (inops != NULL)
        {
            priv->get_sample = inops->get_sample;
        }
        else
        {
            g_free(priv);
            *error = SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT;
            return NULL;
        }
    }

    SAD_buffer_ops *outops = SAD_assign_buf_ops(outbuf_format);

    if (outbuf_format->sample_format != SAD_SAMPLE_FLOAT)
    {
        if (outops != NULL)
        {
            priv->put_sample = outops->put_sample;
        }
        else
        {
            g_free(priv);
            *error = SAD_ERROR_INCORRECT_OUTPUT_SAMPLEFORMAT;
            return NULL;
        }
    }

    priv->input_fracbits = 0;
    priv->output_fracbits = 0;
    priv->input_sample_format = inbuf_format->sample_format;
    priv->output_sample_format = outbuf_format->sample_format;
    priv->input_chorder = inbuf_format->channels_order;
    priv->output_chorder = outbuf_format->channels_order;
    priv->channels = inbuf_format->channels;
    priv->scale = 1.0;
    priv->rg_scale = 1.0;
    priv->dither = TRUE;
    priv->hardlimit = FALSE;
    priv->adaptive_scaler = FALSE;

    switch (outbuf_format->sample_format)
    {
      case SAD_SAMPLE_S8:
      case SAD_SAMPLE_U8:
          priv->output_bits = 8;
          break;
      case SAD_SAMPLE_S16:
      case SAD_SAMPLE_S16_LE:
      case SAD_SAMPLE_S16_BE:
      case SAD_SAMPLE_U16:
      case SAD_SAMPLE_U16_LE:
      case SAD_SAMPLE_U16_BE:
          priv->output_bits = 16;
          break;
      case SAD_SAMPLE_S24:
      case SAD_SAMPLE_S24_LE:
      case SAD_SAMPLE_S24_BE:
      case SAD_SAMPLE_U24:
      case SAD_SAMPLE_U24_LE:
      case SAD_SAMPLE_U24_BE:
          priv->output_bits = 24;
          break;
      case SAD_SAMPLE_S32:
      case SAD_SAMPLE_S32_LE:
      case SAD_SAMPLE_S32_BE:
      case SAD_SAMPLE_U32:
      case SAD_SAMPLE_U32_LE:
      case SAD_SAMPLE_U32_BE:
          priv->output_bits = 32;
          break;
      case SAD_SAMPLE_FLOAT:
          break;
      default:
          g_free(priv);
          *error = SAD_ERROR_INCORRECT_OUTPUT_SAMPLEFORMAT;
          return NULL;
    }

    switch (inbuf_format->sample_format)
    {
      case SAD_SAMPLE_S8:
      case SAD_SAMPLE_U8:
          priv->input_bits = 8;
          break;
      case SAD_SAMPLE_S16:
      case SAD_SAMPLE_S16_LE:
      case SAD_SAMPLE_S16_BE:
      case SAD_SAMPLE_U16:
      case SAD_SAMPLE_U16_LE:
      case SAD_SAMPLE_U16_BE:
          priv->input_bits = 16;
          break;
      case SAD_SAMPLE_S24:
      case SAD_SAMPLE_S24_LE:
      case SAD_SAMPLE_S24_BE:
      case SAD_SAMPLE_U24:
      case SAD_SAMPLE_U24_LE:
      case SAD_SAMPLE_U24_BE:
          priv->input_bits = 24;
          break;
      case SAD_SAMPLE_S32:
      case SAD_SAMPLE_S32_LE:
      case SAD_SAMPLE_S32_BE:
      case SAD_SAMPLE_U32:
      case SAD_SAMPLE_U32_LE:
      case SAD_SAMPLE_U32_BE:
          priv->input_bits = 32;
          break;
      case SAD_SAMPLE_FIXED32:
          priv->input_fracbits = inbuf_format->fracbits;
          break;
      case SAD_SAMPLE_FLOAT:
          break;
      default:
          g_free(priv);
          *error = SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT;
          return NULL;
    }

    *error = SAD_ERROR_OK;
    return (SAD_dither_t *) priv;
}

gint SAD_dither_free(SAD_dither_t * state)
{
    DEBUG_MSG("f: SAD_dither_free\n", 0);
    g_free(state);
    return SAD_ERROR_OK;
}

/*
 * Depend on format->channels_order inbuf and outbuf will be treated as
 * smth* or smth** if channels_order = SAD_CHORDER_INTERLEAVED or SAD_CHORDER_SEPARATED
 * accordingly
 *
 * frame is aggregate of format->channels samples
 */

#define GET_FLOAT_SAMPLE(b,o,n,c,i)     \
    (o == SAD_CHORDER_INTERLEAVED ? (((gfloat*)b)[i*n+c]) : (((gfloat**)b)[c][i]))

#define PUT_FLOAT_SAMPLE(b,o,n,c,i,s) { \
    if (o == SAD_CHORDER_INTERLEAVED) { \
        ((float*)b)[i*n+c] = s;            \
    } else {                            \
        ((float**)b)[c][i] = s;            \
    }                                    \
}

gint SAD_dither_process_buffer(SAD_dither_t * state, void *inbuf, void *outbuf, gint frames)
{
    SAD_state_priv *priv = (SAD_state_priv *) state;
    gint i, ch;
    gint channels = priv->channels;
    gint inbits = priv->input_bits;
    gint outbits = priv->output_bits;
    gint fracbits = priv->input_fracbits;
    gdouble scale = priv->scale * priv->rg_scale;
    gdouble oldscale = scale;
    gint dither = priv->dither;
    gint hardlimit = priv->hardlimit;
    gint adaptive_scale = priv->adaptive_scaler;
    SAD_channels_order input_chorder = priv->input_chorder;
    SAD_channels_order output_chorder = priv->output_chorder;

    SAD_get_sample_proc get_sample = priv->get_sample;
    SAD_put_sample_proc put_sample = priv->put_sample;

#ifdef DEEP_DEBUG
    printf("f: SAD_process_buffer\n");
#endif

    if (priv->input_sample_format == SAD_SAMPLE_FLOAT)
    {
        if (priv->output_sample_format == SAD_SAMPLE_FLOAT)
        {
            /* process buffer */
            for (i = 0; i < frames; i++)
            {
                for (ch = 0; ch < channels; ch++)
                {
                    gfloat sample = GET_FLOAT_SAMPLE(inbuf, input_chorder, channels, ch, i);
                    sample = __dither_sample_float_to_float(sample, scale, hardlimit);
                    PUT_FLOAT_SAMPLE(outbuf, output_chorder, channels, ch, i, sample);
                }
            }
        }
        else
        {
            if (put_sample == NULL)
                return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
            /* process buffer */
            for (i = 0; i < frames; i++)
            {
                for (ch = 0; ch < channels; ch++)
                {
                    gfloat sample = GET_FLOAT_SAMPLE(inbuf, input_chorder, channels, ch, i);
                    gint32 isample = __dither_sample_float_to_int(sample, outbits, &scale, dither, hardlimit, adaptive_scale);
                    put_sample(outbuf, isample, channels, ch, i);
                }
            }
        }
    }
    else
    {
        if (priv->output_sample_format == SAD_SAMPLE_FLOAT)
        {
            if (get_sample == NULL)
                return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
            /* process buffer */
            for (i = 0; i < frames; i++)
            {
                for (ch = 0; ch < channels; ch++)
                {
                    gint32 sample = get_sample(inbuf, channels, ch, i);
                    gfloat fsample = __dither_sample_fixed_to_float(sample, inbits, fracbits, scale, hardlimit);
                    PUT_FLOAT_SAMPLE(outbuf, output_chorder, channels, ch, i, fsample);
                }
            }
        }
        else
        {
            if (put_sample == NULL || get_sample == NULL)
                return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
            /* process buffer */
            for (i = 0; i < frames; i++)
            {
                for (ch = 0; ch < channels; ch++)
                {
                    gint32 sample = get_sample(inbuf, channels, ch, i);
                    gint32 isample = __dither_sample_fixed_to_int(sample, inbits, fracbits, outbits, &scale, dither, hardlimit, adaptive_scale);
                    put_sample(outbuf, isample, channels, ch, i);
                }
            }
        }
    }

    /* recalc scale factor */
    if (adaptive_scale && oldscale != scale)
        priv->rg_scale = scale / priv->scale;

    return SAD_ERROR_OK;
}

gint SAD_dither_apply_replaygain(SAD_dither_t * state, SAD_replaygain_info * rg_info, SAD_replaygain_mode * mode)
{
    SAD_state_priv *priv = (SAD_state_priv *) state;
    gdouble scale = -1.0, peak = 0.0;

    DEBUG_MSG("f: SAD_dither_apply_replaygain\n", 0);

    if (!rg_info->present)
    {
        priv->rg_scale = 1.0;
        priv->hardlimit = FALSE;
        return SAD_ERROR_OK;
    }

    switch (mode->mode)
    {
      case SAD_RG_ALBUM:
          scale = db2scale(rg_info->album_gain);
          peak = rg_info->album_peak;
          if (peak == 0.0)
          {
              scale = db2scale(rg_info->track_gain);    // fallback to per-track mode
              peak = rg_info->track_peak;
              DEBUG_MSG("f: SAD_dither_apply_replaygain: fallback to track mode\n", 0);
          }
          break;
      case SAD_RG_TRACK:
          scale = db2scale(rg_info->track_gain);
          peak = rg_info->track_peak;
          if (peak == 0.0)
          {
              scale = db2scale(rg_info->album_gain);    // fallback to per-album mode
              peak = rg_info->album_peak;
              DEBUG_MSG("f: SAD_dither_apply_replaygain: fallback to album mode\n", 0);
          }
          break;
      case SAD_RG_NONE:
          scale = -1.0;
    }

    if (scale != -1.0 && peak != 0.0)
    {
        DEBUG_MSG("f: SAD_dither_apply_replaygain: applying\n", 0);
        scale *= db2scale(mode->preamp);
        // Clipping prevention
        if (mode->clipping_prevention)
        {
#ifdef DEBUG
            if (scale * peak > 1.0)
                DEBUG_MSG("f: SAD_dither_apply_replaygain: clipping prevented\n", 0);
#endif
            scale = scale * peak > 1.0 ? 1.0 / peak : scale;
            DEBUG_MSG("f: SAD_dither_apply_replaygain: new scale %f\n", scale);
        }
        scale = scale > 15.0 ? 15.0 : scale;    // safety
        priv->rg_scale = scale;
        priv->hardlimit = mode->hard_limit;     // apply settings
        priv->adaptive_scaler = mode->adaptive_scaler;
    }
    else
    {
        priv->rg_scale = 1.0;
        priv->hardlimit = FALSE;
        priv->adaptive_scaler = FALSE;  // apply settings
    }

    return SAD_ERROR_OK;
}

gint SAD_dither_set_scale(SAD_dither_t * state, gfloat scale)
{
    SAD_state_priv *priv = (SAD_state_priv *) state;
    priv->scale = scale;
    return SAD_ERROR_OK;
}

gint SAD_dither_set_dither(SAD_dither_t * state, gint dither)
{
    SAD_state_priv *priv = (SAD_state_priv *) state;
    priv->dither = dither;
    return SAD_ERROR_OK;
}

void SAD_dither_init_rand(guint32 seed)
{
    noicegen_init_rand(seed);
}
