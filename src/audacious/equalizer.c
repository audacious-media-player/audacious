/*
 *  Equalizer filter, implementation of a 10 band time domain graphic equalizer
 *  using IIR filters.  The IIR filters are implemented using a Direct Form II
 *  approach, modified (b1 == 0 always) to save computation.
 *
 *  This software has been released under the terms of the GNU General Public
 *  license.  See http://www.gnu.org/copyleft/gpl.html for details.
 *
 *  Copyright 2001 Anders Johansson <ajh@atri.curtin.edu.au>
 *
 *  Adapted for Audacious by John Lindgren, 2010-2011
 */

#include <glib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "equalizer.h"
#include "misc.h"

#define EQ_BANDS AUD_EQUALIZER_NBANDS
#define MAX_CHANNELS 10

/* Q value for band-pass filters 1.2247 = (3/2)^(1/2)
 * Gives 4 dB suppression at Fc*2 and Fc/2 */
#define Q 1.2247449

/* Center frequencies for band-pass filters (Hz) */
/* These are not the historical WinAmp frequencies, because the IIR filters used
 * here are designed for each frequency to be twice the previous.  Using WinAmp
 * frequencies leads to too much gain in some bands and too little in others. */
static const gfloat CF[EQ_BANDS] = {31.25, 62.5, 125, 250, 500, 1000, 2000,
 4000, 8000, 16000};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static gboolean active;
static gint channels, rate;
static gfloat a[EQ_BANDS][2]; /* A weights */
static gfloat b[EQ_BANDS][2]; /* B weights */
static gfloat wqv[MAX_CHANNELS][EQ_BANDS][2]; /* Circular buffer for W data */
static gfloat gv[MAX_CHANNELS][EQ_BANDS]; /* Gain factor for each channel and band */
static gint K; /* Number of used eq bands */

/* 2nd order band-pass filter design */
static void bp2 (gfloat * a, gfloat * b, gfloat fc, gfloat q)
{
    gfloat th = 2 * M_PI * fc;
    gfloat C = (1 - tanf (th * q / 2)) / (1 + tanf (th * q / 2));

    a[0] = (1 + C) * cosf (th);
    a[1] = -C;
    b[0] = (1 - C) / 2;
    b[1] = -1.005;
}

void eq_set_format (gint new_channels, gint new_rate)
{
    gint k;

    pthread_mutex_lock (& mutex);

    channels = new_channels;
    rate = new_rate;

    /* Calculate number of active filters */
    K = EQ_BANDS;

    while (CF[K - 1] > (gfloat) rate / 2.2)
        K --;

    /* Generate filter taps */
    for (k = 0; k < K; k ++)
        bp2 (a[k], b[k], CF[k] / (gfloat) rate, Q);

    /* Reset state */
    memset (wqv[0][0], 0, sizeof wqv);

    pthread_mutex_unlock (& mutex);
}

static void eq_set_bands_real (gdouble preamp, gdouble * values)
{
    gfloat adj[EQ_BANDS];
    for (gint i = 0; i < EQ_BANDS; i ++)
        adj[i] = preamp + values[i];

    for (gint c = 0; c < MAX_CHANNELS; c ++)
    for (gint i = 0; i < EQ_BANDS; i ++)
        gv[c][i] = pow (10, adj[i] / 20) - 1;
}

void eq_filter (gfloat * data, gint samples)
{
    gint channel;

    pthread_mutex_lock (& mutex);

    if (! active)
    {
        pthread_mutex_unlock (& mutex);
        return;
    }

    for (channel = 0; channel < channels; channel ++)
    {
        gfloat * g = gv[channel]; /* Gain factor */
        gfloat * end = data + samples;
        gfloat * f;

        for (f = data + channel; f < end; f += channels)
        {
            gint k; /* Frequency band index */
            gfloat yt = * f; /* Current input sample */

            for (k = 0; k < K; k ++)
            {
                /* Pointer to circular buffer wq */
                gfloat * wq = wqv[channel][k];
                /* Calculate output from AR part of current filter */
                gfloat w = yt * b[k][0] + wq[0] * a[k][0] + wq[1] * a[k][1];

                /* Calculate output from MA part of current filter */
                yt += (w + wq[1] * b[k][1]) * g[k];

                /* Update circular buffer */
                wq[1] = wq[0];
                wq[0] = w;
            }

            /* Calculate output */
            * f = yt;
        }
    }

    pthread_mutex_unlock (& mutex);
}

static void eq_update (void * data, void * user)
{
    pthread_mutex_lock (& mutex);

    active = get_bool (NULL, "equalizer_active");

    gdouble values[EQ_BANDS];
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

void eq_set_bands (const gdouble * values)
{
    gchar * string = double_array_to_string (values, EQ_BANDS);
    g_return_if_fail (string);
    set_string (NULL, "equalizer_bands", string);
    g_free (string);
}

void eq_get_bands (gdouble * values)
{
    memset (values, 0, sizeof (gdouble) * EQ_BANDS);
    gchar * string = get_string (NULL, "equalizer_bands");
    string_to_double_array (string, values, EQ_BANDS);
    g_free (string);
}

void eq_set_band (gint band, gdouble value)
{
    g_return_if_fail (band >= 0 && band < EQ_BANDS);
    gdouble values[EQ_BANDS];
    eq_get_bands (values);
    values[band] = value;
    eq_set_bands (values);
}

gdouble eq_get_band (gint band)
{
    g_return_val_if_fail (band >= 0 && band < EQ_BANDS, 0);
    gdouble values[EQ_BANDS];
    eq_get_bands (values);
    return values[band];
}
