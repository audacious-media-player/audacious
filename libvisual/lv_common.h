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

#ifndef _LV_COMMON_H
#define _LV_COMMON_H

#include <libvisual/lv_mem.h>
#include <libvisual/lv_log.h>
#include <libvisual/lv_error.h>
#include <libvisual/lv_types.h>
#include <libvisual/lv_object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NULL
/**
 * NULL define.
 */
#define NULL	(0)
#endif

#ifndef FALSE
/**
 * FALSE define.
 */
#define FALSE	(0)
#endif

#ifndef TRUE
/**
 * TRUE define.
 */
#define TRUE	(!FALSE)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
	
#endif /* _LV_COMMON_H */
