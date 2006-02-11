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

#ifndef __RATE_H__
#define __RATE_H__

#include <glib.h>

#ifdef HAVE_LIBSAMPLERATE
#  include <samplerate.h>
#endif

#include "volume.h"

typedef struct               /* private state */
{
	gboolean valid;            /* TRUE after the context has been set up */
	gint     in_rate;          /* sample rate (Hz) */
	gint     out_rate;
	gint16  *data;             /* pointer to output buffer (16bit stereo) */
	gint     size;             /* size in bytes */

	volume_context_t vc;       /* for clipping warnings */ 

#ifdef HAVE_LIBSAMPLERATE
	int        converter_type;
	SRC_STATE *src_state;
	SRC_DATA   src_data;       /* struct for I/O with SRC api */
	gint       src_in_size, src_out_size;
#else
	guint32  lcm_rate;         /* lcm(in_rate, out_rate) */
	guint32  in_skip;          /* distance to next sample (lcm-units) */
	guint32  out_skip;         
	guint32  in_ofs;           /* offset (lcm-units) */
	guint32  out_ofs;

	gboolean started;          /* TRUE after the first sample has been parsed */
	gint16   last_l;           /* last input sample */
	gint16   last_r;
#endif
}
rate_context_t;

void rate_init  (rate_context_t *rc);
void rate_config(rate_context_t *rc,
	               gint            in_rate,
	               gint            out_rate,
	               int             converter_type);
gint rate_flow  (rate_context_t *rc,
	               gpointer       *buffer,
	               gint            length);
void rate_free  (rate_context_t *rc);

#endif  /* _RATE_H_ */
