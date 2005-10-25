/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Jean-Christophe Hoelt <jeko@ios-software.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <lvconfig.h>
#include "lv_common.h"
#include "lv_video.h"

int _lv_blit_overlay_alpha32_mmx (VisVideo *dest, const VisVideo *src, int x, int y)
{
#ifdef VISUAL_ARCH_X86
	uint8_t *destbuf;
	uint8_t *srcbuf;
	int lwidth = (x + src->width);
	int lwidth4;
	int lheight = (y + src->height);
	int ya, xa;
	uint8_t alpha;

	if (lwidth > dest->width)
		lwidth += dest->width - lwidth;

	if (lheight > dest->height)
		lheight += dest->height - lheight;

	destbuf = dest->pixels;
	srcbuf = src->pixels;

	if (lwidth < 0)
		return VISUAL_OK;

	lwidth4 = lwidth * 4;
	
	/* Reset some regs */
	__asm __volatile
		("\n\t pxor %%mm6, %%mm6"
		 ::: "mm6");
	
	destbuf += ((y > 0 ? y : 0) * dest->pitch) + (x > 0 ? x * 4 : 0);
	srcbuf += ((y < 0 ? abs(y) : 0) * src->pitch) + (x < 0 ? abs(x) * 4 : 0);
	for (ya = y > 0 ? y : 0; ya < lheight; ya++) {
		for (xa = x > 0 ? x * 4 : 0; xa < lwidth4; xa += 4) {
			/* pixel = ((alpha * ((src - dest)) / 255) + dest) */
			__asm __volatile
				("\n\t movd %[spix], %%mm0"
				 "\n\t movd %[dpix], %%mm1"
				 "\n\t movq %%mm0, %%mm2"
				 "\n\t movq %%mm0, %%mm3"
				 "\n\t psrlq $24, %%mm2"	/* The alpha */
				 "\n\t movq %%mm0, %%mm4"
				 "\n\t psrld $24, %%mm3"
				 "\n\t psrld $24, %%mm4"
				 "\n\t psllq $32, %%mm2"
				 "\n\t psllq $16, %%mm3"
				 "\n\t por %%mm4, %%mm2"
				 "\n\t punpcklbw %%mm6, %%mm0"	/* interleaving dest */
				 "\n\t por %%mm3, %%mm2"
				 "\n\t punpcklbw %%mm6, %%mm1"	/* interleaving source */
				 "\n\t psubsw %%mm1, %%mm0"	/* (src - dest) part */
				 "\n\t pmullw %%mm2, %%mm0"	/* alpha * (src - dest) */
				 "\n\t psrlw $8, %%mm0"		/* / 256 */
				 "\n\t paddb %%mm1, %%mm0"	/* + dest */
				 "\n\t packuswb %%mm0, %%mm0"
				 "\n\t movd %%mm0, %[dest]"
				 : [dest] "=m" (*destbuf)
				 : [dpix] "m" (*destbuf)
				 , [spix] "m" (*srcbuf)
				 : "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7");

			destbuf += 4;
			srcbuf += 4;
		}

		destbuf += (dest->pitch - ((lwidth - x) * 4)) - (x < 0 ? x * 4 : 0);
		srcbuf += x < 0 ? abs(x) * 4 : 0;
		srcbuf += x + src->width > dest->width ? ((x + (src->pitch / 4)) - dest->width) * 4 : 0;
	}

	__asm __volatile
		("\n\t emms");

	return VISUAL_OK;
#else /* !VISUAL_ARCH_X86 */
	return VISUAL_ERROR_CPU_INVALID_CODE;
#endif
}

