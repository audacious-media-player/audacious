/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
 *
 * volumecontrol.c: High quality volume PCM padding flow.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include "output.h"
#include "main.h"
#include "input.h"
#include "playback.h"

#include "playlist.h"
#include "configdb.h"

#include "effect.h"

#include "volumecontrol.h"

#include <math.h>

typedef struct {
    gint left;
    gint right;
} volumecontrol_req_t;

static volumecontrol_req_t vc_state_ = { 100, 100 };

#define GAIN -50.0 /* dB */

void volumecontrol_pad_audio(gpointer data, gint length, AFormat fmt,
    gint channels)
{
    gint i, k, samples;
    float vol, lvol, rvol;
    float lgain, rgain;

    if (vc_state_.left == 100 && vc_state_.right == 100)
        return;

    if (channels != 2 && (vc_state_.left == 100 || vc_state_.right == 100))
        return;

    lgain = (float)(100 - vc_state_.left) * GAIN / 100.0;
    rgain = (float)(100 - vc_state_.right) * GAIN / 100.0;

    lvol = pow(10.0, lgain / 20.0);
    rvol = pow(10.0, rgain / 20.0);
    vol = MAX(lvol, rvol);

    float *ptr = data;
    samples = length / sizeof(float);
    for (k = 0; k < samples; k++) {
        if (channels == 2) {
            *ptr *= lvol;
            ptr++;
            *ptr *= rvol;
            ptr++;
        } else { /* for mono and other channels number */
            for (i = 0; i < channels; i++) {
                *ptr *= vol; ptr++;
            }
        }
    }
}

void
volumecontrol_get_volume_state(gint *l, gint *r)
{
    *l = vc_state_.left;
    *r = vc_state_.right;
}

void
volumecontrol_set_volume_state(gint l, gint r)
{
    vc_state_.left = l;
    vc_state_.right = r;
}

void
volumecontrol_flow(FlowContext *context)
{
    if (!cfg.software_volume_control)
        return;

    if (context->fmt != FMT_FLOAT) {
        g_warning("volumecontrol_flow(): unhandled format %d.", context->fmt);
        context->error = TRUE;
        return;
    }

    volumecontrol_pad_audio(context->data, context->len, context->fmt, context->channels);
}
