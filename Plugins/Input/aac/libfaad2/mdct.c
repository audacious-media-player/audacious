/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: mdct.c,v 1.34 2003/11/12 20:47:58 menno Exp $
**/

/*
 * Fast (I)MDCT Implementation using (I)FFT ((Inverse) Fast Fourier Transform)
 * and consists of three steps: pre-(I)FFT complex multiplication, complex
 * (I)FFT, post-(I)FFT complex multiplication,
 * 
 * As described in:
 *  P. Duhamel, Y. Mahieux, and J.P. Petit, "A Fast Algorithm for the
 *  Implementation of Filter Banks Based on 'Time Domain Aliasing
 *  Cancellation’," IEEE Proc. on ICASSP‘91, 1991, pp. 2209-2212.
 *
 *
 * As of April 6th 2002 completely rewritten.
 * This (I)MDCT can now be used for any data size n, where n is divisible by 8.
 *
 */

#include "common.h"
#include "structs.h"

#include <stdlib.h>
#ifdef _WIN32_WCE
#define assert(x)
#else
#include <assert.h>
#endif

#include "cfft.h"
#include "mdct.h"

/* const_tab[]:
    0: sqrt(2 / N)
    1: cos(2 * PI / N)
    2: sin(2 * PI / N)
    3: cos(2 * PI * (1/8) / N)
    4: sin(2 * PI * (1/8) / N)
 */
#ifdef FIXED_POINT
real_t const_tab[][5] =
{
    {    /* 2048 */
        COEF_CONST(1),
        FRAC_CONST(0.99999529380957619),
        FRAC_CONST(0.0030679567629659761),
        FRAC_CONST(0.99999992646571789),
        FRAC_CONST(0.00038349518757139556)
    }, { /* 1920 */
        COEF_CONST(/* sqrt(1024/960) */ 1.0327955589886444),
        FRAC_CONST(0.99999464540169647),
        FRAC_CONST(0.0032724865065266251),
        FRAC_CONST(0.99999991633432805),
        FRAC_CONST(0.00040906153202803459)
    }, { /* 1024 */
        COEF_CONST(1),
        FRAC_CONST(0.99998117528260111),
        FRAC_CONST(0.0061358846491544753),
        FRAC_CONST(0.99999970586288223),
        FRAC_CONST(0.00076699031874270449)
    }, { /* 960 */
        COEF_CONST(/* sqrt(512/480) */ 1.0327955589886444),
        FRAC_CONST(0.99997858166412923),
        FRAC_CONST(0.0065449379673518581),
        FRAC_CONST(0.99999966533732598),
        FRAC_CONST(0.00081812299560725323)
    }, { /* 256 */
        COEF_CONST(1),
        FRAC_CONST(0.99969881869620425),
        FRAC_CONST(0.024541228522912288),
        FRAC_CONST(0.99999529380957619),
        FRAC_CONST(0.0030679567629659761)
    }, {  /* 240 */
        COEF_CONST(/* sqrt(256/240) */ 1.0327955589886444),
        FRAC_CONST(0.99965732497555726),
        FRAC_CONST(0.026176948307873149),
        FRAC_CONST(0.99999464540169647),
        FRAC_CONST(0.0032724865065266251)
    }
#ifdef SSR_DEC
    ,{   /* 512 */
        COEF_CONST(1),
        FRAC_CONST(0.9999247018391445),
        FRAC_CONST(0.012271538285719925),
        FRAC_CONST(0.99999882345170188),
        FRAC_CONST(0.0015339801862847655)
    }, { /* 64 */
        COEF_CONST(1),
        FRAC_CONST(0.99518472667219693),
        FRAC_CONST(0.098017140329560604),
        FRAC_CONST(0.9999247018391445),
        FRAC_CONST(0.012271538285719925)
    }
#endif
};
#endif

uint8_t map_N_to_idx(uint16_t N)
{
    /* gives an index into const_tab above */
    /* for normal AAC deocding (eg. no scalable profile) only */
    /* index 0 and 4 will be used */
    switch(N)
    {
    case 2048: return 0;
    case 1920: return 1;
    case 1024: return 2;
    case 960:  return 3;
    case 256:  return 4;
    case 240:  return 5;
#ifdef SSR_DEC
    case 512:  return 6;
    case 64:   return 7;
#endif
    }
    return 0;
}

mdct_info *faad_mdct_init(uint16_t N)
{
    uint16_t k;
#ifdef FIXED_POINT
    uint16_t N_idx;
    real_t cangle, sangle, c, s, cold;
#endif
	real_t scale;

    mdct_info *mdct = (mdct_info*)malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (complex_t*)malloc(N/4*sizeof(complex_t));

#ifdef FIXED_POINT
    N_idx = map_N_to_idx(N);

    scale = const_tab[N_idx][0];
    cangle = const_tab[N_idx][1];
    sangle = const_tab[N_idx][2];
    c = const_tab[N_idx][3];
    s = const_tab[N_idx][4];
#else
    scale = (real_t)sqrt(2.0 / (real_t)N);
#endif

    /* (co)sine table build using recurrence relations */
    /* this can also be done using static table lookup or */
    /* some form of interpolation */
    for (k = 0; k < N/4; k++)
    {
#ifdef FIXED_POINT
        RE(mdct->sincos[k]) = c; //MUL_C_C(c,scale);
        IM(mdct->sincos[k]) = s; //MUL_C_C(s,scale);

        cold = c;
        c = MUL_F(c,cangle) - MUL_F(s,sangle);
        s = MUL_F(s,cangle) + MUL_F(cold,sangle);
#else
        /* no recurrence, just sines */
        RE(mdct->sincos[k]) = scale*(real_t)(cos(2.0*M_PI*(k+1./8.) / (real_t)N));
        IM(mdct->sincos[k]) = scale*(real_t)(sin(2.0*M_PI*(k+1./8.) / (real_t)N));
#endif
    }

    /* initialise fft */
    mdct->cfft = cffti(N/4);

    return mdct;
}

