/*
 * audio.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <glib.h>
#include <stdint.h>

#include "audio.h"

#define FROM_INT_LOOP(NAME, TYPE, SWAP, OFFSET, RANGE) \
static void NAME (const TYPE * in, float * out, int samples) \
{ \
    const TYPE * end = in + samples; \
    while (in < end) \
        * out ++ = (TYPE) (SWAP (* in ++) - OFFSET) / (double) RANGE; \
}

#define TO_INT_LOOP(NAME, TYPE, SWAP, OFFSET, RANGE) \
static void NAME (const float * in, TYPE * out, int samples) \
{ \
    const float * end = in + samples; \
    while (in < end) \
    { \
        double f = * in ++; \
        * out ++ = SWAP (OFFSET + (TYPE) (CLAMP (f, -1, 1) * (double) RANGE)); \
    } \
}

static inline int8_t noop8 (int8_t i) {return i;}
static inline int16_t noop16 (int16_t i) {return i;}
static inline int32_t noop32 (int32_t i) {return i;}

FROM_INT_LOOP (from_s8, int8_t, noop8, 0x00, 0x7f)
FROM_INT_LOOP (from_u8, int8_t, noop8, 0x80, 0x7f)
FROM_INT_LOOP (from_s16, int16_t, noop16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16, int16_t, noop16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24, int32_t, noop32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24, int32_t, noop32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32, int32_t, noop32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32, int32_t, noop32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s8, int8_t, noop8, 0x00, 0x7f)
TO_INT_LOOP (to_u8, int8_t, noop8, 0x80, 0x7f)
TO_INT_LOOP (to_s16, int16_t, noop16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16, int16_t, noop16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24, int32_t, noop32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24, int32_t, noop32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32, int32_t, noop32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32, int32_t, noop32, 0x80000000, 0x7fffffff)

static inline int16_t swap16 (int16_t i) {return GUINT16_SWAP_LE_BE (i);}
static inline int32_t swap32 (int32_t i) {return GUINT32_SWAP_LE_BE (i);}

FROM_INT_LOOP (from_s16_swap, int16_t, swap16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16_swap, int16_t, swap16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24_swap, int32_t, swap32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24_swap, int32_t, swap32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32_swap, int32_t, swap32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32_swap, int32_t, swap32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s16_swap, int16_t, swap16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16_swap, int16_t, swap16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24_swap, int32_t, swap32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24_swap, int32_t, swap32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32_swap, int32_t, swap32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32_swap, int32_t, swap32, 0x80000000, 0x7fffffff)

typedef void (* FromFunc) (const void * in, float * out, int samples);
typedef void (* ToFunc) (const float * in, void * out, int samples);

struct
{
    int format;
    FromFunc from;
    ToFunc to;
}
convert_table [] =
{
    {FMT_S8, (FromFunc) from_s8, (ToFunc) to_s8},
    {FMT_U8, (FromFunc) from_u8, (ToFunc) to_u8},

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    {FMT_S16_LE, (FromFunc) from_s16, (ToFunc) to_s16},
    {FMT_U16_LE, (FromFunc) from_u16, (ToFunc) to_u16},
    {FMT_S24_LE, (FromFunc) from_s24, (ToFunc) to_s24},
    {FMT_U24_LE, (FromFunc) from_u24, (ToFunc) to_u24},
    {FMT_S32_LE, (FromFunc) from_s32, (ToFunc) to_s32},
    {FMT_U32_LE, (FromFunc) from_u32, (ToFunc) to_u32},

    {FMT_S16_BE, (FromFunc) from_s16_swap, (ToFunc) to_s16_swap},
    {FMT_U16_BE, (FromFunc) from_u16_swap, (ToFunc) to_u16_swap},
    {FMT_S24_BE, (FromFunc) from_s24_swap, (ToFunc) to_s24_swap},
    {FMT_U24_BE, (FromFunc) from_u24_swap, (ToFunc) to_u24_swap},
    {FMT_S32_BE, (FromFunc) from_s32_swap, (ToFunc) to_s32_swap},
    {FMT_U32_BE, (FromFunc) from_u32_swap, (ToFunc) to_u32_swap},
#else
    {FMT_S16_BE, (FromFunc) from_s16, (ToFunc) to_s16},
    {FMT_U16_BE, (FromFunc) from_u16, (ToFunc) to_u16},
    {FMT_S24_BE, (FromFunc) from_s24, (ToFunc) to_s24},
    {FMT_U24_BE, (FromFunc) from_u24, (ToFunc) to_u24},
    {FMT_S32_BE, (FromFunc) from_s32, (ToFunc) to_s32},
    {FMT_U32_BE, (FromFunc) from_u32, (ToFunc) to_u32},

    {FMT_S16_LE, (FromFunc) from_s16_swap, (ToFunc) to_s16_swap},
    {FMT_U16_LE, (FromFunc) from_u16_swap, (ToFunc) to_u16_swap},
    {FMT_S24_LE, (FromFunc) from_s24_swap, (ToFunc) to_s24_swap},
    {FMT_U24_LE, (FromFunc) from_u24_swap, (ToFunc) to_u24_swap},
    {FMT_S32_LE, (FromFunc) from_s32_swap, (ToFunc) to_s32_swap},
    {FMT_U32_LE, (FromFunc) from_u32_swap, (ToFunc) to_u32_swap},
#endif
};

void audio_from_int (const void * in, int format, float * out, int samples)
{
    int entry;

    for (entry = 0; entry < G_N_ELEMENTS (convert_table); entry ++)
    {
        if (convert_table[entry].format == format)
        {
            convert_table[entry].from (in, out, samples);
            return;
        }
    }
}

void audio_to_int (const float * in, void * out, int format, int samples)
{
    int entry;

    for (entry = 0; entry < G_N_ELEMENTS (convert_table); entry ++)
    {
        if (convert_table[entry].format == format)
        {
            convert_table[entry].to (in, out, samples);
            return;
        }
    }
}

void audio_amplify (float * data, int channels, int frames, float * factors)
{
    float * end = data + channels * frames;
    int channel;

    while (data < end)
    {
        for (channel = 0; channel < channels; channel ++)
        {
            * data = * data * factors[channel];
            data ++;
        }
    }
}
