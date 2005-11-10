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
 *   $Id: iir.h,v 1.1.1.1 2004/12/19 12:48:13 bogorodskiy Exp $
 */
#ifndef IIR_H
#define IIR_H

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <pthread.h>
#include <string.h>

/* XMMS public headers */
#include <bmp/plugin.h>
#include <bmp/util.h>

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

/* BETA, ALPHA, GAMMA */
static sIIRCoefficients iir_cforiginal10[] __attribute__((aligned)) = {
    { (9.9421504945e-01), (2.8924752745e-03), (1.9941421835e+00) },    /*    60.0 Hz */
    { (9.8335039428e-01), (8.3248028618e-03), (1.9827686547e+00) },    /*   170.0 Hz */
    { (9.6958094144e-01), (1.5209529281e-02), (1.9676601546e+00) },    /*   310.0 Hz */
    { (9.4163923306e-01), (2.9180383468e-02), (1.9345490229e+00) },    /*   600.0 Hz */
    { (9.0450844499e-01), (4.7745777504e-02), (1.8852109613e+00) },    /*  1000.0 Hz */
    { (7.3940088234e-01), (1.3029955883e-01), (1.5829158753e+00) },    /*  3000.0 Hz */
    { (5.4697667908e-01), (2.2651166046e-01), (1.0153238114e+00) },    /*  6000.0 Hz */
    { (3.1023210589e-01), (3.4488394706e-01), (-1.8142472036e-01) },    /* 12000.0 Hz */
    { (2.6718639778e-01), (3.6640680111e-01), (-5.2117742267e-01) },    /* 14000.0 Hz */
    { (2.4201241845e-01), (3.7899379077e-01), (-8.0847117831e-01) },    /* 16000.0 Hz */
};

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