int _lv_scale_bilinear_32_mmx (VisVideo *dest, const VisVideo *src)
{
#ifdef VISUAL_ARCH_X86
	uint32_t y;
	uint32_t u, v, du, dv; /* fixed point 16.16 */
	uint32_t *dest_pixel, *src_pixel_rowu, *src_pixel_rowl;

	dest_pixel = dest->pixels;

	du = ((src->width - 1)  << 16) / dest->width;
	dv = ((src->height - 1) << 16) / dest->height;
	v = 0;

	__asm__ __volatile__ ("\n\temms");
	
	for (y = dest->height; y--; v += dv) {
		uint32_t x;
		uint32_t fracU, fracV;     /* fixed point 28.4 [0,1[    */

		if (v >> 16 >= src->height - 1)
			v -= 0x10000;

		src_pixel_rowu = (src->pixel_rows[v >> 16]);
		src_pixel_rowl = (src->pixel_rows[(v >> 16) + 1]);

		/* fracV = frac(v) = v & 0xffff */
		/* fixed point format convertion: fracV >>= 8) */
		fracV = ((v & 0xffff) >> 12) | 0x100000;
		u = 0;


		for (x = dest->width - 1; x--; u += du) {

			/* fracU = frac(u) = u & 0xffff */
			/* fixed point format convertion: fracU >>= 8) */
			fracU  = ((u & 0xffff) >> 12) | 0x100000;
			
			__asm__ __volatile__
				("\n\t pxor %%mm7, %%mm7"
				 /* Prefetching does not show improvement on my Duron (maybe due to its small cache?) */
				 /*"\n\t prefetch  64%[pixel_l]" / * only work on 3now!/SSE cpu */
				 /*"\n\t prefetchw 64%[output]"  / * only work on 3now!/SSE cpu */

				 /* Computing coefs values (Thread #1 and #2) => ends on #C
				  *
				  * notice 0x10 = 1.0 (fixed point 28.4 - like fracU and fracV)
				  *
				  * coef[0] = (0x10 - fracU) * (0x10 - fracV); * UL=0 *
				  * coef[1] = (0x10 - fracU) * fracV;          * LL=1 *
				  * coef[2] = fracU * (0x10 - fracV);          * UR=2 *
				  * coef[3] = fracU * fracV;                   * LR=3 *
				  */

				 /*
				  * Unpacking colors (Thread #3 and #4)
				  */
				 /*
				  * Multiplying colors by coefs (Threads #5 and #6)
				  */
				 /*
				  * Adding colors together. (Thread #7)
				  */

				"#1\n\t movd %[fracu], %%mm4"   /* mm4 = [ 0 | 0 | 0x10 | fracU ] */
				"#2\n\t movd %[fracv], %%mm6"   /* mm6 = [ 0 | 0 | 0x10 | fracV ] */

				"#1\n\t punpcklwd %%mm4, %%mm4" /* mm4 = [ 0x10 | 0x10 | fracU | fracU ] */
				"#2\n\t movq      %%mm6, %%mm3"

				"#1\n\t pxor      %%mm5, %%mm5"
				"#2\n\t punpckldq %%mm6, %%mm6" /* mm6 = [ 0x10 | fracv | 0x10 | fracV ] */ 
				"#3\n\t movq %[pixel_u], %%mm0" /* mm0 = [ col[0] | col[2] ] */

				"#1\n\t punpckldq %%mm4, %%mm5" /* mm5 = [ fracU | fracU | 0 | 0 ] */
				"#2\n\t punpcklwd %%mm7, %%mm3" /* mm3 = [ 0    | 0x10  | 0    | fracV ] */
				"#3\n\t movq      %%mm0, %%mm2"

				"#1\n\t psubusw   %%mm5, %%mm4" /* mm4 = [ 0x10-fracU | 0x10-fracU | fracU | fracU ] */
				"#2\n\t punpckldq %%mm3, %%mm3" /* mm3 = [ 0    | fracV | 0    | fracV ] */
				"#4\n\t movq %[pixel_l], %%mm1" /* mm1 = [ col[1] | col[3] ] */

				"#2\n\t pslld     $16,   %%mm3" /* mm3 = [ fracV | 0 | fracV | 0 ] */
				"#3\n\t punpcklbw %%mm7, %%mm0" /* mm0 = [ col[0] unpacked ] */

				"#2\n\t psubusw   %%mm3, %%mm6" /* mm6 = [ 0x10-fracV | fracV | 0x10-fracV | fracV ] */
				"#4\n\t movq      %%mm1, %%mm3"

				"#C\n\t pmullw    %%mm6, %%mm4" /* mm4 = [ coef[0]|coef[1]|coef[2]|coef[3] ] */
				"#5\n\t movq      %%mm4, %%mm5"

				"#4\n\t punpcklbw %%mm7, %%mm1" /* mm1 = [ col[1] unpacked ] */
				"#6\n\t punpckhwd %%mm4, %%mm4" /* mm4 = [ coef[1]|coef[1]|coef[0]|coef[0] ] */

				"#3\n\t punpckhbw %%mm7, %%mm2" /* mm2 = [ col[2] unpacked ] */
				"#5\n\t punpcklwd %%mm5, %%mm5" /* mm5 = [ coef[2]|coef[2]|coef[3]|coef[3] ] */

				"#4\n\t punpckhbw %%mm7, %%mm3" /* mm3 = [ col[3] unpacked ] */
				"#5\n\t movq      %%mm5, %%mm6"

				"#6\n\t movq      %%mm4, %%mm7"
				"#5\n\t punpcklwd %%mm6, %%mm6" /* mm6 = [ coef[3]|coef[3]|coef[3]|coef[3] ] */

				"#6\n\t punpcklwd %%mm7, %%mm7" /* mm6 = [ coef[1]|coef[1]|coef[1]|coef[1] ] */
				"#5\n\t pmullw    %%mm6, %%mm3" /* mm3 = [ coef[3] * col[3] unpacked ] */

				"#5\n\t punpckhwd %%mm5, %%mm5" /* mm5 = [ coef[2]|coef[2]|coef[2]|coef[2] ] */
				"#6\n\t pmullw    %%mm7, %%mm1" /* mm1 = [ coef[1] * col[1] unpacked ] */

				"#5\n\t pmullw    %%mm5, %%mm2" /* mm2 = [ coef[2] * col[2] unpacked ] */
				"#6\n\t punpckhwd %%mm4, %%mm4" /* mm4 = [ coef[0]|coef[0]|coef[0]|coef[0] ] */

				"#6\n\t pmullw    %%mm4, %%mm0" /* mm0 = [ coef[0] * col[0] unpacked ] */
				"#7\n\t paddw     %%mm2, %%mm3"
				"#7\n\t paddw     %%mm1, %%mm0"

				"#7\n\t paddw     %%mm3, %%mm0"
				"#7\n\t psrlw     $8,    %%mm0"

				/* Unpacking the resulting pixel */
				"\n\t packuswb  %%mm7, %%mm0"
				"\n\t movd    %%mm0, %[output]"

				: [output]  "=m"(*dest_pixel)
				: [pixel_u] "m"(src_pixel_rowu[u>>16])
				, [pixel_l] "m"(src_pixel_rowl[u>>16])
				, [fracu]   "g"(fracU)
				, [fracv]   "g"(fracV)
				: "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7");
			
			++dest_pixel;
		}

		memset (dest_pixel, 0, (dest->pitch - ((dest->width - 1) * 4)));
		dest_pixel += (dest->pitch / 4) - ((dest->width - 1));

	}

	__asm__ __volatile__ ("\n\temms");

	return VISUAL_OK;
#else /* !VISUAL_ARCH_X86 */
	return VISUAL_ERROR_CPU_INVALID_CODE;
#endif
}

