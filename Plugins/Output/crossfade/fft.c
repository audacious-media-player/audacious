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
 *  FFT/volume playground. Does not do anything usefull yet.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "fft.h"

#include <math.h>
#ifdef HAVE_RFFTW_H
#  include <rfftw.h>
#endif


void
fft_init(fft_context_t * fftc)
{
	DEBUG(("[crossfade] fft_init: fftc=0x%08lx\n", (glong) fftc));
	if (!fftc)
		return;

	memset(fftc, 0, sizeof(*fftc));
	fftc->num_bands = 32;
	fftc->plan = rfftw_create_plan(fftc->num_bands, FFTW_FORWARD, FFTW_ESTIMATE);
	fftc->valid = TRUE;
}

void
fft_flow(fft_context_t * fftc, gpointer buffer, gint length)
{
	gint isamp;
	gint16 *in = buffer;

	/* some sanity checks */
	if (length & 3)
	{
		DEBUG(("[crossfade] fft_flow: truncating %d bytes!", length & 3));
		length &= -4;
	}
	isamp = length / 4;
	if ((isamp <= 0) || !fftc)
		return;

	while (isamp--)
	{
		gint16 val = (gint16) (((glong) * in++ + *in++) / 2);
		if (val < 0)
			val = -val;

		if ((fftc->used == 0) || (val > fftc->smooth[fftc->used - 1]))
		{
			int i, u;
			if (fftc->used < FFT_SMOOTH_SIZE)
			{
				fftc->smooth[fftc->used] = 0;
				fftc->used++;
			}
			for (i = fftc->used - 1; (i > 0) && (val > fftc->smooth[i - 1]); i--);
			for (u = fftc->used - 1; u > i; u--)
				fftc->smooth[i] = fftc->smooth[i - 1];
			fftc->smooth[i] = val;
		}

		if (fftc->ival++ == FFT_INTERVAL)
		{
			int i, vu;

			if (fftc->used)
			{
				int sum = 0;
				for (i = 0; i < fftc->used; i++)
					sum += fftc->smooth[i];
				vu = (gint) ((gfloat) sum / fftc->used / 32768.0 * 100.0);
				fftc->used = 0;
			}
			else
				vu = 0;

			DEBUG(("[crossfade] fft_flow: %03d %s\n", vu,
			       "****************************************************************************************************"
			       + 100 - MIN(vu, 100)));

			fftc->used = 0;
			fftc->ival = 0;
		}
	}
}

void
fft_free(fft_context_t * fftc)
{
	if (fftc->valid)
		rfftw_destroy_plan(fftc->plan);
	memset(fftc, 0, sizeof(*fftc));
}
