/*
 * This code is derivative of guess.c of Gauche-0.8.3.
 * The following is the original copyright notice.
 */

/*
 *   Copyright (c) 2000-2003 Shiro Kawai, All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _LIBGUESS_H
#define _LIBGUESS_H	1

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
#ifdef LIBGUESS_CORE

const char *guess_jp(const char *buf, int buflen);
const char *guess_tw(const char *buf, int buflen);
const char *guess_cn(const char *buf, int buflen);
const char *guess_kr(const char *buf, int buflen);
const char *guess_ru(const char *buf, int buflen);
const char *guess_ar(const char *buf, int buflen);
const char *guess_tr(const char *buf, int buflen);
const char *guess_gr(const char *buf, int buflen);
const char *guess_hw(const char *buf, int buflen);
const char *guess_pl(const char *buf, int buflen);
const char *guess_bl(const char *buf, int buflen);

typedef const char *(*guess_impl_f)(const char *buf, int len);

#endif

int libguess_validate_utf8(const char *buf, int buflen);

#define GUESS_REGION_JP		"japanese"
#define GUESS_REGION_TW		"taiwanese"
#define GUESS_REGION_CN		"chinese"
#define GUESS_REGION_KR		"korean"
#define GUESS_REGION_RU		"russian"
#define GUESS_REGION_AR		"arabic"
#define GUESS_REGION_TR		"turkish"
#define GUESS_REGION_GR		"greek"
#define GUESS_REGION_HW		"hebrew"
#define GUESS_REGION_PL		"polish"
#define GUESS_REGION_BL		"baltic"

typedef void (*libguess_result_f)(const char *encodingname, const char *res);

const char *libguess_determine_encoding(const char *buf, int buflen, const char *langset);

#ifdef __cplusplus
}
#endif

#endif
