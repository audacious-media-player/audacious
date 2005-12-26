/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002  Felipe Rivera <liebremx at users sourceforge net>
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
 *   $Id: iir.c,v 1.5 2004/06/20 18:48:54 mderezynski Exp $
 */

#include "equalizer.h"
#include "main.h"
#include <math.h>
#include <string.h>
#include "output.h"

#include "iir.h"

// Fixed Point Fractional bits
#define FP_FRBITS 28

// Conversions
#define EQ_REAL(x) ((gint)((x) * (1 << FP_FRBITS)))

/* Floating point */
typedef struct {
    float beta;
    float alpha;
    float gamma;
} sIIRCoefficients;

/* Coefficient history for the IIR filter */
typedef struct {
    float x[3];                 /* x[n], x[n-1], x[n-2] */
    float y[3];                 /* y[n], y[n-1], y[n-2] */
} sXYData;

/* BETA, ALPHA, GAMMA */
static sIIRCoefficients iir_cforiginal10[] = {
    {(9.9421504945e-01), (2.8924752745e-03), (1.9941421835e+00)},   /*    60.0 Hz */
    {(9.8335039428e-01), (8.3248028618e-03), (1.9827686547e+00)},   /*   170.0 Hz */
    {(9.6958094144e-01), (1.5209529281e-02), (1.9676601546e+00)},   /*   310.0 Hz */
    {(9.4163923306e-01), (2.9180383468e-02), (1.9345490229e+00)},   /*   600.0 Hz */
    {(9.0450844499e-01), (4.7745777504e-02), (1.8852109613e+00)},   /*  1000.0 Hz */
    {(7.3940088234e-01), (1.3029955883e-01), (1.5829158753e+00)},   /*  3000.0 Hz */
    {(5.4697667908e-01), (2.2651166046e-01), (1.0153238114e+00)},   /*  6000.0 Hz */
    {(3.1023210589e-01), (3.4488394706e-01), (-1.8142472036e-01)},  /* 12000.0 Hz */
    {(2.6718639778e-01), (3.6640680111e-01), (-5.2117742267e-01)},  /* 14000.0 Hz */
    {(2.4201241845e-01), (3.7899379077e-01), (-8.0847117831e-01)},  /* 16000.0 Hz */
};

/* History for two filters */
static sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS];
static sXYData data_history2[EQ_MAX_BANDS][EQ_CHANNELS];

/* Coefficients */
static sIIRCoefficients *iir_cf;

/* Gain for each band
 * values should be between -0.2 and 1.0 */
float gain[10];
float preamp;

int round_trick(float floatvalue_to_round);

/* Init the filter */
void
init_iir()
{
    iir_cf = iir_cforiginal10;

    /* Zero the history arrays */
    memset(data_history, 0, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
    memset(data_history2, 0, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);

    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

int
iir(gpointer * d, gint length)
{
    gint16 *data = (gint16 *) * d;
    static gint i = 0, j = 2, k = 1;

    gint index, band, channel;
    gint tempgint, halflength;
    float out[EQ_CHANNELS], pcm[EQ_CHANNELS];

    halflength = (length >> 1);
    for (index = 0; index < halflength; index += 2) {
        /* For each channel */
        for (channel = 0; channel < EQ_CHANNELS; channel++) {
            /* No need to scale when processing the PCM with the filter */
            pcm[channel] = data[index + channel];

            /* Preamp gain */
            pcm[channel] *= preamp;

            out[channel] = 0;
            /* For each band */
            for (band = 0; band < 10; band++) {
                /* Store Xi(n) */
                data_history[band][channel].x[i] = pcm[channel];
                /* Calculate and store Yi(n) */
                data_history[band][channel].y[i] =
                    (iir_cf[band].alpha * (data_history[band][channel].x[i]
                                           - data_history[band][channel].x[k])
                     + iir_cf[band].gamma * data_history[band][channel].y[j]
                     - iir_cf[band].beta * data_history[band][channel].y[k]);
                /*
                 * The multiplication by 2.0 was 'moved' into the coefficients to save
                 * CPU cycles here */
                /* Apply the gain  */
                out[channel] += data_history[band][channel].y[i] * gain[band];
            }                   /* For each band */

            if (cfg.eq_extra_filtering) {
                /* Filter the sample again */
                for (band = 0; band < 10; band++) {
                    /* Store Xi(n) */
                    data_history2[band][channel].x[i] = out[channel];
                    /* Calculate and store Yi(n) */
                    data_history2[band][channel].y[i] =
                        (iir_cf[band].alpha *
                         (data_history2[band][channel].x[i]
                          - data_history2[band][channel].x[k])
                         +
                         iir_cf[band].gamma *
                         data_history2[band][channel].y[j]
                         -
                         iir_cf[band].beta * data_history2[band][channel].y[k]
                        );
                    /* Apply the gain */
                    out[channel] +=
                        data_history2[band][channel].y[i] * gain[band];
                }               /* For each band */
            }

            /* Volume stuff
               Scale down original PCM sample and add it to the filters
               output. This substitutes the multiplication by 0.25
             */

            out[channel] += (data[index + channel]);
            tempgint = (int) out[channel];

            if (tempgint < -32768)
                data[index + channel] = -32768;
            else if (tempgint > 32767)
                data[index + channel] = 32767;
            else
                data[index + channel] = tempgint;
        }

        i++;
        j++;
        k++;

        /* Wrap around the indexes */
        if (i == 3)
            i = 0;
        else if (j == 3)
            j = 0;
        else
            k = 0;
    }

    return length;
}
