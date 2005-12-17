/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002  Felipe Rivera <liebremx at users.sourceforge.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: iir.h,v 1.1 2003/05/21 13:59:59 liebremx Exp $
 */
#ifndef IIR_H
#define IIR_H

#include <stdio.h>
#include <string.h>

#include <audacious/plugin.h>
#include <libaudacious/util.h>

#define EQ_MAX_BANDS 10
/* Number of channels (Stereo) */
#define EQ_CHANNELS 2


// Fixed Point Fractional bits
#define FP_FRBITS 28	

// Conversions
#define EQ_REAL(x) ((gint)((x) * (1 << FP_FRBITS)))

/* Floating point */
typedef struct 
{
	float beta;
	float alpha; 
	float gamma;
}sIIRCoefficients;

/* Coefficient history for the IIR filter */
typedef struct
{
	float x[3]; /* x[n], x[n-1], x[n-2] */
	float y[3]; /* y[n], y[n-1], y[n-2] */
}sXYData;

/* Gain for each band
 * values should be between -0.2 and 1.0 */
float gain[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
/* Volume gain
 * values should be between 0.0 and 1.0
 * Use the preamp from XMMS for now
 * */
float preamp[EQ_CHANNELS] __attribute__((aligned));

__inline__ int iir(gpointer * d, gint length);
void init_iir();

#endif /* #define IIR_H */
