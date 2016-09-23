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

#include <fenv.h>
#include <math.h>
#include <stdint.h>

#define WANT_AUD_BSWAP
#include "audio.h"
#include "objects.h"

#define SW_VOLUME_RANGE 40 /* decibels */

struct packed24_t { uint8_t b[3]; };
static_assert (sizeof (packed24_t) == 3, "invalid packed 24-bit type");

template<class Word>
void interlace_loop (const void * const * in, int channels, void * out, int frames)
{
    for (int c = 0; c < channels; c ++)
    {
        auto get = (const Word *) in[c];
        auto end = get + frames;
        auto set = (Word *) out + c;
        while (get < end)
        {
            * set = * get ++;
            set += channels;
        }
    }
}

template<class Word>
void deinterlace_loop (const void * in, int channels, void * const * out, int frames)
{
    for (int c = 0; c < channels; c ++)
    {
        auto get = (const Word *) in + c;
        auto set = (Word *) out[c];
        auto end = set + frames;
        while (set < end)
        {
            * set ++ = * get;
            get += channels;
        }
    }
}

EXPORT void audio_interlace (const void * const * in, int format, int channels,
 void * out, int frames)
{
    switch (format)
    {
    case FMT_FLOAT:
        interlace_loop<float> (in, channels, out, frames);
        break;

    case FMT_S8: case FMT_U8:
        interlace_loop<int8_t> (in, channels, out, frames);
        break;

    case FMT_S16_LE: case FMT_S16_BE:
    case FMT_U16_LE: case FMT_U16_BE:
        interlace_loop<int16_t> (in, channels, out, frames);
        break;

    case FMT_S24_LE: case FMT_S24_BE:
    case FMT_U24_LE: case FMT_U24_BE:
    case FMT_S32_LE: case FMT_S32_BE:
    case FMT_U32_LE: case FMT_U32_BE:
        interlace_loop<int32_t> (in, channels, out, frames);
        break;

    case FMT_S24_3LE: case FMT_S24_3BE:
    case FMT_U24_3LE: case FMT_U24_3BE:
        interlace_loop<packed24_t> (in, channels, out, frames);
        break;
    }
}

EXPORT void audio_deinterlace (const void * in, int format, int channels,
 void * const * out, int frames)
{
    switch (format)
    {
    case FMT_FLOAT:
        deinterlace_loop<float> (in, channels, out, frames);
        break;

    case FMT_S8: case FMT_U8:
        deinterlace_loop<int8_t> (in, channels, out, frames);
        break;

    case FMT_S16_LE: case FMT_S16_BE:
    case FMT_U16_LE: case FMT_U16_BE:
        deinterlace_loop<int16_t> (in, channels, out, frames);
        break;

    case FMT_S24_LE: case FMT_S24_BE:
    case FMT_U24_LE: case FMT_U24_BE:
    case FMT_S32_LE: case FMT_S32_BE:
    case FMT_U32_LE: case FMT_U32_BE:
        deinterlace_loop<int32_t> (in, channels, out, frames);
        break;

    case FMT_S24_3LE: case FMT_S24_3BE:
    case FMT_U24_3LE: case FMT_U24_3BE:
        deinterlace_loop<packed24_t> (in, channels, out, frames);
        break;
    }
}

static constexpr bool is_le (int format)
{
    return format == FMT_S16_LE || format == FMT_U16_LE ||
           format == FMT_S24_LE || format == FMT_U24_LE ||
           format == FMT_S32_LE || format == FMT_U32_LE ||
           format == FMT_S24_3LE || format == FMT_U24_3LE;
}

static constexpr bool is_signed (int format)
{
    return (format == FMT_S8 ||
            format == FMT_S16_LE || format == FMT_S16_BE ||
            format == FMT_S24_LE || format == FMT_S24_BE ||
            format == FMT_S32_LE || format == FMT_S32_BE ||
            format == FMT_S24_3LE || format == FMT_S24_3BE);
}

static constexpr unsigned neg_range (int format)
{
    return (format >= FMT_S32_LE && format < FMT_S24_3LE) ? 0x80000000 :
           (format >= FMT_S24_LE) ? 0x800000 :
           (format >= FMT_S16_LE) ? 0x8000 : 0x80;
}

// 0x7fffff80 = largest representable floating-point value before 2^31
static constexpr unsigned pos_range (int format)
{
    return (format >= FMT_S32_LE && format < FMT_S24_3LE) ? 0x7fffff80 :
           (format >= FMT_S24_LE) ? 0x7fffff :
           (format >= FMT_S16_LE) ? 0x7fff : 0x7f;
}

template<class T> T do_swap (T value) { return value; }
template<> int16_t do_swap (int16_t value) { return bswap16 (value); }
template<> int32_t do_swap (int32_t value) { return bswap32 (value); }

template<int format, class Word, class Int>
struct Convert
{
#ifdef WORDS_BIGENDIAN
    static constexpr bool native_le = false;
#else
    static constexpr bool native_le = true;
#endif

    static Int to_int (Word value)
    {
        if (is_le (format) ^ native_le)
            value = do_swap (value);
        if (is_signed (format))
            value += neg_range (format);
        if (format >= FMT_S24_LE && format <= FMT_U24_BE)
            value &= 0xffffff; /* ignore high byte */

        return value - neg_range (format);
    }

    static Word to_word (Int value)
    {
        if (! is_signed (format))
            value += neg_range (format);
        if (format >= FMT_S24_LE && format <= FMT_U24_BE)
            value &= 0xffffff; /* zero high byte */
        if (is_le (format) ^ native_le)
            value = do_swap (value);

        return value;
    }
};

