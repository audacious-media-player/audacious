/*
 * equalizer.c
 * Copyright 2001 Anders Johansson
 * Copyright 2010-2015 John Lindgren
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

#include "equalizer.h"
#include "internal.h"

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

#include "audio.h"
#include "audstrings.h"
#include "hook.h"
#include "runtime.h"

/* Q value for band-pass filters 1.2247 = (3/2)^(1/2)
 * Gives 4 dB suppression at Fc*2 and Fc/2 */
#define Q 1.2247449

/* Center frequencies for band-pass filters (Hz) */
/* These are not the historical WinAmp frequencies, because the IIR filters used
 * here are designed for each frequency to be twice the previous.  Using WinAmp
 * frequencies leads to too much gain in some bands and too little in others. */
static const float CF[AUD_EQ_NBANDS] = {31.25, 62.5, 125, 250, 500, 1000, 2000,
 4000, 8000, 16000};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool active;
static int channels, rate;
static float a[AUD_EQ_NBANDS][2]; /* A weights */
static float b[AUD_EQ_NBANDS][2]; /* B weights */
static float wqv[AUD_MAX_CHANNELS][AUD_EQ_NBANDS][2]; /* Circular buffer for W data */
static float gv[AUD_MAX_CHANNELS][AUD_EQ_NBANDS]; /* Gain factor for each channel and band */
static int K; /* Number of used EQ bands */

/* 2nd order band-pass filter design */
static void bp2 (float *a, float *b, float fc)
{
    float th = 2 * M_PI * fc;
    float C = (1 - tanf (th * Q / 2)) / (1 + tanf (th * Q / 2));

    a[0] = (1 + C) * cosf (th);
    a[1] = -C;
    b[0] = (1 - C) / 2;
    b[1] = -1.005;
}

void eq_set_format (int new_channels, int new_rate)
{
    pthread_mutex_lock (& mutex);

    channels = new_channels;
    rate = new_rate;

    /* Calculate number of active filters: the center frequency must be less
     * than rate/2Q to avoid singularities in the tangent used in bp2() */
    K = AUD_EQ_NBANDS;

    while (K > 0 && CF[K - 1] > (float) rate / (2.005 * Q))
        K --;

    /* Generate filter taps */
    for (int k = 0; k < K; k ++)
        bp2 (a[k], b[k], CF[k] / (float) rate);

    /* Reset state */
    memset (wqv[0][0], 0, sizeof wqv);

    pthread_mutex_unlock (& mutex);
}

static void eq_set_bands_real (double preamp, double *values)
{
    float adj[AUD_EQ_NBANDS];

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        adj[i] = preamp + values[i];

    for (int c = 0; c < AUD_MAX_CHANNELS; c ++)
    {
        for (int i = 0; i < AUD_EQ_NBANDS; i ++)
            gv[c][i] = pow (10, adj[i] / 20) - 1;
    }
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

    active = aud_get_bool (nullptr, "equalizer_active");

    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);
    eq_set_bands_real (aud_get_double (nullptr, "equalizer_preamp"), values);

    pthread_mutex_unlock (& mutex);
}

void eq_init ()
{
    eq_update (nullptr, nullptr);
    hook_associate ("set equalizer_active", eq_update, nullptr);
    hook_associate ("set equalizer_preamp", eq_update, nullptr);
    hook_associate ("set equalizer_bands", eq_update, nullptr);
}

void eq_cleanup ()
{
    hook_dissociate ("set equalizer_active", eq_update);
    hook_dissociate ("set equalizer_preamp", eq_update);
    hook_dissociate ("set equalizer_bands", eq_update);
}

EXPORT void aud_eq_set_bands (const double values[AUD_EQ_NBANDS])
{
    StringBuf string = double_array_to_str (values, AUD_EQ_NBANDS);
    aud_set_str (nullptr, "equalizer_bands", string);
}

EXPORT void aud_eq_get_bands (double values[AUD_EQ_NBANDS])
{
    memset (values, 0, sizeof (double) * AUD_EQ_NBANDS);
    String string = aud_get_str (nullptr, "equalizer_bands");
    str_to_double_array (string, values, AUD_EQ_NBANDS);
}

EXPORT void aud_eq_set_band (int band, double value)
{
    assert (band >= 0 && band < AUD_EQ_NBANDS);

    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);
    values[band] = value;
    aud_eq_set_bands (values);
}

EXPORT double aud_eq_get_band (int band)
{
    assert (band >= 0 && band < AUD_EQ_NBANDS);

    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);
    return values[band];
}

EXPORT void aud_eq_apply_preset (const EqualizerPreset & preset)
{
    double bands[AUD_EQ_NBANDS];

    /* convert float to double :( */
    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        bands[i] = preset.bands[i];

    aud_eq_set_bands (bands);
    aud_set_double (nullptr, "equalizer_preamp", preset.preamp);
}

EXPORT void aud_eq_update_preset (EqualizerPreset & preset)
{
    double bands[AUD_EQ_NBANDS];
    aud_eq_get_bands (bands);

    /* convert double to float :( */
    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        preset.bands[i] = bands[i];

    preset.preamp = aud_get_double (nullptr, "equalizer_preamp");
}
