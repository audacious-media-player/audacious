/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This has been derived from the implementation in the FreeBSD libc.
 *
 * 2000-12-28  Håvard Kvålen <havardk@xmms.org>:
 * Stripped down to only mkdtemp() and made more portable
 * 
 */

#ifndef HAVE_MKDTEMP

#if 0
static const char rcsid[] =
    "$FreeBSD: /c/ncvs/src/lib/libc/stdio/mktemp.c,v 1.20 2000/11/10 23:27:55 kris Exp $";
#endif

#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

static const char padchar[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char *
mkdtemp(char *path)
{
    register char *start, *trv, *suffp;
    char *pad;
    struct stat sbuf;
    int rval;

    for (trv = path; *trv; ++trv);
    suffp = trv;
    --trv;
    if (trv < path) {
        errno = EINVAL;
        return NULL;
    }

    /* Fill space with random characters */
    /*
     * I hope this is random enough.  The orginal implementation
     * uses arc4random(3) which is not available everywhere.
     */
    while (*trv == 'X') {
        int randv = g_random_int_range(0, sizeof(padchar) - 1);
        *trv-- = padchar[randv];
    }
    start = trv + 1;

    /*
     * check the target directory.
     */
    for (;; --trv) {
        if (trv <= path)
            break;
        if (*trv == '/') {
            *trv = '\0';
            rval = stat(path, &sbuf);
            *trv = '/';
            if (rval != 0)
                return NULL;
            if (!S_ISDIR(sbuf.st_mode)) {
                errno = ENOTDIR;
                return NULL;
            }
            break;
        }
    }

    for (;;) {
        if (mkdir(path, 0700) == 0)
            return path;
        if (errno != EEXIST)
            return NULL;

        /* If we have a collision, cycle through the space of filenames */
        for (trv = start;;) {
            if (*trv == '\0' || trv == suffp)
                return NULL;
            pad = strchr(padchar, *trv);
            if (pad == NULL || !*++pad)
                *trv++ = padchar[0];
            else {
                *trv++ = *pad;
                break;
            }
        }
    }
 /*NOTREACHED*/}

#endif                          /* HAVE_MKDTEMP */
