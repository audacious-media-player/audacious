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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <inttypes.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef int SAD_error;

typedef enum {
  SAD_SAMPLE_S8,
  SAD_SAMPLE_U8,
  SAD_SAMPLE_S16, 
  SAD_SAMPLE_S16_LE, 
  SAD_SAMPLE_S16_BE, 
  SAD_SAMPLE_U16,
  SAD_SAMPLE_U16_LE,
  SAD_SAMPLE_U16_BE,
  SAD_SAMPLE_S24,
  SAD_SAMPLE_S24_LE,
  SAD_SAMPLE_S24_BE,
  SAD_SAMPLE_U24,
  SAD_SAMPLE_U24_LE,
  SAD_SAMPLE_U24_BE,
  SAD_SAMPLE_S32,
  SAD_SAMPLE_S32_LE,
  SAD_SAMPLE_S32_BE,
  SAD_SAMPLE_U32,
  SAD_SAMPLE_U32_LE,
  SAD_SAMPLE_U32_BE,
  SAD_SAMPLE_FIXED32,
  SAD_SAMPLE_FLOAT,
  SAD_SAMPLE_MAX /*EOF*/
} SAD_sample_format;

/* sample format -> sample size */
static inline unsigned sf2ss(SAD_sample_format fmt) {
  switch(fmt) {
    case SAD_SAMPLE_S8:
    case SAD_SAMPLE_U8: return sizeof(int8_t);
    case SAD_SAMPLE_S16:
    case SAD_SAMPLE_S16_LE:
    case SAD_SAMPLE_S16_BE:
    case SAD_SAMPLE_U16:
    case SAD_SAMPLE_U16_LE:
    case SAD_SAMPLE_U16_BE: return sizeof(int16_t);
    case SAD_SAMPLE_S24:
    case SAD_SAMPLE_S24_LE:
    case SAD_SAMPLE_S24_BE:
    case SAD_SAMPLE_U24:
    case SAD_SAMPLE_U24_LE:
    case SAD_SAMPLE_U24_BE:
    case SAD_SAMPLE_S32:
    case SAD_SAMPLE_S32_LE:
    case SAD_SAMPLE_S32_BE:
    case SAD_SAMPLE_U32:
    case SAD_SAMPLE_U32_LE:
    case SAD_SAMPLE_U32_BE:
    case SAD_SAMPLE_FIXED32: return sizeof(int32_t);
    case SAD_SAMPLE_FLOAT: return sizeof(float);
    default: return 0;
  }
}

typedef enum {
  SAD_CHORDER_INTERLEAVED,
  SAD_CHORDER_SEPARATED,
  SAD_CHORDER_MAX /*EOF*/
} SAD_channels_order;

typedef struct {
  SAD_sample_format sample_format;
  int fracbits; /* for fixed-point only */
  int channels;
  SAD_channels_order channels_order;
  int samplerate;
} SAD_buffer_format;

static inline unsigned bytes2frames(SAD_buffer_format *fmt, unsigned bytes) {
  return bytes / sf2ss(fmt->sample_format) / fmt->channels;
}

static inline unsigned frames2bytes(SAD_buffer_format *fmt, unsigned frames) {
  return frames * sf2ss(fmt->sample_format) * fmt->channels;
}

static inline float db2scale(float db) {
  return pow(10, db / 20);
}

enum {
  SAD_RG_NONE,
  SAD_RG_TRACK,
  SAD_RG_ALBUM
};

#define SAD_ERROR_OK 0
#define SAD_ERROR_FAIL -1

#ifdef DEBUG
#define DEBUG_MSG(f,x) {printf("debug: "f, x);}
#else
#define DEBUG_MSG(f,x) {}
#endif

typedef struct {
  int present;
  float track_gain; /* in dB !!! */
  float track_peak;
  float album_gain;
  float album_peak;
} SAD_replaygain_info;

typedef struct {
  int mode;
  int clipping_prevention;
  int hard_limit;
  int adaptive_scaler;
  float preamp; /* in dB ! */
} SAD_replaygain_mode;


#endif /* COMMON_H */

