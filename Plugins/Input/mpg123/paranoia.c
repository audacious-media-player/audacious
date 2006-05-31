/* libmpgdec: An advanced MPEG layer 1/2/3 decoder.
 * psycho.c: Psychoaccoustic modeling.
 *
 * Copyright (C) 2005-2006 William Pitcock <nenolod@nenolod.net>
 * Portions copyright (C) 2001 Rafal Bosak <gyver@fanthom.irc.pl>
 * Portions copyright (C) 1999 Michael Hipp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"

int bext_level;
int stereo_level;
int filter_level;
int harmonics_level;
int bext_sfactor;
int stereo_sfactor;
int harmonics_sfactor;
int enable_plugin = 1;
int lsine[65536];
int rsine[65536];

/*
 * Initialize and configure the psychoaccoustics engine.
 */
void psycho_init(void)
{
    int i, x;
    double lsum;
    double rsum;

    bext_level = 34;
    stereo_level = stereo_sfactor = 16;
    filter_level = 3;
    harmonics_level = harmonics_sfactor = 43;

    bext_sfactor = (float)(((float)16384 * 10) / (float)(bext_level + 1)) + (float)(102 - bext_level) * 128;

#define COND 0
    /* calculate sinetables */
    for (i = 0; i < 32768; ++i)
    {
	lsum = rsum = 0;
	if (COND || i < 32768 )lsum+=  ((cos((double)i * 3.141592535 / 32768/2	)) + 0) / 2;
	if (COND || i < 16384 )rsum-=  ((cos((double)i * 3.141592535 / 16384/2	)) + 0) / 4;
	rsum = lsum;

	if (COND || i < 8192 ) lsum += ((cos((double)i * 3.141592535 / 8192/2	 )) + 0) /  8;
	if (COND || i < 5641 ) rsum += ((cos((double)i * 3.141592535 / 5641.333333/2)) + 0) /  8;

	lsine[32768 + i] = (double)(lsum - 0.5) * 32768 * 1.45;
	lsine[32768 - i] = lsine[32768 + i];
	rsine[32768 + i] = (double)(rsum - 0.5) * 32768 * 1.45;
	rsine[32768 - i] = rsine[32768 + i];

	x = i;
    }
}

/*
 * This routine computes a reverb for the track.
 */

#define DELAY2 21000
#define DELAY1 35000
#define DELAY3 14000
#define DELAY4 5

#define BUF_SIZE DELAY1+DELAY2+DELAY3+DELAY4

