/* 
 * Mpeg Layer-1,2,3 audio decoder 
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"

#define WRITE_SAMPLE(samples,sum,clip) \
  if( (sum) > REAL_PLUS_32767) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < REAL_MINUS_32768) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = sum; }

int mpgdec_synth_1to1_8bit(mpgdec_real *bandPtr,int channel,unsigned char *samples,int *pnt)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp + channel;
  int i,ret;
  int pnt1=0;

  ret = mpgdec_synth_1to1(bandPtr,channel,(unsigned char *) samples_tmp,&pnt1);
  samples += channel + *pnt;

  for(i=0;i<32;i++) {
    *samples = mpgdec_conv16to8[*tmp1>>AUSHIFT];
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 64;

  return ret;
}

int mpgdec_synth_1to1_8bit_mono(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpgdec_synth_1to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<32;i++) {
    *samples++ = mpgdec_conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  *pnt += 32;
  
  return ret;
}

int mpgdec_synth_1to1_8bit_mono2stereo(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpgdec_synth_1to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<32;i++) {
    *samples++ = mpgdec_conv16to8[*tmp1>>AUSHIFT];
    *samples++ = mpgdec_conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  *pnt += 64;

  return ret;
}

int mpgdec_synth_1to1_mono(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpgdec_synth_1to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<32;i++) {
    *( (short *)samples) = *tmp1;
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 64;

  return ret;
}


int mpgdec_synth_1to1_mono2stereo(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  int i,ret;

  ret = mpgdec_synth_1to1(bandPtr,0,samples,pnt);
  samples = samples + *pnt - 128;

  for(i=0;i<32;i++) {
    ((short *)samples)[1] = ((short *)samples)[0];
    samples+=4;
  }

  return ret;
}


int mpgdec_synth_1to1(mpgdec_real *bandPtr,int channel,unsigned char *out,int *pnt)
{
  static mpgdec_real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
  short *samples = (short *) (out+*pnt);

  mpgdec_real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    mpgdec_dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    mpgdec_dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }


  {
    register int j;
    mpgdec_real *window = mpgdec_decwin + 16 - bo1;
 
    for (j=16;j;j--,window+=0x10,samples+=step)
    {
      mpgdec_real sum;
      sum  = REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);

      WRITE_SAMPLE(samples,sum,clip);
    }

    {
      mpgdec_real sum;
      sum  = REAL_MUL(window[0x0], b0[0x0]);
      sum += REAL_MUL(window[0x2], b0[0x2]);
      sum += REAL_MUL(window[0x4], b0[0x4]);
      sum += REAL_MUL(window[0x6], b0[0x6]);
      sum += REAL_MUL(window[0x8], b0[0x8]);
      sum += REAL_MUL(window[0xA], b0[0xA]);
      sum += REAL_MUL(window[0xC], b0[0xC]);
      sum += REAL_MUL(window[0xE], b0[0xE]);
      WRITE_SAMPLE(samples,sum,clip);
      b0-=0x10,window-=0x20,samples+=step;
    }
    window += bo1<<1;

    for (j=15;j;j--,b0-=0x20,window-=0x10,samples+=step)
    {
      mpgdec_real sum;
      sum = -REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);

      WRITE_SAMPLE(samples,sum,clip);
    }
  }

  *pnt += 128;

  return clip;
}
