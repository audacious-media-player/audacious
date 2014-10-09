/*
 * audio.c
 * Copyright 2009-2013 John Lindgren, Michał Lipski, and Anders Johansson
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

#include <math.h>
#include <stdint.h>

#define WANT_AUD_BSWAP
#include "audio.h"
#include "objects.h"

#define SW_VOLUME_RANGE 40 /* decibels */

#define INTERLACE_LOOP(TYPE) \
for (int c = 0; c < channels; c ++) \
{ \
    const TYPE * get = (const TYPE *) in[c]; \
    const TYPE * end = get + frames; \
    TYPE * set = (TYPE *) out + c; \
    while (get < end) \
    { \
        * set = * get ++; \
        set += channels; \
    } \
}

EXPORT void audio_interlace (const void * const * in, int format, int channels,
 void * out, int frames)
{
    switch (format)
    {
    case FMT_FLOAT:
        INTERLACE_LOOP (float);
        break;

    case FMT_S8:
    case FMT_U8:
        INTERLACE_LOOP (int8_t);
        break;

    case FMT_S16_LE:
    case FMT_S16_BE:
    case FMT_U16_LE:
    case FMT_U16_BE:
        INTERLACE_LOOP (int16_t);
        break;

    case FMT_S24_LE:
    case FMT_S24_BE:
    case FMT_U24_LE:
    case FMT_U24_BE:
    case FMT_S32_LE:
    case FMT_S32_BE:
    case FMT_U32_LE:
    case FMT_U32_BE:
        INTERLACE_LOOP (int32_t);
        break;
    }
}

#define DEINTERLACE_LOOP(TYPE) \
for (int c = 0; c < channels; c ++) \
{ \
    const TYPE * get = (const TYPE *) in + c; \
    TYPE * set = (TYPE *) out[c]; \
    TYPE * end = set + frames; \
    while (set < end) \
    { \
        * set ++ = * get; \
        get += channels; \
    } \
}

EXPORT void audio_deinterlace (const void * in, int format, int channels,
 void * const * out, int frames)
{
    switch (format)
    {
    case FMT_FLOAT:
        DEINTERLACE_LOOP (float);
        break;

    case FMT_S8:
    case FMT_U8:
        DEINTERLACE_LOOP (int8_t);
        break;

    case FMT_S16_LE:
    case FMT_S16_BE:
    case FMT_U16_LE:
    case FMT_U16_BE:
        DEINTERLACE_LOOP (int16_t);
        break;

    case FMT_S24_LE:
    case FMT_S24_BE:
    case FMT_U24_LE:
    case FMT_U24_BE:
    case FMT_S32_LE:
    case FMT_S32_BE:
    case FMT_U32_LE:
    case FMT_U32_BE:
        DEINTERLACE_LOOP (int32_t);
        break;
    }
}

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
        * out ++ = SWAP (OFFSET + (TYPE) round (aud::clamp<double> (f, -RANGE - 1, RANGE))); \
    } \
}

FROM_INT_LOOP (from_s8, int8_t, , 0x00, 0x7f)
FROM_INT_LOOP (from_u8, int8_t, , 0x80, 0x7f)

TO_INT_LOOP (to_s8, int8_t, , 0x00, 0x7f)
TO_INT_LOOP (to_u8, int8_t, , 0x80, 0x7f)

FROM_INT_LOOP (from_s16le, int16_t, FROM_LE16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16le, int16_t, FROM_LE16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24le, int32_t, FROM_LE32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24le, int32_t, FROM_LE32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32le, int32_t, FROM_LE32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32le, int32_t, FROM_LE32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s16le, int16_t, TO_LE16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16le, int16_t, TO_LE16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24le, int32_t, TO_LE32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24le, int32_t, TO_LE32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32le, int32_t, TO_LE32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32le, int32_t, TO_LE32, 0x80000000, 0x7fffffff)

