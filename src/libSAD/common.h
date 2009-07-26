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

#ifndef LIBSAD_COMMON_H
#define LIBSAD_COMMON_H

#include <glib.h>
#include <math.h>

#ifdef _AUDACIOUS_CORE
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

typedef gint SAD_error;

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
static inline guint sf2ss(SAD_sample_format fmt) {
  switch(fmt) {
    case SAD_SAMPLE_S8:
    case SAD_SAMPLE_U8: return sizeof(gint8);
    case SAD_SAMPLE_S16:
    case SAD_SAMPLE_S16_LE:
    case SAD_SAMPLE_S16_BE:
    case SAD_SAMPLE_U16:
    case SAD_SAMPLE_U16_LE:
    case SAD_SAMPLE_U16_BE: return sizeof(gint16);
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
    case SAD_SAMPLE_FIXED32: return sizeof(gint32);
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
  gint fracbits; /* for fixed-point only */
  gint channels;
  SAD_channels_order channels_order;
  gint samplerate;
} SAD_buffer_format;

static inline guint bytes2frames(SAD_buffer_format *fmt, guint bytes) {
  return bytes / sf2ss(fmt->sample_format) / fmt->channels;
}

static inline guint frames2bytes(SAD_buffer_format *fmt, guint frames) {
  return frames * sf2ss(fmt->sample_format) * fmt->channels;
}

static inline gfloat db2scale(gfloat db) {
  return pow(10, db / 20);
}

enum {
  SAD_RG_NONE,
  SAD_RG_TRACK,
  SAD_RG_ALBUM
};

#define SAD_ERROR_OK 0
#define SAD_ERROR_FAIL -1

typedef struct {
  gint present;
  gfloat track_gain; /* in dB !!! */
  gfloat track_peak;
  gfloat album_gain;
  gfloat album_peak;
} SAD_replaygain_info;

typedef struct {
  gint mode;
  gint clipping_prevention;
  gint hard_limit;
  gint adaptive_scaler;
  gfloat preamp; /* in dB ! */
} SAD_replaygain_mode;


#endif /* LIBSAD_COMMON_H */
