/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002-2005  Felipe Rivera <liebremx at users.sourceforge.net>
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
 *   $Id: iir.h,v 1.12 2005/10/17 01:57:59 liebremx Exp $
 */
#ifndef IIR_H
#define IIR_H

#include <glib.h>
#include "main.h"
#include "flow.h"
#include "iir_cfs.h"

/*
 * Flush-to-zero to avoid flooding the CPU with underflow exceptions 
 */
#ifdef SSE_MATH
#define FTZ 0x8000
#define FTZ_ON { \
    unsigned int mxcsr; \
  __asm__  __volatile__ ("stmxcsr %0" : "=m" (*&mxcsr)); \
  mxcsr |= FTZ; \
  __asm__  __volatile__ ("ldmxcsr %0" : : "m" (*&mxcsr)); \
}
#define FTZ_OFF { \
    unsigned int mxcsr; \
  __asm__  __volatile__ ("stmxcsr %0" : "=m" (*&mxcsr)); \
  mxcsr &= ~FTZ; \
  __asm__  __volatile__ ("ldmxcsr %0" : : "m" (*&mxcsr)); \
}
#else
#define FTZ_ON
#define FTZ_OFF
#endif

/*
 * Function prototypes
 */
extern void init_iir();
extern void clean_history();
extern void set_gain(gint index, gint chn, float val);
extern void set_preamp(gint chn, float val);


extern int iir(gpointer * d, gint length, gint nch);

#ifdef ARCH_X86
extern int round_trick(float floatvalue_to_round);
#endif
#ifdef ARCH_PPC
extern int round_ppc(float x);
#endif

#define EQ_CHANNELS 2
#define EQ_MAX_BANDS 10

extern float preamp[EQ_CHANNELS];
extern sIIRCoefficients *iir_cf;
extern gint rate;
extern gint band_count;

#ifdef BENCHMARK
extern double timex;
extern int count;
extern unsigned int blength;
#endif

void iir_flow(FlowContext *context);

#endif /* #define IIR_H */