FROM_INT_LOOP (from_s16be, int16_t, FROM_BE16, 0x0000, 0x7fff)
FROM_INT_LOOP (from_u16be, int16_t, FROM_BE16, 0x8000, 0x7fff)
FROM_INT_LOOP (from_s24be, int32_t, FROM_BE32, 0x000000, 0x7fffff)
FROM_INT_LOOP (from_u24be, int32_t, FROM_BE32, 0x800000, 0x7fffff)
FROM_INT_LOOP (from_s32be, int32_t, FROM_BE32, 0x00000000, 0x7fffffff)
FROM_INT_LOOP (from_u32be, int32_t, FROM_BE32, 0x80000000, 0x7fffffff)

TO_INT_LOOP (to_s16be, int16_t, TO_BE16, 0x0000, 0x7fff)
TO_INT_LOOP (to_u16be, int16_t, TO_BE16, 0x8000, 0x7fff)
TO_INT_LOOP (to_s24be, int32_t, TO_BE32, 0x000000, 0x7fffff)
TO_INT_LOOP (to_u24be, int32_t, TO_BE32, 0x800000, 0x7fffff)
TO_INT_LOOP (to_s32be, int32_t, TO_BE32, 0x00000000, 0x7fffffff)
TO_INT_LOOP (to_u32be, int32_t, TO_BE32, 0x80000000, 0x7fffffff)

typedef void (* FromFunc) (const void * in, float * out, int samples);
typedef void (* ToFunc) (const float * in, void * out, int samples);

static const struct
{
    int format;
    FromFunc from;
    ToFunc to;
}
convert_table [] =
{
    {FMT_S8, (FromFunc) from_s8, (ToFunc) to_s8},
    {FMT_U8, (FromFunc) from_u8, (ToFunc) to_u8},

    {FMT_S16_LE, (FromFunc) from_s16le, (ToFunc) to_s16le},
    {FMT_U16_LE, (FromFunc) from_u16le, (ToFunc) to_u16le},
    {FMT_S24_LE, (FromFunc) from_s24le, (ToFunc) to_s24le},
    {FMT_U24_LE, (FromFunc) from_u24le, (ToFunc) to_u24le},
    {FMT_S32_LE, (FromFunc) from_s32le, (ToFunc) to_s32le},
    {FMT_U32_LE, (FromFunc) from_u32le, (ToFunc) to_u32le},

    {FMT_S16_BE, (FromFunc) from_s16be, (ToFunc) to_s16be},
    {FMT_U16_BE, (FromFunc) from_u16be, (ToFunc) to_u16be},
    {FMT_S24_BE, (FromFunc) from_s24be, (ToFunc) to_s24be},
    {FMT_U24_BE, (FromFunc) from_u24be, (ToFunc) to_u24be},
    {FMT_S32_BE, (FromFunc) from_s32be, (ToFunc) to_s32be},
    {FMT_U32_BE, (FromFunc) from_u32be, (ToFunc) to_u32be},
};

EXPORT void audio_from_int (const void * in, int format, float * out, int samples)
{
    for (auto & conv : convert_table)
    {
        if (conv.format == format)
        {
            conv.from (in, out, samples);
            return;
        }
    }
}

EXPORT void audio_to_int (const float * in, void * out, int format, int samples)
{
    for (auto & conv : convert_table)
    {
        if (conv.format == format)
        {
            conv.to (in, out, samples);
            return;
        }
    }
}

EXPORT void audio_amplify (float * data, int channels, int frames, const float * factors)
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

EXPORT void audio_amplify (float * data, int channels, int frames, StereoVolume volume)
{
    if (channels < 1 || channels > AUD_MAX_CHANNELS)
        return;

    if (volume.left == 100 && volume.right == 100)
        return;

    float lfactor = 0, rfactor = 0;
    float factors[AUD_MAX_CHANNELS];

    if (volume.left > 0)
        lfactor = powf (10, (float) SW_VOLUME_RANGE * (volume.left - 100) / 100 / 20);
    if (volume.right > 0)
        rfactor = powf (10, (float) SW_VOLUME_RANGE * (volume.right - 100) / 100 / 20);

    if (channels == 2)
    {
        factors[0] = lfactor;
        factors[1] = rfactor;
    }
    else
    {
        for (int c = 0; c < channels; c ++)
            factors[c] = aud::max (lfactor, rfactor);
    }

    audio_amplify (data, channels, frames, factors);
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
