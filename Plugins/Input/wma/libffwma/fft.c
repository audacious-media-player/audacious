/*
 * FFT/IFFT transforms
 * Copyright (c) 2002 Fabrice Bellard.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA
 */

/**
 * @file fft.c
 * FFT/IFFT transforms.
 */

#include "dsputil.h"

#ifdef HAVE_ALTIVEC

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#ifdef CONFIG_DARWIN
#include <sys/sysctl.h>
#else /* CONFIG_DARWIN */
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void sigill_handler (int sig)
{
    if (!canjump) {
        signal (sig, SIG_DFL);
        raise (sig);
    }

    canjump = 0;
    siglongjmp (jmpbuf, 1);
}
#endif /* CONFIG_DARWIN */


#define WORD_0 0x00,0x01,0x02,0x03
#define WORD_1 0x04,0x05,0x06,0x07
#define WORD_2 0x08,0x09,0x0a,0x0b
#define WORD_3 0x0c,0x0d,0x0e,0x0f
#define WORD_s0 0x10,0x11,0x12,0x13
#define WORD_s1 0x14,0x15,0x16,0x17
#define WORD_s2 0x18,0x19,0x1a,0x1b
#define WORD_s3 0x1c,0x1d,0x1e,0x1f

#ifdef CONFIG_DARWIN
#define vcprm(a,b,c,d) (const vector unsigned char)(WORD_ ## a, WORD_ ## b, WORD_ ## c, WORD_ ## d)
#else
#define vcprm(a,b,c,d) (const vector unsigned char){WORD_ ## a, WORD_ ## b, WORD_ ## c, WORD_ ## d}
#endif

// vcprmle is used to keep the same index as in the SSE version.
// it's the same as vcprm, with the index inversed
// ('le' is Little Endian)
#define vcprmle(a,b,c,d) vcprm(d,c,b,a)

// used to build inverse/identity vectors (vcii)
// n is _n_egative, p is _p_ositive
#define FLOAT_n -1.
#define FLOAT_p 1.


#ifdef CONFIG_DARWIN
#define vcii(a,b,c,d) (const vector float)(FLOAT_ ## a, FLOAT_ ## b, FLOAT_ ## c, FLOAT_ ## d)
#else
#define vcii(a,b,c,d) (const vector float){FLOAT_ ## a, FLOAT_ ## b, FLOAT_ ## c, FLOAT_ ## d}
#endif

int has_altivec(void)
{
#ifdef CONFIG_DARWIN
    int sels[2] = {CTL_HW, HW_VECTORUNIT};
    int has_vu = 0;
    size_t len = sizeof(has_vu);
    int err;

    err = sysctl(sels, 2, &has_vu, &len, NULL, 0);

    if (err == 0) return (has_vu != 0);
#else /* CONFIG_DARWIN */
/* no Darwin, do it the brute-force way */
/* this is borrowed from the libmpeg2 library */
    {
      signal (SIGILL, sigill_handler);
      if (sigsetjmp (jmpbuf, 1)) {
        signal (SIGILL, SIG_DFL);
      } else {
        canjump = 1;

        asm volatile ("mtspr 256, %0\n\t"
                      "vand %%v0, %%v0, %%v0"
                      :
                      : "r" (-1));

        signal (SIGILL, SIG_DFL);
        return 1;
      }
    }
#endif /* CONFIG_DARWIN */
    return 0;
}


void fft_calc_altivec(FFTContext *s, FFTComplex *z)
{
#ifdef CONFIG_DARWIN
    register const vector float vczero = (const vector float)(0.);
#else
    register const vector float vczero = (const vector float){0.,0.,0.,0.};
#endif

    int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;
    register FFTComplex *p, *q;
    FFTComplex *cptr, *cptr1;
    int k;

    np = 1 << ln;

    {
        vector float *r, a, b, a1, c1, c2;

        r = (vector float *)&z[0];

        c1 = vcii(p,p,n,n);

        if (s->inverse)
            {
                c2 = vcii(p,p,n,p);
            }
        else
            {
                c2 = vcii(p,p,p,n);
            }

        j = (np >> 2);
        do {
            a = vec_ld(0, r);
            a1 = vec_ld(sizeof(vector float), r);

            b = vec_perm(a,a,vcprmle(1,0,3,2));
            a = vec_madd(a,c1,b);
            /* do the pass 0 butterfly */

            b = vec_perm(a1,a1,vcprmle(1,0,3,2));
            b = vec_madd(a1,c1,b);
            /* do the pass 0 butterfly */

            /* multiply third by -i */
            b = vec_perm(b,b,vcprmle(2,3,1,0));

            /* do the pass 1 butterfly */
            vec_st(vec_madd(b,c2,a), 0, r);
            vec_st(vec_nmsub(b,c2,a), sizeof(vector float), r);

            r += 2;
        } while (--j != 0);
    }
    /* pass 2 .. ln-1 */

    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;

    cptr1 = s->exptab1;
    do {
        p = z;
        q = z + nloops;
        j = nblocks;
        do {
            cptr = cptr1;
            k = nloops >> 1;
            do {
                vector float a,b,c,t1;

                a = vec_ld(0, (float*)p);
                b = vec_ld(0, (float*)q);

                /* complex mul */
                c = vec_ld(0, (float*)cptr);
                /*  cre*re cim*re */
                t1 = vec_madd(c, vec_perm(b,b,vcprmle(2,2,0,0)),vczero);
                c = vec_ld(sizeof(vector float), (float*)cptr);
                /*  -cim*im cre*im */
                b = vec_madd(c, vec_perm(b,b,vcprmle(3,3,1,1)),t1);

                /* butterfly */
                vec_st(vec_add(a,b), 0, (float*)p);
                vec_st(vec_sub(a,b), 0, (float*)q);

                p += 2;
                q += 2;
                cptr += 4;
            } while (--k);

            p += nloops;
            q += nloops;
        } while (--j);
        cptr1 += nloops * 2;
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    } while (nblocks != 0);
}

