/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Iterative FFT implementation found in this file is
 * Copyright (C) 1999 Richard Boulton <richard@tartarus.org>
 * Richard gave permission to relicense the FFT code under the LGPL.
 * 
 * Authors: Richard Boulton <richard@tartarus.org>
 * 	    Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_FFT_H
#define _LV_FFT_H

#include <libvisual/lv_common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_FFTSTATE(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisFFTState))

/**
 * Private FFT define that contains the log size.
 */
#define VISUAL_FFT_BUFFER_SIZE_LOG 9

/**
 * Private FFT define that contains the buffer size.
 */
#define VISUAL_FFT_BUFFER_SIZE (1 << VISUAL_FFT_BUFFER_SIZE_LOG)

typedef struct _VisFFTState VisFFTState;

/**
 * Private structure to contain Fast Fourier Transform states in.
 */
struct _VisFFTState {
	VisObject	object;				/**< The VisObject data. */
	/* Temporary data stores to perform FFT in. */
	float		real[VISUAL_FFT_BUFFER_SIZE];	/**< Private data that is used by the FFT engine. */
	float		imag[VISUAL_FFT_BUFFER_SIZE];	/**< Private data that is used by the FFT engine. */
};

VisFFTState *visual_fft_init (void);
void visual_fft_perform (int16_t *input, float *output, VisFFTState *state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_FFT_H */