void echo3d(gint16 *data, int datasize)
{
  int x;
  int left, right, dif, left0, right0, left1, right1, left2, right2, left3, right3, left4, right4, leftc, rightc, lsf, rsf;
  static int left0p = 0, right0p = 0;
  static int rangeErrorsUp = 0;
  static int rangeErrorsDown = 0;
  gint16 *dataptr;
  static int l0, l1, l2, r0, r1, r2, ls, rs, ls1, rs1;
  static int ll0, ll1, ll2, rr0, rr1, rr2;
  static int lharmb = 0, rharmb = 0, lhfb = 0, rhfb = 0;
  int lharm0, rharm0;
  static gint16 buf[BUF_SIZE];
  static int bufPos1 = 1 + BUF_SIZE - DELAY1;
  static int bufPos2 = 1 + BUF_SIZE - DELAY1 - DELAY2;
  static int bufPos3 = 1 + BUF_SIZE - DELAY1 - DELAY2 - DELAY3;
  static int bufPos4 = 1 + BUF_SIZE - DELAY1 - DELAY2 - DELAY3 - DELAY4;
  dataptr = data;

  for (x = 0; x < datasize; x += 4) {

    // ************ load sample **********
    left0 = dataptr[0];	
    right0 = dataptr[1];

    // ************ slightly expand stereo for direct input **********
    ll0=left0;rr0=right0;
    dif = (ll0+ll1+ll2 - rr0-rr1-rr2) * stereo_sfactor / 256;
    left0 += dif;
    right0 -= dif;
    ll2= ll1; ll1= ll0;
    rr2= rr1; rr1= rr0;
    
    // ************ echo from buffer - first echo **********
    // ************  **********
    left1 = buf[bufPos1++];
    if (bufPos1 == BUF_SIZE)
      bufPos1 = 0;
    right1 = buf[bufPos1++];
    if (bufPos1 == BUF_SIZE)
      bufPos1 = 0;

    // ************ highly expand stereo for first echo **********
    dif = (left1 - right1);
    left1 = left1 + dif;
    right1 = right1 - dif;

    // ************ second echo  **********
    left2 = buf[bufPos2++];
    if (bufPos2 == BUF_SIZE)
      bufPos2 = 0;	
    right2 = buf[bufPos2++];
    if (bufPos2 == BUF_SIZE)
      bufPos2 = 0;

    // ************ expand stereo for second echo **********
    dif = (left2 - right2);
    left2 = left2 - dif;
    right2 = right2 - dif;

    // ************ third echo  **********
    left3 = buf[bufPos3++];
    if (bufPos3 == BUF_SIZE)
      bufPos3 = 0;	
    right3 = buf[bufPos3++];
    if (bufPos3 == BUF_SIZE)
      bufPos3 = 0;

    // ************ fourth echo  **********
    left4 = buf[bufPos4++];
    if (bufPos4 == BUF_SIZE)
      bufPos4 = 0;	
    right4 = buf[bufPos4++];
    if (bufPos4 == BUF_SIZE)
      bufPos4 = 0;

    left3 = (left4+left3) / 2;
    right3 = (right4+right3) / 2;

    // ************ expand stereo for second echo **********
    dif = (left4 - right4);
    left3 = left3 - dif;
    right3 = right3 - dif;

    // ************ a weighted sum taken from reverb buffer **********
    leftc = left1 / 9 + right2 /8  + left3 / 8;
    rightc = right1 / 11 + left2 / 9 + right3 / 10;

    left = left0p;
    right = right0p;

    l0 = leftc + left0 / 2;
    r0 = rightc + right0 / 2;

    ls = l0 + l1 + l2;	// do not reverb high frequencies (filter)
    rs = r0 + r1 + r2;  //

    // ************ add some extra even harmonics **********
    // ************ or rather specific nonlinearity

    lhfb = lhfb + (ls * 32768 - lhfb) / 32;
    rhfb = rhfb + (rs * 32768 - rhfb) / 32;

    lsf = ls - lhfb / 32768;
    rsf = rs - rhfb / 32768;

    lharm0 = 0
	+ ((lsf + 10000) * ((((lsine[((lsf/4) + 32768 + 65536) % 65536] * harmonics_sfactor)) / 64))) / 32768
       	- ((lsine[((lsf/4) + 32768 +65536) % 65536]) * harmonics_sfactor) / 128
	;

    rharm0 =
	+ ((rsf + 10000) * ((((lsine[((rsf/4) + 32768 + 65536) % 65536] * harmonics_sfactor)) / 64))) / 32768
     	- ((rsine[((rsf/4) + 32768 +65536) % 65536]) * harmonics_sfactor) / 128
	;

    lharmb = lharmb + (lharm0 * 32768 - lharmb) / 16384;
    rharmb = rharmb + (rharm0 * 32768 - rharmb) / 16384;

    // ************ for convolution filters **********
    l2= l1; r2= r1;
    l1 = l0; r1 = r0;
    ls1 = ls; rs1 = rs;

    left  = 0 + lharm0 - lharmb / 32768 + left;
    right = 0 + rharm0 - rharmb / 32768 + right;

    left0p = left0;
    right0p = right0;


    // ************ limiter **********
    if (left < -32768) {
	left = -32768;	// limit
	rangeErrorsDown++;
    }
    else if (left > 32767) {
	left = 32767;
	rangeErrorsUp++;
    }
    if (right < -32768) {
	right = -32768;
	rangeErrorsDown++;
    }
    else if (right > 32767) {
	right = 32767;
	rangeErrorsUp++;
    }
    // ************ store sample **********
    dataptr[0] = left;
    dataptr[1] = right;
    dataptr += 2;

   }
}

/*
 * simple pith shifter, plays short fragments at 1.5 speed
 */
void pitchShifter(const int lin, const int rin, int *lout, int *rout)
{ 
#define SH_BUF_SIZE 100 * 3
    static gint16 shBuf[SH_BUF_SIZE];
    static int shBufPos = SH_BUF_SIZE - 6;
    static int shBufPos1 = SH_BUF_SIZE - 6;
    static int cond;

    shBuf[shBufPos++] = lin;
    shBuf[shBufPos++] = rin;

    if (shBufPos == SH_BUF_SIZE) shBufPos = 0;

    switch (cond){
	case 1:
	    *lout = (shBuf[shBufPos1 + 0] * 2 + shBuf[shBufPos1 + 2])/4;
	    *rout = (shBuf[shBufPos1 + 1] * 2 + shBuf[shBufPos1 + 3])/4;
	    break;
	case 0:
	    *lout = (shBuf[shBufPos1 + 4] * 2 + shBuf[shBufPos1 + 2])/4;
	    *rout = (shBuf[shBufPos1 + 5] * 2 + shBuf[shBufPos1 + 3])/4;
	    cond = 2;
	    shBufPos1 += 6;
	    if (shBufPos1 == SH_BUF_SIZE) {
		shBufPos1 = 0;
	    }
	    break;
    }
    cond--;
}


struct Interpolation{
    int acount;		// counter
    int lval, rval;	// value
    int sal, sar;	// sum
    int al, ar;		
    int a1l, a1r;
};

/*
 * interpolation routine for ampliude and "energy"
 */
