/*
 *  Copyright (C) 2001-2003  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "audacious/plugin.h"
#include <glib.h>

#ifdef WORDS_BIGENDIAN
# define IS_BIG_ENDIAN TRUE
#else
# define IS_BIG_ENDIAN FALSE
#endif



static int convert_swap_endian(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr);

	return i;
}

static int convert_swap_sign_and_endian_to_native(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr) ^ 1 << 15;

	return i;
}

static int convert_swap_sign_and_endian_to_alien(void **data, int length)
{
	guint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr = GUINT16_SWAP_LE_BE(*ptr ^ 1 << 15);

	return i;
}

static int convert_swap_sign16(void **data, int length)
{
	gint16 *ptr = *data;
	int i;
	for (i = 0; i < length; i += 2, ptr++)
		*ptr ^= 1 << 15;

	return i;
}

static int convert_swap_sign8(void **data, int length)
{
	gint8 *ptr = *data;
	int i;
	for (i = 0; i < length; i++)
		*ptr++ ^= 1 << 7;

	return i;
}

int (*arts_get_convert_func(int input))(void **, int)
{
	if (input == FMT_S16_NE)
		input = IS_BIG_ENDIAN ? FMT_S16_BE : FMT_S16_LE;
	else if (input == FMT_U16_NE)
		input = IS_BIG_ENDIAN ? FMT_U16_BE : FMT_U16_LE;

	if (input == FMT_S16_LE || input == FMT_U8)
		return NULL;
	
	if (input == FMT_S16_BE)
		return convert_swap_endian;

	if (input == FMT_U16_LE)
		return convert_swap_sign16;

	if (!IS_BIG_ENDIAN && input == FMT_U16_BE)
		return convert_swap_sign_and_endian_to_native;
		
	if (IS_BIG_ENDIAN && input == FMT_U16_BE)
		return convert_swap_sign_and_endian_to_alien;

	if (input == FMT_S8)
		return convert_swap_sign8;

	g_warning("Translation needed, but not available.\n"
		  "Input: %d.", input);
	return NULL;
}
