/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002-2005  Felipe Rivera <liebremx at users sourceforge net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *   $Id: iir.c,v 1.15 2005/10/17 01:57:59 liebremx Exp $
 */

#include <math.h>

#include "main.h"
#include "iir.h"

/* Coefficients */
sIIRCoefficients *iir_cf;

/* Volume gain 
 * values should be between 0.0 and 1.0 
 * Use the preamp from XMMS for now
 * */
float preamp[EQ_CHANNELS];

#ifdef BENCHMARK
#include "benchmark.h"
double timex = 0.0;
int count = 0;
unsigned int blength = 0;
#endif

/* 
 * Global vars
 */
gint rate;
int band_count;

void set_preamp(gint chn, float val)
{
  preamp[chn] = val;
}

/* Init the filters */
void init_iir()
{
  calc_coeffs();
#if 0
  band_count = cfg.band_num;
#endif

  band_count = 10;

  rate = 44100;

  iir_cf = get_coeffs(&band_count, rate, TRUE);
  clean_history();
}

#ifdef ARCH_X86
/* Round function provided by Frank Klemm which saves around 100K
 * CPU cycles in my PIII for each call to the IIR function with 4K samples
 */
__inline__ int round_trick(float floatvalue_to_round)
{
  float   floattmp ;
  int     rounded_value ;

  floattmp      = (int) 0x00FD8000L + (floatvalue_to_round);
  rounded_value = *(int*)(&floattmp) - (int)0x4B7D8000L;

  if ( rounded_value != (short) rounded_value )
    rounded_value = ( rounded_value >> 31 ) ^ 0x7FFF;
  return rounded_value;
}
#endif

static void
byteswap(size_t size,
         gint16 * buf)
{
    gint16 *it;
    size &= ~1;                  /* must be multiple of 2  */
    for (it = buf; it < buf + size / 2; ++it)
        *(guint16 *) it = GUINT16_SWAP_LE_BE(*(guint16 *) it);
}

void
iir_flow(FlowContext *context)
{
    static int init = 0;
    int swapped = 0;
    guint myorder = G_BYTE_ORDER == G_LITTLE_ENDIAN ? FMT_S16_LE : FMT_S16_BE;
    int caneq = (context->fmt == FMT_S16_NE || context->fmt == myorder);

    if (!caneq && cfg.equalizer_active) {         /* wrong byte order */
        byteswap(context->len, context->data);    /* so convert */
        ++swapped;
        ++caneq;
    }                                        /* can eq now, mark swapd */
    else if (caneq && !cfg.equalizer_active) /* right order but no eq */
        caneq = 0;                           /* so don't eq */

    if (caneq) {                /* if eq enab */
        if (!init) {            /* if first run */
            init_iir();         /* then init eq */
            ++init;
        }

        iir(&context->data, context->len, context->channels);

        if (swapped)                               /* if was swapped */
            byteswap(context->len, context->data); /* swap back for output */
    }
}
