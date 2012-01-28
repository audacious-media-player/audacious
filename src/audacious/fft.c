/*
 * fft.c
 * Copyright 2011 John Lindgren
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

#include <complex.h>
#include <math.h>

#include "config.h"
#include "fft.h"

#ifndef HAVE_CEXPF
/* e^(a+bi) = (e^a)(cos(b)+sin(b)i) */
#define cexpf(x) (expf(crealf(x))*(cosf(cimagf(x))+sinf(cimagf(x))*I))
#warning Your C library does not have cexpf(). Please update it.
#endif

#define N 512                         /* size of the DFT */
#define LOGN 9                        /* log N (base 2) */

static float hamming[N];              /* hamming window, scaled to sum to 1 */
static int reversed[N];               /* bit-reversal table */
static float complex roots[N / 2];    /* N-th roots of unity */
static char generated = 0;            /* set if tables have been generated */

/* Reverse the order of the lowest LOGN bits in an integer. */

static int bit_reverse (int x)
{
    int y = 0;

    for (int n = LOGN; n --; )
    {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }

    return y;
}

/* Generate lookup tables. */

static void generate_tables (void)
{
    if (generated)
        return;

    for (int n = 0; n < N; n ++)
        hamming[n] = 1 - 0.85 * cosf (2 * M_PI * n / N);
    for (int n = 0; n < N; n ++)
        reversed[n] = bit_reverse (n);
    for (int n = 0; n < N / 2; n ++)
        roots[n] = cexpf (2 * M_PI * I * n / N);

    generated = 1;
}

/* Perform the DFT using the Cooley-Tukey algorithm.  At each step s, where
 * s=1..log N (base 2), there are N/(2^s) groups of intertwined butterfly
 * operations.  Each group contains (2^s)/2 butterflies, and each butterfly has
 * a span of (2^s)/2.  The twiddle factors are nth roots of unity where n = 2^s. */

static void do_fft (float complex a[N])
{
    int half = 1;       /* (2^s)/2 */
    int inv = N / 2;    /* N/(2^s) */

    /* loop through steps */
    while (inv)
    {
        /* loop through groups */
        for (int g = 0; g < N; g += half << 1)
        {
            /* loop through butterflies */
            for (int b = 0, r = 0; b < half; b ++, r += inv)
            {
                float complex even = a[g + b];
                float complex odd = roots[r] * a[g + half + b];
                a[g + b] = even + odd;
                a[g + half + b] = even - odd;
            }
        }

        half <<= 1;
        inv >>= 1;
    }
}

/* Input is N=512 PCM samples.
 * Output is intensity of frequencies from 1 to N/2=256. */

void calc_freq (const float data[N], float freq[N / 2])
{
    generate_tables ();

    /* input is filtered by a Hamming window */
    /* input values are in bit-reversed order */
    float complex a[N];
    for (int n = 0; n < N; n ++)
        a[reversed[n]] = data[n] * hamming[n];

    do_fft (a);

    /* output values are divided by N */
    /* frequencies from 1 to N/2-1 are doubled */
    for (int n = 0; n < N / 2 - 1; n ++)
        freq[n] = 2 * cabsf (a[1 + n]) / N;

    /* frequency N/2 is not doubled */
    freq[N / 2 - 1] = cabsf (a[N / 2]) / N;
}
