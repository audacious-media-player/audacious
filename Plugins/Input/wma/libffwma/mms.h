/*
 * Copyright (C) 2002-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: mms.h,v 1.11 2006/06/06 09:49:15 shawarma Exp $
 *
 * libmms public header
 */

/* TODO/dexineification:
 * + functions needed:
 * 	- _x_io_*()
 * 	- xine_malloc() [?]
 * 	- xine_fast_memcpy() [?]
 */

#ifndef HAVE_MMS_H
#define HAVE_MMS_H

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>

/* #include "xine_internal.h" */

#include "bswap.h"
#include "mmsio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct mms_s mms_t;

mms_t*   mms_connect (mms_io_t *io, void *data, const char *url, int bandwidth);

int      mms_read (mms_io_t *io, mms_t *instance, char *data, int len);
uint32_t mms_get_length (mms_t *instance);
void     mms_close (mms_t *instance);

int      mms_peek_header (mms_t *instance, char *data, int maxsize);

off_t    mms_get_current_pos (mms_t *instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

