
/* 
 * Mpeg Layer-1,2,3 audio decoder 
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 * slighlty optimized for machines without autoincrement/decrement.
 * The performance is highly compiler dependend. Maybe
 * the decode.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"

int mpg123_synth_1to1_pent(real *, int, unsigned char *);

#if 0
 /* old WRITE_SAMPLE */
#define WRITE_SAMPLE(samples,sum,clip) \
  if( (sum) > 32767.0) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < -32768.0) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = sum; }
#else
 /* new WRITE_SAMPLE */
#define WRITE_SAMPLE(samples,sum,clip) { \
  double dtemp; int v; /* sizeof(int) == 4 */ \
  dtemp = ((((65536.0 * 65536.0 * 16)+(65536.0 * 0.5))* 65536.0)) + (sum);  \
  v = ((*(int *)&dtemp) - 0x80000000); \
  if( v > 32767) { *(samples) = 0x7fff; (clip)++; } \
  else if( v < -32768) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = v; }  \
}
#endif

int
mpg123_synth_1to1_8bit(real * bandPtr, int channel,
                       unsigned char *samples, int *pnt)
{
    short samples_tmp[64];
    short *tmp1 = samples_tmp + channel;
    int i, ret;
    int pnt1 = 0;

    ret =
        mpg123_synth_1to1(bandPtr, channel, (unsigned char *) samples_tmp,
                          &pnt1);
    samples += channel + *pnt;

    for (i = 0; i < 32; i++) {
        *samples = (*tmp1 >> 8) ^ 128;
        samples += 2;
        tmp1 += 2;
    }
    *pnt += 64;

    return ret;
}

int
mpg123_synth_1to1_8bit_mono(real * bandPtr, unsigned char *samples, int *pnt)
{
    short samples_tmp[64];
    short *tmp1 = samples_tmp;
    int i, ret;
    int pnt1 = 0;

    ret = mpg123_synth_1to1(bandPtr, 0, (unsigned char *) samples_tmp, &pnt1);
    samples += *pnt;

    for (i = 0; i < 32; i++) {
        *samples++ = (*tmp1 >> 8) ^ 128;
        tmp1 += 2;
    }
    *pnt += 32;

    return ret;
}

#if 0
int
mpg123_synth_1to1_8bit_mono2stereo(real * bandPtr,
                                   unsigned char *samples, int *pnt)
{
    short samples_tmp[64];
    short *tmp1 = samples_tmp;
    int i, ret;
    int pnt1 = 0;

    ret = mpg123_synth_1to1(bandPtr, 0, (unsigned char *) samples_tmp, &pnt1);
    samples += *pnt;

    for (i = 0; i < 32; i++) {
        *samples++ = (*tmp1 >> 8) ^ 128;
        *samples++ = (*tmp1 >> 8) ^ 128;
        tmp1 += 2;
    }
    *pnt += 64;

    return ret;
}
#endif

int
mpg123_synth_1to1_mono(real * bandPtr, unsigned char *samples, int *pnt)
{
    short samples_tmp[64];
    short *tmp1 = samples_tmp;
    int i, ret;
    int pnt1 = 0;

    ret = mpg123_synth_1to1(bandPtr, 0, (unsigned char *) samples_tmp, &pnt1);
    samples += *pnt;

    for (i = 0; i < 32; i++) {
        *((short *) samples) = *tmp1;
        samples += 2;
        tmp1 += 2;
    }
    *pnt += 64;

    return ret;
}

#if 0
int
mpg123_synth_1to1_mono2stereo(real * bandPtr, unsigned char *samples,
                              int *pnt)
{
    int i, ret;

    ret = mpg123_synth_1to1(bandPtr, 0, samples, pnt);
    samples = samples + *pnt - 128;

    for (i = 0; i < 32; i++) {
        ((short *) samples)[1] = ((short *) samples)[0];
        samples += 4;
    }

    return ret;
}
#endif

