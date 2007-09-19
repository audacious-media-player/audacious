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
#include "iir.h"
#include "main.h"
#include "input.h"
#include "playback.h"

#include "playlist.h"
#include "configdb.h"

#include "effect.h"
#include "xconvert.h"

#include "volumecontrol.h"

#include <math.h>

typedef struct {
    gint left;
    gint right;
} volumecontrol_req_t;

static volumecontrol_req_t vc_state_ = { 100, 100 };

#define STEREO_ADJUST(type, type2, endian)                                      \
do {                                                                            \
        type *ptr = data;                                                       \
        for (i = 0; i < length; i += 4)                                         \
        {                                                                       \
                *ptr = type2##_TO_##endian(type2##_FROM_## endian(*ptr) *       \
                                           lvol / 256);                         \
                ptr++;                                                          \
                *ptr = type2##_TO_##endian(type2##_FROM_##endian(*ptr) *        \
                                           rvol / 256);                         \
                ptr++;                                                          \
        }                                                                       \
} while (0)

#define MONO_ADJUST(type, type2, endian)                                        \
do {                                                                            \
        type *ptr = data;                                                       \
        for (i = 0; i < length; i += 2)                                         \
        {                                                                       \
                *ptr = type2##_TO_##endian(type2##_FROM_## endian(*ptr) *       \
                                           vol / 256);                          \
                ptr++;                                                          \
        }                                                                       \
} while (0)

#define VOLUME_ADJUST(type, type2, endian)              \
do {                                                    \
        if (channels == 2)                              \
                STEREO_ADJUST(type, type2, endian);     \
        else                                            \
                MONO_ADJUST(type, type2, endian);       \
} while (0)

#define STEREO_ADJUST8(type)                            \
do {                                                    \
        type *ptr = data;                               \
        for (i = 0; i < length; i += 2)                 \
        {                                               \
                *ptr = *ptr * lvol / 256;               \
                ptr++;                                  \
                *ptr = *ptr * rvol / 256;               \
                ptr++;                                  \
        }                                               \
} while (0)

#define MONO_ADJUST8(type)                      \
do {                                            \
        type *ptr = data;                       \
        for (i = 0; i < length; i++)            \
        {                                       \
                *ptr = *ptr * vol / 256;        \
                ptr++;                          \
        }                                       \
} while (0)

#define VOLUME_ADJUST8(type)                    \
do {                                            \
        if (channels == 2)                      \
                STEREO_ADJUST8(type);           \
        else                                    \
                MONO_ADJUST8(type);             \
} while (0)

void volumecontrol_pad_audio(gpointer data, gint length, AFormat fmt,
    gint channels)
{
    gint i, vol, lvol, rvol;

    if (vc_state_.left == 100 && vc_state_.right == 100)
        return;

    if (channels == 1 && (vc_state_.left == 100 || vc_state_.right == 100))
        return;

    lvol = pow(10, (vc_state_.left - 100) / 40.0) * 256;
    rvol = pow(10, (vc_state_.right - 100) / 40.0) * 256;
    vol = MAX(lvol, rvol);

    switch (fmt)
    {
        case FMT_S16_LE:
            VOLUME_ADJUST(gint16, GINT16, LE);
            break;
        case FMT_U16_LE:
            VOLUME_ADJUST(guint16, GUINT16, LE);
            break;
        case FMT_S16_BE:
            VOLUME_ADJUST(gint16, GINT16, LE);
            break;
        case FMT_U16_BE:
            VOLUME_ADJUST(guint16, GUINT16, LE);
            break;
        case FMT_S8:
            VOLUME_ADJUST8(gint8);
            break;
        case FMT_U8:
            VOLUME_ADJUST8(guint8);
            break;
        default:
            g_warning("unhandled format %d.", fmt);
            break;
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

    volumecontrol_pad_audio(context->data, context->len, context->fmt, context->channels);
}
