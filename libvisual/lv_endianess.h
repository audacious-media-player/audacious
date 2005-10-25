/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_ENDIANESS_H
#define _LV_ENDIANESS_H

#include <libvisual/lvconfig.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Macros to convert LE <-> BE
 * 'I' stands for integer here.
 */
#define VISUAL_ENDIAN_LE_BE_I16(w) (\
	(((w) & 0xff00) >> 8) |\
	(((w) & 0x00ff) << 8) )

#define VISUAL_ENDIAN_LE_BE_I32(w) (\
	(((w) & 0x000000ff) << 24) | \
	(((w) & 0x0000ff00) << 8) | \
	(((w) & 0x00ff0000) >> 8) | \
	(((w) & 0xff000000) >> 24) )


#if VISUAL_BIG_ENDIAN & VISUAL_LITTLE_ENDIAN
#	error determining system endianess.
#endif

/**
 * Arch. dependant definitions
 */

#if VISUAL_BIG_ENDIAN
#	define VISUAL_ENDIAN_BEI16(x) (x)
#	define VISUAL_ENDIAN_BEI32(x) (x)
#	define VISUAL_ENDIAN_LEI16(x) VISUAL_ENDIAN_LE_BE_I16(x)
#	define VISUAL_ENDIAN_LEI32(x) VISUAL_ENDIAN_LE_BE_I32(x)
#endif

#if VISUAL_LITTLE_ENDIAN
#	define VISUAL_ENDIAN_LEI16(x) (x)
#	define VISUAL_ENDIAN_LEI32(x) (x)
#	define VISUAL_ENDIAN_BEI16(x) VISUAL_ENDIAN_LE_BE_I16(x)
#	define VISUAL_ENDIAN_BEI32(x) VISUAL_ENDIAN_LE_BE_I32(x)
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */   

#endif /* _LV_ENDIANESS_H */
