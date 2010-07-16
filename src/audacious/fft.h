/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Copyright (C) 1999 Richard Boulton <richard@tartarus.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

/* fft.h: header for iterative implementation of a FFT */

#ifndef AUDACIOUS_FFT_H
#define AUDACIOUS_FFT_H

#include <glib.h>

#define FFT_BUFFER_SIZE_LOG 9
#define FFT_BUFFER_SIZE (1 << FFT_BUFFER_SIZE_LOG)

/* sound sample - should be a signed 16 bit value */
typedef gint16 sound_sample;

typedef struct _struct_fft_state fft_state;
fft_state *fft_init(void);
void fft_perform(const sound_sample * input, float *output, fft_state * state);
void fft_close(fft_state * state);

#endif /* AUDACIOUS_FFT_H */
