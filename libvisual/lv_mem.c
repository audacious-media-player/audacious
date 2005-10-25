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

#include <string.h>
#include <stdlib.h>

#include "lv_mem.h"
#include "lv_common.h"
#include "lv_log.h"
#include "lv_error.h"
#include "lv_cpu.h"

/**
 * @defgroup VisMem VisMem
 * @{
 */

/**
 * Allocates @a nbytes of memory initialized to 0.
 *
 * @param nbytes N bytes of mem requested to be allocated.
 * 
 * @return On success, a pointer to a new allocated memory initialized
 * to 0 of size @a nbytes, on failure, program is aborted, so you never need
 * to check the return value.
 */
void *visual_mem_malloc0 (visual_size_t nbytes)
{
	void *buf = malloc (nbytes);

	visual_log_return_val_if_fail (nbytes > 0, NULL);

	if (buf == NULL) {
		visual_log (VISUAL_LOG_ERROR, "Cannot get %" VISUAL_SIZE_T_FORMAT " bytes of memory", nbytes);
		return NULL;
	}
	
	memset (buf, 0, nbytes);

	return buf;
}

/**
 * Frees allocated memory.
 *
 * @param ptr Frees memory to which ptr points to.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MEM_NULL on failure.
 */
int visual_mem_free (void *ptr)
{
	visual_log_return_val_if_fail (ptr != NULL, -VISUAL_ERROR_MEM_NULL);

	free (ptr);

	return VISUAL_OK;
}

/**
 * Copies a block of mem using optimized methods.
 *
 * @param dest Pointer to destination.
 * @param src Pointer to source.
 * @param n The number of bytes to be copied.
 *
 * @return Pointer to dest.
 */
void *visual_mem_copy (void *dest, const void *src, size_t n)
{
	VisCPU *cpucaps = visual_cpu_get_caps ();
	uint32_t *d = dest;
	const uint32_t *s = src;
	uint8_t *dc = dest;
	const uint8_t *sc = src;

	/* FIXME Add mmx,sse versions, optionally with prefetching and such
	 * with checking for optimal scan lines */

	/* FIXME #else for the VISUAL_ARCH_X86 */

	/* FIXME fix this stuff ! */
	return memcpy (dest, src, n);
	
	if (cpucaps->hasMMX == 1) {
#ifdef VISUAL_ARCH_X86
		while (n > 64) {
			__asm __volatile
				(//"\n\t prefetch 256(%0)" /* < only use when 3dnow is present */
				 //"\n\t prefetch 320(%0)"
				 "\n\t movq (%0), %%mm0"
				 "\n\t movq 8(%0), %%mm1"
				 "\n\t movq 16(%0), %%mm2"
				 "\n\t movq 24(%0), %%mm3"
				 "\n\t movq 32(%0), %%mm4"
				 "\n\t movq 40(%0), %%mm5"
				 "\n\t movq 48(%0), %%mm6"
				 "\n\t movq 56(%0), %%mm7"
				 "\n\t movntq %%mm0, (%1)"
				 "\n\t movntq %%mm1, 8(%1)"
				 "\n\t movntq %%mm2, 16(%1)"
				 "\n\t movntq %%mm3, 24(%1)"
				 "\n\t movntq %%mm4, 32(%1)"
				 "\n\t movntq %%mm5, 40(%1)"
				 "\n\t movntq %%mm6, 48(%1)"
				 "\n\t movntq %%mm7, 56(%1)"
				 :: "r" (s), "r" (d) : "memory");


			d += 16;
			s += 16;

			n -= 64;
		}
#endif /* VISUAL_ARCH_X86 */
	} else {
		while (n >= 4) {
			*d++ = *s++;
			n -= 4;
		}

		dc = (uint8_t *) d;
		sc = (const uint8_t *) s;

	}
	
	while (n--)
		*dc++ = *sc++;

	return dest;
}

/**
 * @}
 */