template<int format>
struct Convert<format, packed24_t, int32_t>
{
    static int32_t to_int (packed24_t value)
    {
        uint8_t hi, mid, lo;

        if (is_le (format))
            hi = value.b[2], mid = value.b[1], lo = value.b[0];
        else
            hi = value.b[0], mid = value.b[1], lo = value.b[2];

        if (! is_signed (format))
            hi -= 0x80;

        return (int8_t (hi) << 16) | (mid << 8) | lo;
    }

    static packed24_t to_word (int32_t value)
    {
        auto hi = uint8_t (value >> 16),
             mid = uint8_t (value >> 8),
             lo = uint8_t (value);

        if (! is_signed (format))
            hi += 0x80;

        if (is_le (format))
            return {{lo, mid, hi}};
        else
            return {{hi, mid, lo}};
    }
};

template<int format, class Word, class Int = Word>
void from_int_loop (const void * in_, float * out, int samples)
{
    auto in = (const Word *) in_;
    auto end = in + samples;
    while (in < end)
    {
        Int value = Convert<format, Word, Int>::to_int (* in ++);
        * out ++ = value * (1.0f / neg_range (format));
    }
}

template<int format, class Word, class Int = Word>
void to_int_loop (const float * in, void * out_, int samples)
{
    auto end = in + samples;
    auto out = (Word *) out_;
    while (in < end)
    {
        float f = (* in ++) * neg_range (format);
        f = aud::clamp (f, -(float) neg_range (format), (float) pos_range (format));
        * out ++ = Convert<format, Word, Int>::to_word (lrintf (f));
    }
}

EXPORT void audio_from_int (const void * in, int format, float * out, int samples)
{
    switch (format)
    {
        case FMT_S8: from_int_loop<FMT_S8, int8_t> (in, out, samples); break;
        case FMT_U8: from_int_loop<FMT_U8, int8_t> (in, out, samples); break;

        case FMT_S16_LE: from_int_loop<FMT_S16_LE, int16_t> (in, out, samples); break;
        case FMT_S16_BE: from_int_loop<FMT_S16_BE, int16_t> (in, out, samples); break;
        case FMT_U16_LE: from_int_loop<FMT_U16_LE, int16_t> (in, out, samples); break;
        case FMT_U16_BE: from_int_loop<FMT_U16_BE, int16_t> (in, out, samples); break;

        case FMT_S24_LE: from_int_loop<FMT_S24_LE, int32_t> (in, out, samples); break;
        case FMT_S24_BE: from_int_loop<FMT_S24_BE, int32_t> (in, out, samples); break;
        case FMT_U24_LE: from_int_loop<FMT_U24_LE, int32_t> (in, out, samples); break;
        case FMT_U24_BE: from_int_loop<FMT_U24_BE, int32_t> (in, out, samples); break;

        case FMT_S32_LE: from_int_loop<FMT_S32_LE, int32_t> (in, out, samples); break;
        case FMT_S32_BE: from_int_loop<FMT_S32_BE, int32_t> (in, out, samples); break;
        case FMT_U32_LE: from_int_loop<FMT_U32_LE, int32_t> (in, out, samples); break;
        case FMT_U32_BE: from_int_loop<FMT_U32_BE, int32_t> (in, out, samples); break;

        case FMT_S24_3LE: from_int_loop<FMT_S24_3LE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_S24_3BE: from_int_loop<FMT_S24_3BE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_U24_3LE: from_int_loop<FMT_U24_3LE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_U24_3BE: from_int_loop<FMT_U24_3BE, packed24_t, int32_t> (in, out, samples); break;
    }
}

EXPORT void audio_to_int (const float * in, void * out, int format, int samples)
{
    int save = fegetround ();
    fesetround (FE_TONEAREST);

    switch (format)
    {
        case FMT_S8: to_int_loop<FMT_S8, int8_t> (in, out, samples); break;
        case FMT_U8: to_int_loop<FMT_U8, int8_t> (in, out, samples); break;

        case FMT_S16_LE: to_int_loop<FMT_S16_LE, int16_t> (in, out, samples); break;
        case FMT_S16_BE: to_int_loop<FMT_S16_BE, int16_t> (in, out, samples); break;
        case FMT_U16_LE: to_int_loop<FMT_U16_LE, int16_t> (in, out, samples); break;
        case FMT_U16_BE: to_int_loop<FMT_U16_BE, int16_t> (in, out, samples); break;

        case FMT_S24_LE: to_int_loop<FMT_S24_LE, int32_t> (in, out, samples); break;
        case FMT_S24_BE: to_int_loop<FMT_S24_BE, int32_t> (in, out, samples); break;
        case FMT_U24_LE: to_int_loop<FMT_U24_LE, int32_t> (in, out, samples); break;
        case FMT_U24_BE: to_int_loop<FMT_U24_BE, int32_t> (in, out, samples); break;

        case FMT_S32_LE: to_int_loop<FMT_S32_LE, int32_t> (in, out, samples); break;
        case FMT_S32_BE: to_int_loop<FMT_S32_BE, int32_t> (in, out, samples); break;
        case FMT_U32_LE: to_int_loop<FMT_U32_LE, int32_t> (in, out, samples); break;
        case FMT_U32_BE: to_int_loop<FMT_U32_BE, int32_t> (in, out, samples); break;

        case FMT_S24_3LE: to_int_loop<FMT_S24_3LE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_S24_3BE: to_int_loop<FMT_S24_3BE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_U24_3LE: to_int_loop<FMT_U24_3LE, packed24_t, int32_t> (in, out, samples); break;
        case FMT_U24_3BE: to_int_loop<FMT_U24_3BE, packed24_t, int32_t> (in, out, samples); break;
    }

    fesetround (save);
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