void faad_mdct_end(mdct_info *mdct)
{
    if (mdct != NULL)
    {
        cfftu(mdct->cfft);

        if (mdct->sincos) free(mdct->sincos);

        free(mdct);
    }
}

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    complex_t x;
    complex_t Z1[512];
    complex_t *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

    /* pre-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        ComplexMult(&IM(Z1[k]), &RE(Z1[k]),
            X_in[2*k], X_in[N2 - 1 - 2*k], RE(sincos[k]), IM(sincos[k]));
    }

    /* complex IFFT, any non-scaling FFT can be used here */
    cfftb(mdct->cfft, Z1);

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        RE(x) = RE(Z1[k]);
        IM(x) = IM(Z1[k]);
        ComplexMult(&IM(Z1[k]), &RE(Z1[k]),
            IM(x), RE(x), RE(sincos[k]), IM(sincos[k]));

#ifdef FIXED_POINT
#if (REAL_BITS == 16)
        if (abs(RE(Z1[k])) > REAL_CONST(16383.5))
        {
            if (RE(Z1[k]) > 0) RE(Z1[k]) = REAL_CONST(32767.0);
            else RE(Z1[k]) = REAL_CONST(-32767.0);
        } else {
            RE(Z1[k]) *= 2;
        }
        if (abs(IM(Z1[k])) > REAL_CONST(16383.5))
        {
            if (IM(Z1[k]) > 0) IM(Z1[k]) = REAL_CONST(32767.0);
            else IM(Z1[k]) = REAL_CONST(-32767.0);
        } else {
            IM(Z1[k]) *= 2;
        }
#endif
#endif
    }

    /* reordering */
    for (k = 0; k < N8; k++)
    {
        X_out[              2*k] =  IM(Z1[N8 +     k]);
        X_out[          1 + 2*k] = -RE(Z1[N8 - 1 - k]);
        X_out[N4 +          2*k] =  RE(Z1[         k]);
        X_out[N4 +      1 + 2*k] = -IM(Z1[N4 - 1 - k]);
        X_out[N2 +          2*k] =  RE(Z1[N8 +     k]);
        X_out[N2 +      1 + 2*k] = -IM(Z1[N8 - 1 - k]);
        X_out[N2 + N4 +     2*k] = -IM(Z1[         k]);
        X_out[N2 + N4 + 1 + 2*k] =  RE(Z1[N4 - 1 - k]);
    }
}

#ifdef LTP_DEC
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    uint16_t k;

    complex_t x;
    complex_t Z1[512];
    complex_t *sincos = mdct->sincos;

    uint16_t N  = mdct->N;
    uint16_t N2 = N >> 1;
    uint16_t N4 = N >> 2;
    uint16_t N8 = N >> 3;

#ifndef FIXED_POINT
	real_t scale = REAL_CONST(N);
#else
	real_t scale = REAL_CONST(4.0/N);
#endif

    /* pre-FFT complex multiplication */
    for (k = 0; k < N8; k++)
    {
        uint16_t n = k << 1;
        RE(x) = X_in[N - N4 - 1 - n] + X_in[N - N4 +     n];
        IM(x) = X_in[    N4 +     n] - X_in[    N4 - 1 - n];

        ComplexMult(&RE(Z1[k]), &IM(Z1[k]),
            RE(x), IM(x), RE(sincos[k]), IM(sincos[k]));

        RE(Z1[k]) = MUL_R(RE(Z1[k]), scale);
        IM(Z1[k]) = MUL_R(IM(Z1[k]), scale);

        RE(x) =  X_in[N2 - 1 - n] - X_in[        n];
        IM(x) =  X_in[N2 +     n] + X_in[N - 1 - n];

        ComplexMult(&RE(Z1[k + N8]), &IM(Z1[k + N8]),
            RE(x), IM(x), RE(sincos[k + N8]), IM(sincos[k + N8]));

        RE(Z1[k + N8]) = MUL_R(RE(Z1[k + N8]), scale);
        IM(Z1[k + N8]) = MUL_R(IM(Z1[k + N8]), scale);
    }

    /* complex FFT, any non-scaling FFT can be used here  */
    cfftf(mdct->cfft, Z1);

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        uint16_t n = k << 1;
        ComplexMult(&RE(x), &IM(x),
            RE(Z1[k]), IM(Z1[k]), RE(sincos[k]), IM(sincos[k]));

        X_out[         n] = -RE(x);
        X_out[N2 - 1 - n] =  IM(x);
        X_out[N2 +     n] = -IM(x);
        X_out[N  - 1 - n] =  RE(x);
    }
}
#endif
