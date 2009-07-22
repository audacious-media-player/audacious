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
#include "configdb.h"

#include "effect.h"

#include "volumecontrol.h"

#include <math.h>

#define GAIN -50.0 /* dB */

static void pad_audio (void * data, int length, AFormat fmt, int channels)
{
    int i, k, frames;
    float vol, lvol, rvol;
    float lgain, rgain;

    if (cfg.sw_volume_left == 100 && cfg.sw_volume_right == 100)
        return;

    if (channels != 2 && MAX (cfg.sw_volume_left, cfg.sw_volume_right) == 100)
        return;

    lgain = (float) (100 - cfg.sw_volume_left) * GAIN / 100;
    rgain = (float) (100 - cfg.sw_volume_right) * GAIN / 100;

    lvol = pow(10.0, lgain / 20.0);
    rvol = pow(10.0, rgain / 20.0);
    vol = MAX(lvol, rvol);

    float *ptr = data;
    frames = length / channels / sizeof(float);
    for (k = 0; k < frames; k++) {
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
volumecontrol_flow(FlowContext *context)
{
    if (!cfg.software_volume_control)
        return;

    if (context->fmt != FMT_FLOAT) {
        g_warning("volumecontrol_flow(): unhandled format %d.", context->fmt);
        context->error = TRUE;
        return;
    }

    pad_audio (context->data, context->len, context->fmt, context->channels);
}

void sw_volume_toggled (void)
{
    int vol[2];

    if (cfg.software_volume_control)
    {
        vol[0] = cfg.sw_volume_left;
        vol[1] = cfg.sw_volume_right;
    }
    else
        input_get_volume (& vol[0], & vol[1]);

    hook_call ("volume set", vol);
}
