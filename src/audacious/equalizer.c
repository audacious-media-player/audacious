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
 *  Adapted for Audacious by John Lindgren, 2010
 */

#include <glib.h>
#include <math.h>
#include <string.h>

#include <libaudcore/hook.h>

#include "audconfig.h"
#include "equalizer.h"

#define EQ_BANDS AUD_EQUALIZER_NBANDS
#define MAX_CHANNELS 10

/* Q value for band-pass filters 1.2247 = (3/2)^(1/2)
 * Gives 4 dB suppression at Fc*2 and Fc/2 */
#define Q 1.2247449

/* Center frequencies for band-pass filters (Hz) */
static const gfloat CF[EQ_BANDS] = {60, 170, 310, 600, 1000, 3000, 6000, 12000,
 14000, 16000};

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
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

    g_static_mutex_lock (& mutex);

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

    g_static_mutex_unlock (& mutex);
}

static void eq_set_channel_bands (gint channel, gfloat * bands)
{
    gint k;

    for (k = 0; k < EQ_BANDS; k ++)
        gv[channel][k] = pow (10, bands[k] / 20) - 1;
}

static void eq_set_bands (gfloat preamp, gfloat * bands)
{
    gfloat adjusted[EQ_BANDS];
    gint i;

    for (i = 0; i < EQ_BANDS; i ++)
        adjusted[i] = preamp + bands[i];

    for (i = 0; i < MAX_CHANNELS; i ++)
        eq_set_channel_bands (i, adjusted);
}

void eq_filter (gfloat * data, gint samples)
{
    gint channel;

    g_static_mutex_lock (& mutex);

    if (! active)
    {
        g_static_mutex_unlock (& mutex);
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

    g_static_mutex_unlock (& mutex);
}

static void eq_update (void * data, void * user_data)
{
    g_static_mutex_lock (& mutex);

    active = cfg.equalizer_active;
    eq_set_bands (cfg.equalizer_preamp, cfg.equalizer_bands);

    g_static_mutex_unlock (& mutex);
}

void eq_init (void)
{
    eq_update (NULL, NULL);
    hook_associate ("equalizer changed", eq_update, NULL);
}
