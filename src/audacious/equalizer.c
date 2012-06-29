/*
 * equalizer.c
 * Copyright 2001 Anders Johansson
 * Copyright 2010-2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/*
 * Anders Johansson prefers float *ptr; formatting. Please keep it that way.
 *    - tallica
 */

#include <glib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "equalizer.h"
#include "misc.h"
#include "types.h"

#define EQ_BANDS AUD_EQUALIZER_NBANDS
#define MAX_CHANNELS 10

/* Q value for band-pass filters 1.2247 = (3/2)^(1/2)
 * Gives 4 dB suppression at Fc*2 and Fc/2 */
#define Q 1.2247449

/* Center frequencies for band-pass filters (Hz) */
/* These are not the historical WinAmp frequencies, because the IIR filters used
 * here are designed for each frequency to be twice the previous.  Using WinAmp
 * frequencies leads to too much gain in some bands and too little in others. */
static const float CF[EQ_BANDS] = {31.25, 62.5, 125, 250, 500, 1000, 2000,
 4000, 8000, 16000};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool_t active;
static int channels, rate;
static float a[EQ_BANDS][2]; /* A weights */
static float b[EQ_BANDS][2]; /* B weights */
static float wqv[MAX_CHANNELS][EQ_BANDS][2]; /* Circular buffer for W data */
static float gv[MAX_CHANNELS][EQ_BANDS]; /* Gain factor for each channel and band */
static int K; /* Number of used eq bands */

/* 2nd order band-pass filter design */
static void bp2 (float *a, float *b, float fc, float q)
{
    float th = 2 * M_PI * fc;
    float C = (1 - tanf (th * q / 2)) / (1 + tanf (th * q / 2));

    a[0] = (1 + C) * cosf (th);
    a[1] = -C;
    b[0] = (1 - C) / 2;
    b[1] = -1.005;
}

void eq_set_format (int new_channels, int new_rate)
{
    int k;

    pthread_mutex_lock (& mutex);

    channels = new_channels;
    rate = new_rate;

    /* Calculate number of active filters */
    K = EQ_BANDS;

    while (CF[K - 1] > (float) rate / 2.2)
        K --;

    /* Generate filter taps */
    for (k = 0; k < K; k ++)
        bp2 (a[k], b[k], CF[k] / (float) rate, Q);

    /* Reset state */
    memset (wqv[0][0], 0, sizeof wqv);

    pthread_mutex_unlock (& mutex);
}

static void eq_set_bands_real (double preamp, double *values)
{
    float adj[EQ_BANDS];
    for (int i = 0; i < EQ_BANDS; i ++)
        adj[i] = preamp + values[i];

    for (int c = 0; c < MAX_CHANNELS; c ++)
    for (int i = 0; i < EQ_BANDS; i ++)
        gv[c][i] = pow (10, adj[i] / 20) - 1;
}

void eq_filter (float *data, int samples)
{
    int channel;

    pthread_mutex_lock (& mutex);

    if (! active)
    {
        pthread_mutex_unlock (& mutex);
        return;
    }

    for (channel = 0; channel < channels; channel ++)
    {
        float *g = gv[channel]; /* Gain factor */
        float *end = data + samples;
        float *f;

        for (f = data + channel; f < end; f += channels)
        {
            int k; /* Frequency band index */
            float yt = *f; /* Current input sample */

            for (k = 0; k < K; k ++)
            {
                /* Pointer to circular buffer wq */
                float *wq = wqv[channel][k];
                /* Calculate output from AR part of current filter */
                float w = yt * b[k][0] + wq[0] * a[k][0] + wq[1] * a[k][1];

                /* Calculate output from MA part of current filter */
                yt += (w + wq[1] * b[k][1]) * g[k];

                /* Update circular buffer */
                wq[1] = wq[0];
                wq[0] = w;
            }

            /* Calculate output */
            *f = yt;
        }
    }

    pthread_mutex_unlock (& mutex);
}

static void eq_update (void *data, void *user)
{
    pthread_mutex_lock (& mutex);

    active = get_bool (NULL, "equalizer_active");

    double values[EQ_BANDS];
    eq_get_bands (values);
    eq_set_bands_real (get_double (NULL, "equalizer_preamp"), values);

    pthread_mutex_unlock (& mutex);
}

void eq_init (void)
{
    eq_update (NULL, NULL);
    hook_associate ("set equalizer_active", eq_update, NULL);
    hook_associate ("set equalizer_preamp", eq_update, NULL);
    hook_associate ("set equalizer_bands", eq_update, NULL);
}

void eq_cleanup (void)
{
    hook_dissociate ("set equalizer_active", eq_update);
    hook_dissociate ("set equalizer_preamp", eq_update);
    hook_dissociate ("set equalizer_bands", eq_update);
}

void eq_set_bands (const double *values)
{
    char *string = double_array_to_string (values, EQ_BANDS);
    g_return_if_fail (string);
    set_string (NULL, "equalizer_bands", string);
    g_free (string);
}

void eq_get_bands (double *values)
{
    memset (values, 0, sizeof (double) * EQ_BANDS);
    char *string = get_string (NULL, "equalizer_bands");
    string_to_double_array (string, values, EQ_BANDS);
    g_free (string);
}

void eq_set_band (int band, double value)
{
    g_return_if_fail (band >= 0 && band < EQ_BANDS);
    double values[EQ_BANDS];
    eq_get_bands (values);
    values[band] = value;
    eq_set_bands (values);
}

double eq_get_band (int band)
{
    g_return_val_if_fail (band >= 0 && band < EQ_BANDS, 0);
    double values[EQ_BANDS];
    eq_get_bands (values);
    return values[band];
}
