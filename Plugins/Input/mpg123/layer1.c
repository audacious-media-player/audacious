
/*
 * Mpeg Layer-1 audio decoder
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ...
 *
 */


#include "audacious/output.h"
#include "mpg123.h"
#include "getbits.h"


/* Used by the getbits macros */
static unsigned long rval;

void
I_step_one(unsigned int balloc[],
           unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
    unsigned int *ba = balloc;
    unsigned int *sca = (unsigned int *) scale_index;

    if (fr->stereo) {
        int i;
        int jsbound = fr->jsbound;

        for (i = 0; i < jsbound; i++) {
            *ba++ = mpg123_getbits(4);
            *ba++ = mpg123_getbits(4);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            *ba++ = mpg123_getbits(4);

        ba = balloc;

        for (i = 0; i < jsbound; i++) {
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            if ((*ba++)) {
                *sca++ = mpg123_getbits(6);
                *sca++ = mpg123_getbits(6);
            }
    }
    else {
        int i;

        for (i = 0; i < SBLIMIT; i++)
            *ba++ = mpg123_getbits(4);
        ba = balloc;
        for (i = 0; i < SBLIMIT; i++)
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
    }
}

void
I_step_two(real fraction[2][SBLIMIT],
           unsigned int balloc[2 * SBLIMIT],
           unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
    int i, n;
    int smpb[2 * SBLIMIT];      /* values: 0-65535 */
    int *sample;
    register unsigned int *ba;
    register unsigned int *sca = (unsigned int *) scale_index;

    if (fr->stereo) {
        int jsbound = fr->jsbound;
        register real *f0 = fraction[0];
        register real *f1 = fraction[1];

        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);

        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            if ((n = *ba++))
                *f0++ =
                    (real) (((-1) << n) + (*sample++) +
                            1) * mpg123_muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
            if ((n = *ba++))
                *f1++ =
                    (real) (((-1) << n) + (*sample++) +
                            1) * mpg123_muls[n + 1][*sca++];
            else
                *f1++ = 0.0;
        }
        for (i = jsbound; i < SBLIMIT; i++) {
            if ((n = *ba++)) {
                real samp = (((-1) << n) + (*sample++) + 1);

                *f0++ = samp * mpg123_muls[n + 1][*sca++];
                *f1++ = samp * mpg123_muls[n + 1][*sca++];
            }
            else
                *f0++ = *f1++ = 0.0;
        }
        for (i = fr->down_sample_sblimit; i < 32; i++)
            fraction[0][i] = fraction[1][i] = 0.0;
    }
    else {
        register real *f0 = fraction[0];

        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++)
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++) {
            if ((n = *ba++))
                *f0++ =
                    (real) (((-1) << n) + (*sample++) +
                            1) * mpg123_muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
        }
        for (i = fr->down_sample_sblimit; i < 32; i++)
            fraction[0][i] = 0.0;
    }
}

int
mpg123_do_layer1(struct frame *fr)
{
    int i, stereo = fr->stereo;
    unsigned int balloc[2 * SBLIMIT];
    unsigned int scale_index[2][SBLIMIT];
    real fraction[2][SBLIMIT];
    int single = fr->single;

    fr->jsbound =
        (fr->mode == MPG_MD_JOINT_STEREO) ? (fr->mode_ext << 2) + 4 : 32;

    if (stereo == 1 || single == 3)
        single = 0;

    I_step_one(balloc, scale_index, fr);

    for (i = 0; i < SCALE_BLOCK; i++) {
        I_step_two(fraction, balloc, scale_index, fr);

        if (single >= 0) {
            (fr->synth_mono) ((real *) fraction[single], mpg123_pcm_sample,
                              &mpg123_pcm_point);
        }
        else {
            int p1 = mpg123_pcm_point;

            (fr->synth) ((real *) fraction[0], 0, mpg123_pcm_sample, &p1);
            (fr->synth) ((real *) fraction[1], 1, mpg123_pcm_sample,
                         &mpg123_pcm_point);
        }
        if (mpg123_info->output_audio && mpg123_info->jump_to_time == -1) {
            produce_audio(mpg123_ip.output->written_time(),
                          mpg123_cfg.resolution ==
                          16 ? FMT_S16_NE : FMT_U8,
                          mpg123_cfg.channels ==
                          2 ? fr->stereo : 1, mpg123_pcm_point,
                          mpg123_pcm_sample, &mpg123_info->going);
        }

        mpg123_pcm_point = 0;
    }

    return 1;
}
