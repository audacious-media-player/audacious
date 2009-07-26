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

#ifndef LIBSAD_DITHER_H
#define LIBSAD_DITHER_H

#include "common.h"

#define SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT -2
#define SAD_ERROR_INCORRECT_OUTPUT_SAMPLEFORMAT -3
#define SAD_ERROR_CORRUPTED_PRIVATE_DATA -4

typedef gint32(*SAD_get_sample_proc) (void *buf, gint nch, gint ch, gint i);
typedef void (*SAD_put_sample_proc) (void *buf, gint32 sample, gint nch, gint ch, gint i);

typedef struct
{
    SAD_get_sample_proc get_sample;
    SAD_put_sample_proc put_sample;
} SAD_buffer_ops;

/* private data */
typedef struct
{
} SAD_dither_t;

void SAD_dither_init_rand(guint32 seed);

SAD_dither_t *SAD_dither_init(SAD_buffer_format * inbuf_format, SAD_buffer_format * outbuf_format, gint * error);
gint SAD_dither_free(SAD_dither_t * state);
gint SAD_dither_process_buffer(SAD_dither_t * state, void *inbuf, void *outbuf, gint frames);
gint SAD_dither_apply_replaygain(SAD_dither_t * state, SAD_replaygain_info * rg_info, SAD_replaygain_mode * mode);
gint SAD_dither_set_scale(SAD_dither_t * state, gfloat scale);
gint SAD_dither_set_dither(SAD_dither_t * state, gint dither);

#endif
