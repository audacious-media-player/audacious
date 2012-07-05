/*
 * audio.c
 * Copyright 2009-2012 John Lindgren, Michał Lipski, and Anders Johansson
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

#include <glib.h>
#include <stdint.h>
#include <math.h>

#include "audio.h"
#include "config.h"

#define FROM_INT_LOOP(NAME, TYPE, SWAP, OFFSET, RANGE) \
static void NAME (const TYPE * in, float * out, int samples) \
{ \
    const TYPE * end = in + samples; \
    while (in < end) \
        * out ++ = (TYPE) (SWAP (* in ++) - OFFSET) * (1.0 / (RANGE + 1.0)); \
}

#define TO_INT_LOOP(NAME, TYPE, SWAP, OFFSET, RANGE) \
static void NAME (const float * in, TYPE * out, int samples) \
{ \
    const float * end = in + samples; \
    while (in < end) \
    { \
        double f = (* in ++) * (RANGE + 1.0); \
        * out ++ = SWAP (OFFSET + (TYPE) round (CLAMP (f, -RANGE - 1, RANGE))); \
    } \
}

static inline int8_t NOOP8 (int8_t i) {return i;}
static inline int16_t NOOP16 (int16_t i) {return i;}
static inline int32_t NOOP32 (int32_t i) {return i;}

FROM_INT_LOOP (from_s8, int8_t, NOOP8, 0x00, 0x7f)
FROM_INT_LOOP (from_u8, int8_t, NOOP8, 0x80, 0x7f)
FROM_INT_LOOP (from_s16, int16_t, NOOP16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16, int16_t, NOOP16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24, int32_t, NOOP32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24, int32_t, NOOP32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32, int32_t, NOOP32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32, int32_t, NOOP32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s8, int8_t, NOOP8, 0x00, 0x7f)
TO_INT_LOOP (to_u8, int8_t, NOOP8, 0x80, 0x7f)
TO_INT_LOOP (to_s16, int16_t, NOOP16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16, int16_t, NOOP16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24, int32_t, NOOP32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24, int32_t, NOOP32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32, int32_t, NOOP32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32, int32_t, NOOP32, 0x80000000, 0x7fffffff)

static inline int16_t SWAP16 (int16_t i) {return GUINT16_SWAP_LE_BE (i);}
static inline int32_t SWAP32 (int32_t i) {return GUINT32_SWAP_LE_BE (i);}

FROM_INT_LOOP (from_s16_swap, int16_t, SWAP16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16_swap, int16_t, SWAP16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24_swap, int32_t, SWAP32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24_swap, int32_t, SWAP32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32_swap, int32_t, SWAP32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32_swap, int32_t, SWAP32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s16_swap, int16_t, SWAP16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16_swap, int16_t, SWAP16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24_swap, int32_t, SWAP32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24_swap, int32_t, SWAP32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32_swap, int32_t, SWAP32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32_swap, int32_t, SWAP32, 0x80000000, 0x7fffffff)

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

EXPORT void audio_from_int (const void * in, int format, float * out, int samples)
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

EXPORT void audio_to_int (const float * in, void * out, int format, int samples)
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

EXPORT void audio_amplify (float * data, int channels, int frames, float * factors)
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

/* linear approximation of y = sin(x) */
/* contributed by Anders Johansson */
EXPORT void audio_soft_clip (float * data, int samples)
{
    float * end = data + samples;

    while (data < end)
    {
        float x = * data;
        float y = fabsf (x);

        if (y <= 0.4)
            ;                      /* (0, 0.4) -> (0, 0.4) */
        else if (y <= 0.7)
            y = 0.8 * y + 0.08;    /* (0.4, 0.7) -> (0.4, 0.64) */
        else if (y <= 1.0)
            y = 0.7 * y + 0.15;    /* (0.7, 1) -> (0.64, 0.85) */
        else if (y <= 1.3)
            y = 0.4 * y + 0.45;    /* (1, 1.3) -> (0.85, 0.97) */
        else if (y <= 1.5)
            y = 0.15 * y + 0.775;  /* (1.3, 1.5) -> (0.97, 1) */
        else
            y = 1.0;               /* (1.5, inf) -> 1 */

        * data ++ = (x > 0) ? y : -y;
    }
}
