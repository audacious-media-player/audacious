/* Scale & Dither library (libSAD)
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

/*#define CLIPPING_DEBUG*/
/*#define DITHER_DEBUG*/

#include "common.h"
#include "dither_ops.h"
#include "noicegen.h"

#include <assert.h>
#include <math.h>

/* 
 * Supported conversions:
 *
 *					O U T P U T
 *   ,------------------+-----------------------------------------------.
 *   |			|S8 U8 S16 U16 S24 U24 S32 U32 FLOAT FIXED-POINT|
 *   +------------------+-----------------------------------------------+
 *   | S8		|X  X  X   X   X   X   X   X   -     -          |
 *   | U8		|X  X  X   X   X   X   X   X   -     -          |
 * I | S16		|X  X  X   X   X   X   X   X   -     -          |
 * N | U16		|X  X  X   X   X   X   X   X   -     -          |
 * P | S24		|X  X  X   X   X   X   X   X   -     -          |
 * U | U24		|X  X  X   X   X   X   X   X   -     -          |
 * T | S32		|X  X  X   X   X   X   X   X   -     -          |
 *   | U32		|X  X  X   X   X   X   X   X   -     -          |
 *   | FLOAT		|X  X  X   X   X   X   X   X   X     -          |
 *   | FIXED-POINT	|X  X  X   X   X   X   X   X   X     -          |
 *   `------------------+-----------------------------------------------'
 */

#define SCALE(x,s) (s != 1.0 ? x * s : x)
#define MAXINT(a) (1L << ((a)-1))
#define CLIP(x,m) (x > m-1 ? m-1 : (x < -m ? -m : x))

/* private object */
typedef struct {
  SAD_sample_format input_sample_format;
  SAD_sample_format output_sample_format;
  int input_bits;
  int input_fracbits;
  int output_bits;
  int output_fracbits;
  int channels;
  SAD_channels_order input_chorder;
  SAD_channels_order output_chorder;
  SAD_get_sample_proc get_sample;
  SAD_put_sample_proc put_sample;
  int dither;
  int hardlimit;
  float scale;
  float rg_scale;
} SAD_state_priv;

/* error code */

//static SAD_error SAD_last_error = SAD_ERROR_OK;

static inline double compute_hardlimit (double sample, float scale) {
  sample *= scale;
  const double k = 0.5;    /* -6dBFS */
  if (sample > k) {
    return tanh((sample - k) / (1 - k)) * (1 - k) + k;
  }
  else if (sample < -k) {
    return tanh((sample + k) / (1 - k)) * (1 - k) - k;
  }
  return sample;
}

/* 
 * Dither fixed-point normalized or integer sample to n-bits integer
 * samples < -1 and > 1 will be clipped
 */

static inline int32_t __dither_sample_fixed_to_int (int32_t sample, int inbits, int fracbits, int outbits, float scale, int dither,
							int hardlimit)
{
  int n_bits_to_loose, bitwidth, precision_loss;
  int32_t maxint = MAXINT(outbits);

  n_bits_to_loose = 0;
  bitwidth = inbits;
  precision_loss = FALSE;

/*#ifdef DEEP_DEBUG
  printf("f: __dither_sample_fixed_to_int\n");
#endif*/

  if (fracbits == 0) {
    if (inbits<29) {
      /* convert to 4.28 fixed-point */
      n_bits_to_loose = 29 - inbits;
      sample <<= n_bits_to_loose;
      bitwidth += n_bits_to_loose;
    }

    n_bits_to_loose += inbits - outbits;

    if (inbits > outbits) {
      precision_loss = TRUE;
#ifdef PRECISION_DEBUG
      printf("Precision loss, reason: bitwidth loss %d --> %d\n", inbits, outbits);
#endif
    }
  } else {
    n_bits_to_loose = fracbits + 1 - outbits;
    bitwidth = fracbits;
    precision_loss = TRUE;
#ifdef PRECISION_DEBUG
    printf("Precision loss, reason: fixed-point input\n", inbits, outbits);
#endif
  }
  
  assert(n_bits_to_loose >=0 );

  if (hardlimit) {
    sample = (int32_t)(compute_hardlimit((double)sample/(double)MAXINT(bitwidth), scale) * (double)MAXINT(bitwidth));
#ifdef PRECISION_DEBUG
    printf("Precision loss, reason: hard limiter\n", inbits, outbits);
#endif
    precision_loss = TRUE;
  } else {
    sample = SCALE(sample, scale);
  }
 
  if (scale != 1.0){
    precision_loss = TRUE;
#ifdef PRECISION_DEBUG
    printf("Precision loss, reason: scale\n", inbits, outbits);
#endif
  }

  if (precision_loss && (n_bits_to_loose >= 1)) sample += (1L << (n_bits_to_loose - 1));

#ifdef DITHER_DEBUG
  int32_t val_wo_dither = sample >> n_bits_to_loose;
  val_wo_dither = CLIP(val_wo_dither, maxint);
#endif
  if (dither && precision_loss && (n_bits_to_loose >= 1)) {
    int32_t dither_num = triangular_dither_noise(n_bits_to_loose + 1);
    sample += dither_num;
  }

  sample >>= n_bits_to_loose;

  /* Clipping */
#ifdef CLIPPING_DEBUG
  int32_t val_wo_clip = sample;
#endif
  sample = CLIP(sample, maxint);
#ifdef CLIPPING_DEBUG
  if (val_wo_clip != sample) {
    printf("Clipping: %d --> %d\n", val_wo_clip, sample);
  }
#endif
#ifdef DITHER_DEBUG
  if (dither && precision_loss && (n_bits_to_loose >= 1)) printf("%d --> %d, noise: %d\n", val_wo_dither, sample, sample - val_wo_dither);
#endif
  return sample;
}