static inline void interpolate(struct Interpolation *s, int l, int r)
{
#define AMPL_COUNT 64
    int a0l, a0r, dal = 0, dar = 0;

    if (l < 0) l = -l;
    if (r < 0) r = -r;

    s->lval += l / 8;
    s->rval += r / 8;

    s->lval = (s->lval * 120) / 128;
    s->rval = (s->rval * 120) / 128;

    s->sal += s->lval;
    s->sar += s->rval;

    s->acount++;
    if (s->acount == AMPL_COUNT){
	s->acount = 0;
	a0l = s->a1l;
	a0r = s->a1r;
	s->a1l = s->sal / AMPL_COUNT;
	s->a1r = s->sar / AMPL_COUNT;
	s->sal = 0;
	s->sar = 0;
	dal = s->a1l - a0l;
	dar = s->a1r - a0r;
	s->al = a0l * AMPL_COUNT;
	s->ar = a0r * AMPL_COUNT;
    }

    s->al += dal;
    s->ar += dar;
}

/*
 * calculate scalefactor for mixer
 */
inline int calc_scalefactor(int a, int e)
{
    int x;

    if (a > 8192) a = 8192;
    else if (a < 0) a = 0;

    if (e > 8192) e = 8192;
    else if (e < 0) e = 0;

    x = ((e+500) * 4096 )/ (a + 300) + e;

    if (x > 16384) x = 16384;
    else if (x < 0) x = 0;
    return x;
}

static struct Interpolation bandext_energy;
static struct Interpolation bandext_amplitude;

/*
 * exact bandwidth extender ("exciter") routine
 */
static void bandext(gint16 *data, const int datasize)
{

    int x;
    static int saw;		// test stuff
    int left, right;
    gint16 *dataptr = data;
    static int lprev0, rprev0, lprev1, rprev1, lprev2, rprev2;
    int left0, right0, left1, right1, left2, right2, left3, right3;
    static int lamplUp, lamplDown;
    static int ramplUp, ramplDown;
    int lampl, rampl;
    int tmp;

    for (x = 0; x < datasize; x += 4) {

	// ************ load sample **********
       	left0 = dataptr[0];
       	right0 = dataptr[1];

	// ************ highpass filter part 1 **********
	left1  = (left0  - lprev0) * 56880 / 65536;
	right1 = (right0 - rprev0) * 56880 / 65536;

	left2  = (left1  - lprev1) * 56880 / 65536;
	right2 = (right1 - rprev1) * 56880 / 65536;

	left3  = (left2  - lprev2) * 56880 / 65536;
	right3 = (right2 - rprev2) * 56880 / 65536;

	switch (filter_level){
	    case 1:
	       	pitchShifter(left1, right1, &left, &right);
		break;
	    case 2:
	       	pitchShifter(left2, right2, &left, &right);
		break;
	    case 3:
	       	pitchShifter(left3, right3, &left, &right);
		break;
	}

	// ************ amplitude detector ************
	tmp = left1 + lprev1;
	if      (tmp * 16 > lamplUp  ) lamplUp   += (tmp - lamplUp  );
	else if (tmp * 16 < lamplDown) lamplDown += (tmp - lamplDown);
	lamplUp   = (lamplUp   * 1000) /1024;
	lamplDown = (lamplDown * 1000) /1024;
	lampl = lamplUp - lamplDown;

	tmp = right1 + rprev1;
	if      (tmp * 16 > ramplUp  ) ramplUp   += (tmp - ramplUp  );
	else if (tmp * 16 < ramplDown) ramplDown += (tmp - ramplDown);
	ramplUp   = (ramplUp   * 1000) /1024;
	ramplDown = (ramplDown * 1000) /1024;
	rampl = ramplUp - ramplDown;

	interpolate(&bandext_amplitude, lampl, rampl);

	// ************ "sound energy" detector (approx. spectrum complexity) ***********
	interpolate(&bandext_energy, left0  - lprev0, right0 - rprev0);

	// ************ mixer ***********
	left   = left0 + left  * calc_scalefactor(bandext_amplitude.lval, bandext_energy.lval) / bext_sfactor; 
	right  = right0 + right * calc_scalefactor(bandext_amplitude.rval, bandext_energy.rval) / bext_sfactor; //16384

	saw = (saw + 2048) & 0x7fff;

	lprev0 = left0;
	rprev0 = right0;
	lprev1 = left1;
	rprev1 = right1;
	lprev2 = left2;
	rprev2 = right2;

	if (left < -32768) left = -32768;
       	else if (left > 32767) left = 32767;
       	if (right < -32768) right = -32768;
       	else if (right > 32767) right = 32767;

	dataptr[0] = left;
       	dataptr[1] = right;
       	dataptr += 2;
    }
}

int psycho_process(void *data, int length, int nch)
{
    if (nch != 2)
        return length;			/* XXX: we cant process mono yet */

    echo3d((gint16 *)data, length);
    bandext((gint16 *)data, length);
    return length;
}

