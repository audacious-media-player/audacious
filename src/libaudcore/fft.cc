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

#include "internal.h"

#include <complex>
#include <math.h>

#define TWO_PI 6.2831853f

#define N 512                         /* size of the DFT */
#define LOGN 9                        /* log N (base 2) */

typedef std::complex<float> Complex;

static float hamming[N];              /* hamming window, scaled to sum to 1 */
static int reversed[N];               /* bit-reversal table */
static Complex roots[N / 2];          /* N-th roots of unity */
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

static void generate_tables ()
{
    if (generated)
        return;

    for (int n = 0; n < N; n ++)
        hamming[n] = 1 - 0.85f * cosf (n * (TWO_PI / N));
    for (int n = 0; n < N; n ++)
        reversed[n] = bit_reverse (n);
    for (int n = 0; n < N / 2; n ++)
        roots[n] = exp (Complex (0, n * (TWO_PI / N)));

    generated = 1;
}

/* Perform the DFT using the Cooley-Tukey algorithm.  At each step s, where
 * s=1..log N (base 2), there are N/(2^s) groups of intertwined butterfly
 * operations.  Each group contains (2^s)/2 butterflies, and each butterfly has
 * a span of (2^s)/2.  The twiddle factors are nth roots of unity where n = 2^s. */

static void do_fft (Complex a[N])
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
                Complex even = a[g + b];
                Complex odd = roots[r] * a[g + half + b];
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
    Complex a[N];
    for (int n = 0; n < N; n ++)
        a[reversed[n]] = data[n] * hamming[n];

    do_fft (a);

    /* output values are divided by N */
    /* frequencies from 1 to N/2-1 are doubled */
    for (int n = 0; n < N / 2 - 1; n ++)
        freq[n] = 2 * abs (a[1 + n]) / N;

    /* frequency N/2 is not doubled */
    freq[N / 2 - 1] = abs (a[N / 2]) / N;
}
