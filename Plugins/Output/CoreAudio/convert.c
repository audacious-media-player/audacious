/*
 *  Copyright (C) 2001  Haavard Kvaalen
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "coreaudio.h"

void* osx_get_convert_buffer(size_t size)
{
	static size_t length;
	static void *buffer;

	if (size > 0 && size <= length)
		return buffer;

	length = size;
	buffer = g_realloc(buffer, size);
	return buffer;
}

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

static int convert_to_8_native_endian(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = *input++ >> 8;

	return i;
}

static int convert_to_8_native_endian_swap_sign(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = (*input++ >> 8) ^ (1 << 7);

	return i;
}


static int convert_to_8_alien_endian(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = *input++ & 0xff;

	return i;
}

static int convert_to_8_alien_endian_swap_sign(void **data, int length)
{
	gint8 *output = *data;
	gint16 *input = *data;
	int i;
	for (i = 0; i < length / 2; i++)
		*output++ = (*input++ & 0xff) ^ (1 << 7);

	return i;
}

static int convert_to_16_native_endian(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = osx_get_convert_buffer(length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++ << 8;

	return i * 2;
}

static int convert_to_16_native_endian_swap_sign(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = osx_get_convert_buffer(length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = (*input++ << 8) ^ (1 << 15);

	return i * 2;
}


static int convert_to_16_alien_endian(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = osx_get_convert_buffer(length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++;

	return i * 2;
}

static int convert_to_16_alien_endian_swap_sign(void **data, int length)
{
	guint8 *input = *data;
	guint16 *output;
	int i;
	*data = osx_get_convert_buffer(length * 2);
	output = *data;
	for (i = 0; i < length; i++)
		*output++ = *input++ ^ (1 << 7);

	return i * 2;
}

int (*osx_get_convert_func(int output, int input))(void **, int)
{
	if (output == input)
		return NULL;

	if ((output == FMT_U16_BE && input == FMT_U16_LE) ||
	    (output == FMT_U16_LE && input == FMT_U16_BE) ||
	    (output == FMT_S16_BE && input == FMT_S16_LE) ||
	    (output == FMT_S16_LE && input == FMT_S16_BE))
		return convert_swap_endian;

	if ((output == FMT_U16_BE && input == FMT_S16_BE) ||
	    (output == FMT_U16_LE && input == FMT_S16_LE) ||
	    (output == FMT_S16_BE && input == FMT_U16_BE) ||
	    (output == FMT_S16_LE && input == FMT_U16_LE))
		return convert_swap_sign16;

	if ((IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_S16_LE) ||
	      (output == FMT_S16_BE && input == FMT_U16_LE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_S16_BE) ||
	      (output == FMT_S16_LE && input == FMT_U16_BE))))
		return convert_swap_sign_and_endian_to_native;

	if ((!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_S16_LE) ||
	      (output == FMT_S16_BE && input == FMT_U16_LE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_S16_BE) ||
	      (output == FMT_S16_LE && input == FMT_U16_BE))))
		return convert_swap_sign_and_endian_to_alien;

	if ((IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_U16_BE) ||
	      (output == FMT_S8 && input == FMT_S16_BE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_U16_LE) ||
	      (output == FMT_S8 && input == FMT_S16_LE))))
		return convert_to_8_native_endian;

	if ((IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_S16_BE) ||
	      (output == FMT_S8 && input == FMT_U16_BE))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_S16_LE) ||
	      (output == FMT_S8 && input == FMT_U16_LE))))
		return convert_to_8_native_endian_swap_sign;

	if ((!IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_U16_BE) ||
	      (output == FMT_S8 && input == FMT_S16_BE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_U16_LE) ||
	      (output == FMT_S8 && input == FMT_S16_LE))))
		return convert_to_8_alien_endian;

	if ((!IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_S16_BE) ||
	      (output == FMT_S8 && input == FMT_U16_BE))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == FMT_U8 && input == FMT_S16_LE) ||
	      (output == FMT_S8 && input == FMT_U16_LE))))
		return convert_to_8_alien_endian_swap_sign;

	if ((output == FMT_U8 && input == FMT_S8) ||
	    (output == FMT_S8 && input == FMT_U8))
		return convert_swap_sign8;

	if ((IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_U8) ||
	      (output == FMT_S16_BE && input == FMT_S8))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_U8) ||
	      (output == FMT_S16_LE && input == FMT_S8))))
		return convert_to_16_native_endian;

	if ((IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_S8) ||
	      (output == FMT_S16_BE && input == FMT_U8))) ||
	    (!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_S8) ||
	      (output == FMT_S16_LE && input == FMT_U8))))
		return convert_to_16_native_endian_swap_sign;

	if ((!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_U8) ||
	      (output == FMT_S16_BE && input == FMT_S8))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_U8) ||
	      (output == FMT_S16_LE && input == FMT_S8))))
		return convert_to_16_alien_endian;

	if ((!IS_BIG_ENDIAN &&
	     ((output == FMT_U16_BE && input == FMT_S8) ||
	      (output == FMT_S16_BE && input == FMT_U8))) ||
	    (IS_BIG_ENDIAN &&
	     ((output == FMT_U16_LE && input == FMT_S8) ||
	      (output == FMT_S16_LE && input == FMT_U8))))
		return convert_to_16_alien_endian_swap_sign;

	g_warning("Translation needed, but not available.\n"
			  "Input: %d; Output %d.", input, output);
	return NULL;
}