#endif

/**
 * The size of the FFT is 2^nbits. If inverse is TRUE, inverse FFT is
 * done 
 */
int fft_inits(FFTContext *s, int nbits, int inverse)
{
    int i, j, m, n;
    float alpha, c1, s1, s2;
    
    s->nbits = nbits;
    n = 1 << nbits;

    s->exptab = av_malloc((n / 2) * sizeof(FFTComplex));
    if (!s->exptab)
        goto fail;
    s->revtab = av_malloc(n * sizeof(uint16_t));
    if (!s->revtab)
        goto fail;
    s->inverse = inverse;

    s2 = inverse ? 1.0 : -1.0;
        
    for(i=0;i<(n/2);i++) {
        alpha = 2 * M_PI * (float)i / (float)n;
        c1 = cos(alpha);
        s1 = sin(alpha) * s2;
        s->exptab[i].re = c1;
        s->exptab[i].im = s1;
    }
    s->fft_calc = fft_calc_c;
    s->exptab1 = NULL;
    /* compute constant table for HAVE_SSE version */
#if (defined(HAVE_ALTIVEC))
    {
        int has_vectors = 0;

#if defined(HAVE_ALTIVEC)
        has_vectors =  has_altivec();
#endif
        if (has_vectors) {
            int np, nblocks, np2, l;
            FFTComplex *q;
            
            np = 1 << nbits;
            nblocks = np >> 3;
            np2 = np >> 1;
            s->exptab1 = av_malloc(np * 2 * sizeof(FFTComplex));
            if (!s->exptab1)
                goto fail;
            q = s->exptab1;
            do {
                for(l = 0; l < np2; l += 2 * nblocks) {
                    *q++ = s->exptab[l];
                    *q++ = s->exptab[l + nblocks];

                    q->re = -s->exptab[l].im;
                    q->im = s->exptab[l].re;
                    q++;
                    q->re = -s->exptab[l + nblocks].im;
                    q->im = s->exptab[l + nblocks].re;
                    q++;
                }
                nblocks = nblocks >> 1;
            } while (nblocks != 0);
            av_freep(&s->exptab);
            s->fft_calc = fft_calc_altivec;
        }
    }
#endif

    /* compute bit reverse table */

    for(i=0;i<n;i++) {
        m=0;
        for(j=0;j<nbits;j++) {
            m |= ((i >> j) & 1) << (nbits-j-1);
        }
        s->revtab[i]=m;
    }
    return 0;
 fail:
    av_freep(&s->revtab);
    av_freep(&s->exptab);
    av_freep(&s->exptab1);
    return -1;
}

/* butter fly op */
#define BF(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  FFTSample ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}

#define MUL16(a,b) ((a) * (b))

#define CMUL(pre, pim, are, aim, bre, bim) \
{\
   pre = (MUL16(are, bre) - MUL16(aim, bim));\
   pim = (MUL16(are, bim) + MUL16(bre, aim));\
}

/**
 * Do a complex FFT with the parameters defined in fft_init(). The
 * input data must be permuted before with s->revtab table. No
 * 1.0/sqrt(n) normalization is done.  
 */
void fft_calc_c(FFTContext *s, FFTComplex *z)
{
    int ln = s->nbits;
    int	j, np, np2;
    int	nblocks, nloops;
    register FFTComplex *p, *q;
    FFTComplex *exptab = s->exptab;
    int l;
    FFTSample tmp_re, tmp_im;

    np = 1 << ln;

    /* pass 0 */

    p=&z[0];
    j=(np >> 1);
    do {
        BF(p[0].re, p[0].im, p[1].re, p[1].im, 
           p[0].re, p[0].im, p[1].re, p[1].im);
        p+=2;
    } while (--j != 0);

    /* pass 1 */

    
    p=&z[0];
    j=np >> 2;
    if (s->inverse) {
        do {
            BF(p[0].re, p[0].im, p[2].re, p[2].im, 
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im, 
               p[1].re, p[1].im, -p[3].im, p[3].re);
            p+=4;
        } while (--j != 0);
    } else {
        do {
            BF(p[0].re, p[0].im, p[2].re, p[2].im, 
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im, 
               p[1].re, p[1].im, p[3].im, -p[3].re);
            p+=4;
        } while (--j != 0);
    }
    /* pass 2 .. ln-1 */

    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
    do {
        p = z;
        q = z + nloops;
        for (j = 0; j < nblocks; ++j) {
            BF(p->re, p->im, q->re, q->im,
               p->re, p->im, q->re, q->im);
            
            p++;
            q++;
            for(l = nblocks; l < np2; l += nblocks) {
                CMUL(tmp_re, tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
                BF(p->re, p->im, q->re, q->im,
                   p->re, p->im, tmp_re, tmp_im);
                p++;
                q++;
            }

            p += nloops;
            q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    } while (nblocks != 0);
}

/**
 * Do the permutation needed BEFORE calling fft_calc()
 */
void fft_permute(FFTContext *s, FFTComplex *z)
{
    int j, k, np;
    FFTComplex tmp;
    const uint16_t *revtab = s->revtab;
    
    /* reverse */
    np = 1 << s->nbits;
    for(j=0;j<np;j++) {
        k = revtab[j];
        if (k < j) {
            tmp = z[k];
            z[k] = z[j];
            z[j] = tmp;
        }
    }
}

void fft_end(FFTContext *s)
{
    av_freep(&s->revtab);
    av_freep(&s->exptab);
    av_freep(&s->exptab1);
}

