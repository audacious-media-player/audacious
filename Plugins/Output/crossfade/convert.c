/*
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

/*
 *  Convert to standard (16bit-le stereo)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "convert.h"

void
convert_init(convert_context_t *cc)
{
  memset(cc, 0, sizeof(*cc));
}

gint
convert_flow(convert_context_t *cc,
	     gpointer          *buffer,
	     gint               length,
	     format_t          *format)
{
  gpointer data;
  gint     size;
  gint     sample_size, sample_count;
  gint     element_count;
  gint16  *out, s;

  if(!cc) return 0;
  if(length <= 0) return 0;

  /* calculate sample count */
  sample_size  = format->is_8bit ? 1 : 2;
  sample_count = length / sample_size;
  if(sample_count == 0) return 0;

  /* calculate buffer size */
  size = sample_count * 2;
  if(format->nch == 1) size *= 2;

  /* resize buffer if necessary */
  if(!cc->data || (size > cc->size)) {
    if(!(data = g_realloc(cc->data, size))) {
      DEBUG(("[crossfade] convert: g_realloc(%d) failed!\n", size));
      return 0;
    }
    cc->data = data;
    cc->size = size;
  }

  /* calculate number of stereo samples */
  element_count = sample_count;
  if(format->nch == 2) element_count /= 2;
  
#define CONVERT(x)						\
  if(format->nch == 1) {					\
    while(sample_count--) { s = x; *out++ = s; *out++ = s; }	\
  }								\
  else {							\
    while(sample_count--) *out++ = x;				\
  }

  out = cc->data;
  if(format->is_8bit) {
    if(format->is_unsigned) {
      guint8 *in = *buffer;
      CONVERT((gint16)(*in++ ^ 128) << 8);
    }
    else {
      gint8 *in = *buffer;
      CONVERT((gint16)*in++ << 8);
    }
  }
  else {
    if(format->is_unsigned) {
      guint16 *in = *buffer;
      if(format->is_swapped) {
	CONVERT((gint16)(((*in & 0x00ff) << 8) | (*in >> 8)) ^ 32768); in++;
      } else {
	CONVERT((gint16)*in++ ^ 32768);
      }
    }
    else {
      gint16 *in = *buffer;
      if(format->is_swapped) {
	CONVERT(((*in & 0x00ff) << 8) | (*in >> 8)); in++;
      } else {
	if(format->nch == 1) {
	  CONVERT(*in++);
	}
	else memcpy(out, in, size);
      }
    }
  }
  *buffer = cc->data;

  return size;
}

void
convert_free(convert_context_t *cc)
{
  if(cc->data) {
    g_free(cc->data);
    cc->data = NULL;
  }
}