/* 
 * Dither floating-point normalized sample to n-bits integer
 * samples < -1 and > 1 will be clipped
 */
static inline int32_t __dither_sample_float_to_int (float sample, int nbits, float scale, int dither, int hardlimit) {

#ifdef DEEP_DEBUG
  printf("f: __dither_sample_float_to_int\n");
#endif

  int32_t maxint = MAXINT(nbits);

  if (hardlimit) {
    sample = compute_hardlimit((double)sample, scale);
  } else {
    sample = SCALE(sample, scale);
  }

  sample *= maxint;
  /* we want to round precisely */
  sample = (sample < 0 ? sample - 0.5 : sample + 0.5);

#ifdef DITHER_DEBUG
  int32_t val_wo_dither = (int32_t) sample;
  val_wo_dither = CLIP(val_wo_dither, maxint);
#endif
  if (dither) {
    float dither_num = triangular_dither_noise_f();
    sample += dither_num;
  }

  /* Round and clipping */
  int32_t value = (int32_t) sample;
#ifdef CLIPPING_DEBUG
  int32_t val_wo_clip = value;
#endif
  value = CLIP(value, maxint);
#ifdef CLIPPING_DEBUG
  if (val_wo_clip != value) {
    printf("Clipping: %d --> %d\n", val_wo_clip, value);
  }
#endif

#ifdef DITHER_DEBUG
  printf("%d --> %d, noise: %d\n", val_wo_dither, value, value - val_wo_dither);
#endif
  return value;
}

static inline float __dither_sample_float_to_float (float sample, float scale, int hardlimit) {
#ifdef DEEP_DEBUG
  printf("f: __dither_sample_float_to_float\n");
#endif
  if (hardlimit) {
    sample = compute_hardlimit((double)sample, scale);
  } else {
    sample = SCALE(sample, scale);
  }
  return sample;
}

static inline float __dither_sample_fixed_to_float (int32_t sample, int inbits, int fracbits, float scale, int hardlimit) {
  float fsample;

#ifdef DEEP_DEBUG
  printf("f: __dither_sample_fixed_to_float\n");
#endif
  if (fracbits == 0) {
     fsample = (float)sample / (float)MAXINT(inbits);
  } else {
     fsample = (float)sample / (float)MAXINT(fracbits+1);
  }
  return __dither_sample_float_to_float (fsample, scale, hardlimit);
}





