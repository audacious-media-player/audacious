/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Audio rate-conversion filter
   
   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
*/
#include "xs_filter.h"

/* Let's do some preprocessor magic :) */
#define XS_FVAR(T, P, K) g ## K ## int ## P *sp_ ## T ## P , *dp_ ## T ## P

#define XS_FILTER1(T, P, K, Q)							\
	dataSize /= sizeof(g ## K ## int ## P); 				\
	sp_ ## T ## P = (g ## K ## int ## P *) srcBuf;				\
	dp_ ## T ## P = (g ## K ## int ## P *) destBuf;				\
	while (dataSize-- > 0) {						\
		for (tmp = 0, i = 0; i < oversampleFactor; i++)			\
			tmp += (gint32) ((gint ## P) (*(sp_ ## T ## P ++) Q));	\
		xs_filter_mbn = (tmp + xs_filter_mbn) / (oversampleFactor + 1);	\
		*(dp_ ## T ## P ++) = ((g ## K ## int ## P) xs_filter_mbn) Q ;	\
		}


static gint32 xs_filter_mbn = 0;


gint xs_filter_rateconv(void *destBuf, void *srcBuf, const AFormat audioFormat, const gint oversampleFactor,
			const gint bufSize)
{
	static gint32 tmp;
	XS_FVAR(s, 8,);
	XS_FVAR(u, 8, u);
	XS_FVAR(s, 16,);
	XS_FVAR(u, 16, u);
	gint i;
	gint dataSize = bufSize;

	if (dataSize <= 0)
		return dataSize;

	switch (audioFormat) {
	case FMT_U8:
		XS_FILTER1(u, 8, u, ^0x80)
		    break;

	case FMT_S8:
		XS_FILTER1(s, 8,,)
		    break;


	case FMT_U16_BE:
	case FMT_U16_LE:
	case FMT_U16_NE:
		XS_FILTER1(u, 16, u, ^0x8000)
		    break;

	case FMT_S16_BE:
	case FMT_S16_LE:
	case FMT_S16_NE:
		XS_FILTER1(s, 16,,)
		    break;

	default:
		return -1;
	}

	return 0;
}
