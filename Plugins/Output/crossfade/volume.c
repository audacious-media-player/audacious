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
 *  Volume to standard (16bit-le stereo)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "volume.h"
#include <string.h>
#include <math.h>		/* for rintf */

#undef VERBOSE

static void
set_factor(volume_context_t * vc, gfloat factor)
{
	vc->factor = factor;
#if 0
	if (vc->factor < 0.01f)
		vc->factor = 0.01f;
	if (vc->factor > 4.0f)
		vc->factor = 4.0f;
#endif
#ifdef VERBOSE
	DEBUG(("[crossfade] volume_set_factor: new factor=%.3f\n", vc->factor));
#endif
}


void
volume_init(volume_context_t * vc)
{
	memset(vc, 0, sizeof(*vc));
	vc->active = FALSE;
}

void
volume_set_active(volume_context_t * vc, gboolean active)
{
	vc->active = active;
#ifdef VERBOSE
	DEBUG(("[crossfade] volume_set_active: active=%d\n", vc->active));
#endif
}

void
volume_set_target_rms(volume_context_t * vc, gint target_rms)
{
	vc->target_rms = target_rms;
	if (vc->active && (vc->song_rms == 0))
	{
		DEBUG(("[crossfade] volume_set_target_rms: WARNING: song_rms=0!\n"));
		vc->factor = 1;
		return;
	}
	set_factor(vc, (gfloat) vc->target_rms / (gfloat) vc->song_rms);
}

void
volume_set_song_rms(volume_context_t * vc, gint song_rms)
{
	vc->song_rms = song_rms;
	set_factor(vc, (gfloat) vc->target_rms / (gfloat) vc->song_rms);
}

gfloat
volume_compute_factor(gint percent, gint dB_range)
{
	if (percent >= 100)
		return 1;
	if (percent <= 0)
		return 0;
	gfloat dB = (percent - 100) / 100.0 * dB_range;
	return pow(10, dB / 20);
}

void
volume_flow(volume_context_t * vc, gpointer * buffer, gint length)
{
	gint16 *in = *buffer;
	struct timeval tv;
	glong dt;

	if (!vc->active)
		return;

	length /= 2;
	while (length--)
	{
		gint out = (gint) rintf((gfloat) * in * vc->factor);
		if (out > 32767)
		{
			*in++ = 32767;
			vc->clips++;
		}
		else if (out < -32768)
		{
			*in++ = -32768;
			vc->clips++;
		}
		else
			*in++ = out;
	}

	gettimeofday(&tv, NULL);
	dt = (tv.tv_sec - vc->tv_last.tv_sec) * 1000 + (tv.tv_usec - vc->tv_last.tv_usec) / 1000;

	if (((dt < 0) || (dt > 1000)) && (vc->clips > 0))
	{
		DEBUG(("[crossfade] volume_flow: %d samples clipped!\n", vc->clips));
		vc->clips = 0;
		vc->tv_last = tv;
	}
}

void
volume_free(volume_context_t * vc)
{
}