int
mpg123_synth_1to1(real * bandPtr, int channel, unsigned char *out, int *pnt)
{
#ifndef I386_ASSEM
    static real buffs[2][2][0x110];
    static const int step = 2;
    static int bo = 1;
    short *samples = (short *) (out + *pnt);

    real *b0, (*buf)[0x110];
    int clip = 0;
    int bo1;

    if (!channel) {
        bo--;
        bo &= 0xf;
        buf = buffs[0];
    }
    else {
        samples++;
        buf = buffs[1];
    }

    if (bo & 0x1) {
        b0 = buf[0];
        bo1 = bo;
        mpg123_dct64(buf[1] + ((bo + 1) & 0xf), buf[0] + bo, bandPtr);
    }
    else {
        b0 = buf[1];
        bo1 = bo + 1;
        mpg123_dct64(buf[0] + bo, buf[1] + bo + 1, bandPtr);
    }

    {
        register int j;
        real *window = mpg123_decwin + 16 - bo1;

        for (j = 16; j; j--, b0 += 0x10, window += 0x20, samples += step) {
            real sum;

            sum = window[0x0] * b0[0x0];
            sum -= window[0x1] * b0[0x1];
            sum += window[0x2] * b0[0x2];
            sum -= window[0x3] * b0[0x3];
            sum += window[0x4] * b0[0x4];
            sum -= window[0x5] * b0[0x5];
            sum += window[0x6] * b0[0x6];
            sum -= window[0x7] * b0[0x7];
            sum += window[0x8] * b0[0x8];
            sum -= window[0x9] * b0[0x9];
            sum += window[0xA] * b0[0xA];
            sum -= window[0xB] * b0[0xB];
            sum += window[0xC] * b0[0xC];
            sum -= window[0xD] * b0[0xD];
            sum += window[0xE] * b0[0xE];
            sum -= window[0xF] * b0[0xF];

            WRITE_SAMPLE(samples, sum, clip);
        }

        {
            real sum;

            sum = window[0x0] * b0[0x0];
            sum += window[0x2] * b0[0x2];
            sum += window[0x4] * b0[0x4];
            sum += window[0x6] * b0[0x6];
            sum += window[0x8] * b0[0x8];
            sum += window[0xA] * b0[0xA];
            sum += window[0xC] * b0[0xC];
            sum += window[0xE] * b0[0xE];
            WRITE_SAMPLE(samples, sum, clip);
            b0 -= 0x10, window -= 0x20, samples += step;
        }
        window += bo1 << 1;

        for (j = 15; j; j--, b0 -= 0x10, window -= 0x20, samples += step) {
            real sum;

            sum = -window[-0x1] * b0[0x0];
            sum -= window[-0x2] * b0[0x1];
            sum -= window[-0x3] * b0[0x2];
            sum -= window[-0x4] * b0[0x3];
            sum -= window[-0x5] * b0[0x4];
            sum -= window[-0x6] * b0[0x5];
            sum -= window[-0x7] * b0[0x6];
            sum -= window[-0x8] * b0[0x7];
            sum -= window[-0x9] * b0[0x8];
            sum -= window[-0xA] * b0[0x9];
            sum -= window[-0xB] * b0[0xA];
            sum -= window[-0xC] * b0[0xB];
            sum -= window[-0xD] * b0[0xC];
            sum -= window[-0xE] * b0[0xD];
            sum -= window[-0xF] * b0[0xE];
            sum -= window[-0x0] * b0[0xF];

            WRITE_SAMPLE(samples, sum, clip);
        }
    }
    *pnt += 128;

    return clip;
#else
    {
        int ret;

        ret = mpg123_synth_1to1_pent(bandPtr, channel, out + *pnt);
        *pnt += 128;
        return ret;
    }
#endif
}

#ifdef USE_SIMD
int mpg123_synth_MMX(real *, int, short *, short *, int *);

int
mpg123_synth_1to1_mmx(real * bandPtr, int channel, unsigned char *out,
                      int *pnt)
{
    static short buffs[2][2][0x110];
    static int bo = 1;
    short *samples = (short *) (out + *pnt);

    mpg123_synth_MMX(bandPtr, channel, samples, (short *) buffs, &bo);
    *pnt += 128;
    return 0;
}
#endif
