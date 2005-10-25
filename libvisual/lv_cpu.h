/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Chong Kai Xiong <descender@phreaker.net>
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

#ifndef _LV_CPU_H
#define _LV_CPU_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
	VISUAL_CPU_TYPE_MIPS,
	VISUAL_CPU_TYPE_ALPHA,
	VISUAL_CPU_TYPE_SPARC,
	VISUAL_CPU_TYPE_X86,
	VISUAL_CPU_TYPE_POWERPC,
	VISUAL_CPU_TYPE_OTHER
} VisCPUType;

typedef struct _VisCPU VisCPU;

/**
 * This VisCPU structure contains information regarding the processor.
 *
 * @see visual_cpu_get_info
 */
struct _VisCPU {
	VisObject	object;	/**< The VisObject data. */
	VisCPUType	type;	/**< Contains the CPU/arch type. */
	int		nrcpu;	/**< Contains the number of CPUs in the system. */

	/* Feature flags */
	int		x86cpuType;
	int		cacheline;
	int		hasTSC;
	int		hasMMX;
	int		hasMMX2;
	int		hasSSE;
	int		hasSSE2;
	int		has3DNow;
	int		has3DNowExt;

	int		hasAltiVec;
};

void visual_cpu_initialize (void);
VisCPU *visual_cpu_get_caps (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_CPU_H */
