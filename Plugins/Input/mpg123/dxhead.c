/*
 * Handle Xing vbr header
 */
#include "config.h"
#include "dxhead.h"
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <glib.h>

#define GET_INT32BE(b) \
(i = (b[0] << 24) | (b[1] << 16) | b[2] << 8 | b[3], b += 4, i)

int
mpg123_get_xing_header(xing_header_t * xing, unsigned char *buf)
{
    int i, head_flags;
    int id, mode;

    memset(xing, 0, sizeof(xing_header_t));

    /* get selected MPEG header data */
    id = (buf[1] >> 3) & 1;
    mode = (buf[3] >> 6) & 3;
    buf += 4;

    /* Skip the sub band data */
    if (id) {
        /* mpeg1 */
        if (mode != 3)
            buf += 32;
        else
            buf += 17;
    }
    else {
        /* mpeg2 */
        if (mode != 3)
            buf += 17;
        else
            buf += 9;
    }

    if (strncmp((char *) buf, "Xing", 4))
        return 0;
    buf += 4;

    head_flags = GET_INT32BE(buf);

    if (head_flags & FRAMES_FLAG)
        xing->frames = GET_INT32BE(buf);
    if (xing->frames < 1)
        return 0;
    if (head_flags & BYTES_FLAG)
        xing->bytes = GET_INT32BE(buf);

    if (head_flags & TOC_FLAG) {
        for (i = 0; i < 100; i++) {
            xing->toc[i] = buf[i];
            if (i > 0 && xing->toc[i] < xing->toc[i - 1])
                return 0;
        }
        if (xing->toc[99] == 0)
            return 0;
        buf += 100;
    }
    else
        for (i = 0; i < 100; i++)
            xing->toc[i] = (i * 256) / 100;

#ifdef XING_DEBUG
    for (i = 0; i < 100; i++) {
        if ((i % 10) == 0)
            fprintf(stderr, "\n");
        fprintf(stderr, " %3d", xing->toc[i]);
    }
#endif

    return 1;
}

int
mpg123_seek_point(xing_header_t * xing, float percent)
{
    /* interpolate in TOC to get file seek point in bytes */
    int a, seekpoint;
    float fa, fb, fx;

    percent = CLAMP(percent, 0, 100);
    a = MIN(percent, 99);

    fa = xing->toc[a];

    if (a < 99)
        fb = xing->toc[a + 1];
    else
        fb = 256;

    fx = fa + (fb - fa) * (percent - a);
    seekpoint = (1.0f / 256.0f) * fx * xing->bytes;

    return seekpoint;
}