SAD_dither_t* SAD_dither_init(SAD_buffer_format *inbuf_format, SAD_buffer_format *outbuf_format, int *error) {
  SAD_state_priv *priv;

  DEBUG_MSG("f: SAD_dither_init\n",0);

  priv = calloc(sizeof(SAD_state_priv), 1);

  /* Check buffer formats and assign buffer ops */
  SAD_buffer_ops* inops = SAD_assign_buf_ops(inbuf_format);

  if (inbuf_format->sample_format != SAD_SAMPLE_FLOAT) {
    if (inops != NULL) {
      priv->get_sample = inops->get_sample;
    } else {
      free(priv);
      *error = SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT;
      return NULL;
    }
  }

  SAD_buffer_ops* outops = SAD_assign_buf_ops(outbuf_format);

  if (outbuf_format->sample_format != SAD_SAMPLE_FLOAT) {
    if (outops != NULL) {
      priv->put_sample = outops->put_sample;
    } else {
      free(priv);
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

  switch(outbuf_format->sample_format){
    case SAD_SAMPLE_S8:
    case SAD_SAMPLE_U8: priv->output_bits = 8; break;
    case SAD_SAMPLE_S16:
    case SAD_SAMPLE_S16_LE:
    case SAD_SAMPLE_S16_BE:
    case SAD_SAMPLE_U16:
    case SAD_SAMPLE_U16_LE:
    case SAD_SAMPLE_U16_BE: priv->output_bits = 16; break;
    case SAD_SAMPLE_S24:
    case SAD_SAMPLE_S24_LE:
    case SAD_SAMPLE_S24_BE:
    case SAD_SAMPLE_U24:
    case SAD_SAMPLE_U24_LE:
    case SAD_SAMPLE_U24_BE: priv->output_bits = 24; break;
    case SAD_SAMPLE_S32:
    case SAD_SAMPLE_U32: priv->output_bits = 32; break;
    case SAD_SAMPLE_FLOAT: break;
    default:
      free(priv);
      *error = SAD_ERROR_INCORRECT_OUTPUT_SAMPLEFORMAT;
      return NULL;
  }

  switch(inbuf_format->sample_format){
    case SAD_SAMPLE_S8:
    case SAD_SAMPLE_U8: priv->input_bits = 8; break;
    case SAD_SAMPLE_S16:
    case SAD_SAMPLE_S16_LE:
    case SAD_SAMPLE_S16_BE:
    case SAD_SAMPLE_U16:
    case SAD_SAMPLE_U16_LE:
    case SAD_SAMPLE_U16_BE: priv->input_bits = 16; break;
    case SAD_SAMPLE_S24:
    case SAD_SAMPLE_S24_LE:
    case SAD_SAMPLE_S24_BE:
    case SAD_SAMPLE_U24:
    case SAD_SAMPLE_U24_LE:
    case SAD_SAMPLE_U24_BE: priv->input_bits = 24; break;
    case SAD_SAMPLE_S32:
    case SAD_SAMPLE_U32: priv->input_bits = 32; break;
    case SAD_SAMPLE_FIXED32: priv->input_fracbits = inbuf_format->fracbits; break;
    case SAD_SAMPLE_FLOAT: break;
    default:
      free(priv);
      *error = SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT;
      return NULL;
  }

  *error = SAD_ERROR_OK;
  return (SAD_dither_t*)priv;
}

int SAD_dither_free(SAD_dither_t* state) {
  DEBUG_MSG("f: SAD_dither_free\n",0);
  free(state);
  return SAD_ERROR_OK;
}

/* 
 * Depend on format->channels_order inbuf and outbuf will be treated as
 * smth* or smth** if channels_order = SAD_CHORDER_INTERLEAVED or SAD_CHORDER_SEPARATED
 * accordingly
 *
 * frame is aggregate of format->channels samples
 */

#define GET_FLOAT_SAMPLE(b,o,n,c,i) (o == SAD_CHORDER_INTERLEAVED ? (((float*)b)[i*n+c]) : (((float**)b)[c][i]))
#define PUT_FLOAT_SAMPLE(b,o,n,c,i,s) { \
    if (o == SAD_CHORDER_INTERLEAVED) { \
      ((float*)b)[i*n+c] = s;		\
    } else {				\
      ((float**)b)[c][i] = s;		\
    }					\
  }

int SAD_dither_process_buffer (SAD_dither_t *state, void *inbuf, void *outbuf, int frames)
{
  SAD_state_priv *priv = (SAD_state_priv*) state;
  int i, ch;
  int channels = priv->channels;
  int inbits = priv->input_bits;
  int outbits = priv->output_bits;
  int fracbits = priv->input_fracbits;
  float scale = priv->scale * priv->rg_scale;
  int dither = priv->dither;
  int hardlimit = priv->hardlimit;
  SAD_channels_order input_chorder = priv->input_chorder;
  SAD_channels_order output_chorder = priv->output_chorder;

  SAD_get_sample_proc get_sample = priv->get_sample;
  SAD_put_sample_proc put_sample = priv->put_sample;

#ifdef DEEP_DEBUG
  printf("f: SAD_process_buffer\n");
#endif

  if (priv->input_sample_format == SAD_SAMPLE_FLOAT) {
      if (priv->output_sample_format == SAD_SAMPLE_FLOAT) {
          /* process buffer */
          for(i=0; i<frames; i++) {
	      for(ch=0; ch<channels; ch++) {
	          float sample = GET_FLOAT_SAMPLE(inbuf, input_chorder, channels, ch ,i);
	          sample = __dither_sample_float_to_float(sample, scale, hardlimit);
                  PUT_FLOAT_SAMPLE(outbuf, output_chorder, channels, ch ,i, sample);
	      }
	  }
      } else {
          if (put_sample == NULL) return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
          /* process buffer */
          for(i=0; i<frames; i++) {
	      for(ch=0; ch<channels; ch++) {
	          float sample = GET_FLOAT_SAMPLE(inbuf, input_chorder, channels, ch ,i);
	          int32_t isample = __dither_sample_float_to_int(sample, outbits, scale, dither, hardlimit);
                  put_sample (outbuf, isample, channels, ch, i);
	      }
	  }
      }
  } else {
      if (priv->output_sample_format == SAD_SAMPLE_FLOAT) {
          if (get_sample == NULL) return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
          /* process buffer */
          for(i=0; i<frames; i++) {
	      for(ch=0; ch<channels; ch++) {
  	          int32_t sample = get_sample (inbuf, channels, ch, i);
                  float fsample = __dither_sample_fixed_to_float (sample, inbits, fracbits, scale, hardlimit);
                  PUT_FLOAT_SAMPLE(outbuf, output_chorder, channels, ch ,i, fsample);
	      }
	  }
      } else {
          if (put_sample == NULL || get_sample == NULL) return SAD_ERROR_CORRUPTED_PRIVATE_DATA;
          /* process buffer */
          for(i=0; i<frames; i++) {
	      for(ch=0; ch<channels; ch++){
  	          int32_t sample = get_sample (inbuf, channels, ch, i);
	          int32_t isample = __dither_sample_fixed_to_int (sample, inbits, fracbits, outbits, scale, dither, hardlimit);
                  put_sample (outbuf, isample, channels, ch, i);
	      }
	  }
      }
  }

  return SAD_ERROR_OK;
}

int SAD_dither_apply_replaygain (SAD_dither_t *state, SAD_replaygain_info *rg_info, SAD_replaygain_mode *mode) {
  SAD_state_priv *priv = (SAD_state_priv*) state;
  float scale = -1.0, peak = 0.0;

  DEBUG_MSG("f: SAD_dither_apply_replaygain\n",0);
  
  if(!rg_info->present) {
    priv->rg_scale = 1.0;
    priv->hardlimit = FALSE;
    return SAD_ERROR_OK;
  }

  switch(mode->mode) {
    case SAD_RG_ALBUM:
      scale = db2scale(rg_info->album_gain);
      peak = rg_info->album_peak;
      if (peak == 0.0) {
        scale = db2scale(rg_info->track_gain); // fallback to per-track mode
	peak = rg_info->track_peak;
	DEBUG_MSG("f: SAD_dither_apply_replaygain: fallback to track mode\n",0);
      }
      break;
    case SAD_RG_TRACK:
      scale = db2scale(rg_info->track_gain);
      peak = rg_info->track_peak;
      if (peak == 0.0) {
        scale = db2scale(rg_info->album_gain); // fallback to per-album mode
	peak = rg_info->album_peak;
	DEBUG_MSG("f: SAD_dither_apply_replaygain: fallback to album mode\n",0);
      }
      break;
    case SAD_RG_NONE:
      scale = -1.0;
  }
  
  if (scale != -1.0 && peak != 0.0) {
    DEBUG_MSG("f: SAD_dither_apply_replaygain: applying\n",0);
    scale *= db2scale(mode->preamp);
    // Clipping prevention
    if(mode->clipping_prevention) {
#ifdef DEBUG
      if(scale * peak > 1.0) DEBUG_MSG("f: SAD_dither_apply_replaygain: clipping prevented\n",0);
#endif
      scale = scale * peak > 1.0 ? 1.0 / peak : scale;
    }
    scale = scale > 15.0 ? 15.0 : scale; // safety
    priv->rg_scale = scale;
    priv->hardlimit = mode->hard_limit; // apply settings
  } else {
    priv->rg_scale = 1.0;
    priv->hardlimit = FALSE;
  }

  return SAD_ERROR_OK;
}

int SAD_dither_set_scale (SAD_dither_t *state, float scale) {
  SAD_state_priv *priv = (SAD_state_priv*) state;
  priv->scale = scale;
  return SAD_ERROR_OK;
}

int SAD_dither_set_dither (SAD_dither_t *state, int dither) {
  SAD_state_priv *priv = (SAD_state_priv*) state;
  priv->dither = dither;
  return SAD_ERROR_OK;
}

void SAD_dither_init_rand(uint32_t seed) {
  noicegen_init_rand(seed);
}
