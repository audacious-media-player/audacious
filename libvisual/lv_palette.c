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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lv_common.h"
#include "lv_palette.h"

static int palette_dtor (VisObject *object);

static int palette_dtor (VisObject *object)
{
	VisPalette *pal = VISUAL_PALETTE (object);

	if (pal->colors != NULL)
		visual_palette_free_colors (pal);

	pal->colors = NULL;

	return VISUAL_OK;
}

/**
 * @defgroup VisPalette VisPalette
 * @{
 */

/**
 * Creates a new VisPalette.
 *
 * @return A newly allocated VisPalette.
 */
VisPalette *visual_palette_new (int ncolors)
{
	VisPalette *pal;

	pal = visual_mem_new0 (VisPalette, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (pal), TRUE, palette_dtor);

	visual_palette_allocate_colors (pal, ncolors);

	return pal;
}

/**
 * Copies the colors from one VisPalette to another.
 *
 * @param dest Pointer to the destination VisPalette.
 * @param src Pointer to the source VisPalette from which colors are copied into the destination VisPalette.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PALETTE_NULL or -VISUAL_ERROR_PALETTE_SIZE on failure.
 */
int visual_palette_copy (VisPalette *dest, VisPalette *src)
{
	visual_log_return_val_if_fail (dest != NULL, -VISUAL_ERROR_PALETTE_NULL);
	visual_log_return_val_if_fail (src != NULL, -VISUAL_ERROR_PALETTE_NULL);
	visual_log_return_val_if_fail (dest->ncolors == src->ncolors, -VISUAL_ERROR_PALETTE_SIZE);

	visual_mem_copy (dest->colors, src->colors, sizeof (VisColor) * dest->ncolors);

	return VISUAL_OK;
}

/**
 * Allocate an amount of colors for a VisPalette.
 *
 * @param pal Pointer to the VisPalette for which colors are allocated.
 * @param ncolors The number of colors allocated for the VisPalette.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PALETTE_NULL on failure.
 */
int visual_palette_allocate_colors (VisPalette *pal, int ncolors)
{
	visual_log_return_val_if_fail (pal != NULL, -VISUAL_ERROR_PALETTE_NULL);

	pal->colors = visual_mem_new0 (VisColor, ncolors);
	pal->ncolors = ncolors;

	return VISUAL_OK;
}

/**
 * Frees allocated colors from a VisPalette.
 * 
 * @param pal Pointer to the VisPalette from which colors need to be freed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PALETTE_NULL on failure.
 */
int visual_palette_free_colors (VisPalette *pal)
{
	visual_log_return_val_if_fail (pal != NULL, -VISUAL_ERROR_PALETTE_NULL);

	if (pal->colors != NULL)
		visual_mem_free (pal->colors);
	
	pal->colors = NULL;
	pal->ncolors = 0;

	return VISUAL_OK;
}

/**
 * This function is capable of morphing from one palette to another.
 *
 * @param dest Pointer to the destination VisPalette, this is where the result of the morph
 * 	  is put.
 * @param src1 Pointer to a VisPalette that acts as the first source for the morph.
 * @param src2 Pointer to a VisPalette that acts as the second source for the morph.
 * @param rate Value that sets the rate of the morph, which is valid between 0 and 1.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PALETTE_NULL or -VISUAL_ERROR_PALETTE_SIZE on failure.
 */
int visual_palette_blend (VisPalette *dest, VisPalette *src1, VisPalette *src2, float rate)
{
	int i;

	visual_log_return_val_if_fail (dest != NULL, -VISUAL_ERROR_PALETTE_NULL);
	visual_log_return_val_if_fail (src1 != NULL, -VISUAL_ERROR_PALETTE_NULL);
	visual_log_return_val_if_fail (src2 != NULL, -VISUAL_ERROR_PALETTE_NULL);
	
	if (src1->ncolors != src2->ncolors)
		return -VISUAL_ERROR_PALETTE_SIZE;

	if (dest->ncolors != src1->ncolors)
		return -VISUAL_ERROR_PALETTE_SIZE;

	for (i = 0; i < dest->ncolors; i++) {
		dest->colors[i].r = src1->colors[i].r + ((src2->colors[i].r - src1->colors[i].r) * rate);
		dest->colors[i].g = src1->colors[i].g + ((src2->colors[i].g - src1->colors[i].g) * rate);
		dest->colors[i].b = src1->colors[i].b + ((src2->colors[i].b - src1->colors[i].b) * rate);
	}

	return VISUAL_OK;
}

/**
 * Can be used to cycle through the colors of a VisPalette and blend between elements. The rate is from 0.0 to number of
 * VisColors in the VisPalette. The VisColor is newly allocated so you have to unref it. The last VisColor in the VisPalette is
 * morphed with the first.
 *
 * @param pal Pointer to the VisPalette in which the VisColors are cycled.
 * @param rate Selection of the VisColor from the VisPalette, goes from 0.0 to number of VisColors in the VisPalette
 * 	and morphs between colors if needed.
 *
 * @return A new VisColor, possibly a morph between two VisColors, NULL on failure.
 */
VisColor *visual_palette_color_cycle (VisPalette *pal, float rate)
{
	VisColor *color, *tmp1, *tmp2;
	int irate = (int) rate;
	unsigned char alpha;
	float rdiff = rate - irate;

	visual_log_return_val_if_fail (pal != NULL, NULL);
	
	irate = irate % pal->ncolors;
	alpha = rdiff * 255;

	color = visual_color_new ();

	/* If rate is exactly an item, return that item */
	if (rdiff == 0) {
		visual_color_copy (color, &pal->colors[irate]);

		return color;
	}

	tmp1 = &pal->colors[irate];

	if (irate == pal->ncolors - 1)
		tmp2 = &pal->colors[0];
	else
		tmp2 = &pal->colors[irate + 1];

	color->r = ((alpha * (tmp1->r - tmp2->r) >> 8) + tmp2->r);
	color->g = ((alpha * (tmp1->g - tmp2->g) >> 8) + tmp2->g);
	color->b = ((alpha * (tmp1->b - tmp2->b) >> 8) + tmp2->b);

	return color;
}

/**
 * @}
 */

