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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "OSS.h"

struct buffer {
    void *buffer;
    int size;
} format_buffer, stereo_buffer;


static void *
oss_get_convert_buffer(struct buffer *buffer, size_t size)
{
    if (size > 0 && size <= (size_t)buffer->size)
        return buffer->buffer;

    buffer->size = size;
    buffer->buffer = g_realloc(buffer->buffer, size);
    return buffer->buffer;
}

void
oss_free_convert_buffer(void)
{
    oss_get_convert_buffer(&format_buffer, 0);
    oss_get_convert_buffer(&stereo_buffer, 0);
}


static int
convert_swap_endian(void **data, int length)
{
    guint16 *ptr = *data;
    int i;
    for (i = 0; i < length; i += 2, ptr++)
        *ptr = GUINT16_SWAP_LE_BE(*ptr);

    return i;
}

static int
convert_swap_sign_and_endian_to_native(void **data, int length)
{
    guint16 *ptr = *data;
    int i;
    for (i = 0; i < length; i += 2, ptr++)
        *ptr = GUINT16_SWAP_LE_BE(*ptr) ^ 1 << 15;

    return i;
}

static int
convert_swap_sign_and_endian_to_alien(void **data, int length)
{
    guint16 *ptr = *data;
    int i;
    for (i = 0; i < length; i += 2, ptr++)
        *ptr = GUINT16_SWAP_LE_BE(*ptr ^ 1 << 15);

    return i;
}

static int
convert_swap_sign16(void **data, int length)
{
    gint16 *ptr = *data;
    int i;
    for (i = 0; i < length; i += 2, ptr++)
        *ptr ^= 1 << 15;

    return i;
}

static int
convert_swap_sign8(void **data, int length)
{
    gint8 *ptr = *data;
    int i;
    for (i = 0; i < length; i++)
        *ptr++ ^= 1 << 7;

    return i;
}

static int
convert_to_8_native_endian(void **data, int length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    int i;
    for (i = 0; i < length / 2; i++)
        *output++ = *input++ >> 8;

    return i;
}

static int
convert_to_8_native_endian_swap_sign(void **data, int length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    int i;
    for (i = 0; i < length / 2; i++)
        *output++ = (*input++ >> 8) ^ (1 << 7);

    return i;
}


static int
convert_to_8_alien_endian(void **data, int length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    int i;
    for (i = 0; i < length / 2; i++)
        *output++ = *input++ & 0xff;

    return i;
}

static int
convert_to_8_alien_endian_swap_sign(void **data, int length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    int i;
    for (i = 0; i < length / 2; i++)
        *output++ = (*input++ & 0xff) ^ (1 << 7);

    return i;
}

static int
convert_to_16_native_endian(void **data, int length)
{
    guint8 *input = *data;
    guint16 *output;
    int i;
    *data = oss_get_convert_buffer(&format_buffer, length * 2);
    output = *data;
    for (i = 0; i < length; i++)
        *output++ = *input++ << 8;

    return i * 2;
}

static int
convert_to_16_native_endian_swap_sign(void **data, int length)
{
    guint8 *input = *data;
    guint16 *output;
    int i;
    *data = oss_get_convert_buffer(&format_buffer, length * 2);
    output = *data;
    for (i = 0; i < length; i++)
        *output++ = (*input++ << 8) ^ (1 << 15);

    return i * 2;
}


static int
convert_to_16_alien_endian(void **data, int length)
{
    guint8 *input = *data;
    guint16 *output;
    int i;
    *data = oss_get_convert_buffer(&format_buffer, length * 2);
    output = *data;
    for (i = 0; i < length; i++)
        *output++ = *input++;

    return i * 2;
}

static int
convert_to_16_alien_endian_swap_sign(void **data, int length)
{
    guint8 *input = *data;
    guint16 *output;
    int i;
    *data = oss_get_convert_buffer(&format_buffer, length * 2);
    output = *data;
    for (i = 0; i < length; i++)
        *output++ = *input++ ^ (1 << 7);

    return i * 2;
}

int (*oss_get_convert_func(int output, int input)) (void **, int) {
    if (output == input)
        return NULL;

    if ((output == AFMT_U16_BE && input == AFMT_U16_LE) ||
        (output == AFMT_U16_LE && input == AFMT_U16_BE) ||
        (output == AFMT_S16_BE && input == AFMT_S16_LE) ||
        (output == AFMT_S16_LE && input == AFMT_S16_BE))
        return convert_swap_endian;

    if ((output == AFMT_U16_BE && input == AFMT_S16_BE) ||
        (output == AFMT_U16_LE && input == AFMT_S16_LE) ||
        (output == AFMT_S16_BE && input == AFMT_U16_BE) ||
        (output == AFMT_S16_LE && input == AFMT_U16_LE))
        return convert_swap_sign16;

    if ((IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_S16_LE) ||
          (output == AFMT_S16_BE && input == AFMT_U16_LE))) ||
        (!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_S16_BE) ||
          (output == AFMT_S16_LE && input == AFMT_U16_BE))))
        return convert_swap_sign_and_endian_to_native;

    if ((!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_S16_LE) ||
          (output == AFMT_S16_BE && input == AFMT_U16_LE))) ||
        (IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_S16_BE) ||
          (output == AFMT_S16_LE && input == AFMT_U16_BE))))
        return convert_swap_sign_and_endian_to_alien;

    if ((IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_U16_BE) ||
          (output == AFMT_S8 && input == AFMT_S16_BE))) ||
        (!IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_U16_LE) ||
          (output == AFMT_S8 && input == AFMT_S16_LE))))
        return convert_to_8_native_endian;

    if ((IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_S16_BE) ||
          (output == AFMT_S8 && input == AFMT_U16_BE))) ||
        (!IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_S16_LE) ||
          (output == AFMT_S8 && input == AFMT_U16_LE))))
        return convert_to_8_native_endian_swap_sign;

    if ((!IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_U16_BE) ||
          (output == AFMT_S8 && input == AFMT_S16_BE))) ||
        (IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_U16_LE) ||
          (output == AFMT_S8 && input == AFMT_S16_LE))))
        return convert_to_8_alien_endian;

    if ((!IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_S16_BE) ||
          (output == AFMT_S8 && input == AFMT_U16_BE))) ||
        (IS_BIG_ENDIAN &&
         ((output == AFMT_U8 && input == AFMT_S16_LE) ||
          (output == AFMT_S8 && input == AFMT_U16_LE))))
        return convert_to_8_alien_endian_swap_sign;

    if ((output == AFMT_U8 && input == AFMT_S8) ||
        (output == AFMT_S8 && input == AFMT_U8))
        return convert_swap_sign8;

    if ((IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_U8) ||
          (output == AFMT_S16_BE && input == AFMT_S8))) ||
        (!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_U8) ||
          (output == AFMT_S16_LE && input == AFMT_S8))))
        return convert_to_16_native_endian;

    if ((IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_S8) ||
          (output == AFMT_S16_BE && input == AFMT_U8))) ||
        (!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_S8) ||
          (output == AFMT_S16_LE && input == AFMT_U8))))
        return convert_to_16_native_endian_swap_sign;

    if ((!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_U8) ||
          (output == AFMT_S16_BE && input == AFMT_S8))) ||
        (IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_U8) ||
          (output == AFMT_S16_LE && input == AFMT_S8))))
        return convert_to_16_alien_endian;

    if ((!IS_BIG_ENDIAN &&
         ((output == AFMT_U16_BE && input == AFMT_S8) ||
          (output == AFMT_S16_BE && input == AFMT_U8))) ||
        (IS_BIG_ENDIAN &&
         ((output == AFMT_U16_LE && input == AFMT_S8) ||
          (output == AFMT_S16_LE && input == AFMT_U8))))
        return convert_to_16_alien_endian_swap_sign;

    g_warning("Translation needed, but not available.\n"
              "Input: %d; Output %d.", input, output);
    return NULL;
}

static int
convert_mono_to_stereo(void **data, int length, int fmt)
{
    int i;
    void *outbuf = oss_get_convert_buffer(&stereo_buffer, length * 2);

    if (fmt == AFMT_U8 || fmt == AFMT_S8) {
        guint8 *output = outbuf, *input = *data;
        for (i = 0; i < length; i++) {
            *output++ = *input;
            *output++ = *input;
            input++;
        }
    }
    else {
        guint16 *output = outbuf, *input = *data;
        for (i = 0; i < length / 2; i++) {
            *output++ = *input;
            *output++ = *input;
            input++;
        }
    }
    *data = outbuf;

    return length * 2;
}

static int
convert_stereo_to_mono(void **data, int length, int fmt)
{
    int i;

    switch (fmt) {
    case AFMT_U8:
        {
            guint8 *output = *data, *input = *data;
            for (i = 0; i < length / 2; i++) {
                guint16 tmp;
                tmp = *input++;
                tmp += *input++;
                *output++ = tmp / 2;
            }
        }
        break;
    case AFMT_S8:
        {
            gint8 *output = *data, *input = *data;
            for (i = 0; i < length / 2; i++) {
                gint16 tmp;
                tmp = *input++;
                tmp += *input++;
                *output++ = tmp / 2;
            }
        }
        break;
    case AFMT_U16_LE:
        {
            guint16 *output = *data, *input = *data;
            for (i = 0; i < length / 4; i++) {
                guint32 tmp;
                guint16 stmp;
                tmp = GUINT16_FROM_LE(*input);
                input++;
                tmp += GUINT16_FROM_LE(*input);
                input++;
                stmp = tmp / 2;
                *output++ = GUINT16_TO_LE(stmp);
            }
        }
        break;
    case AFMT_U16_BE:
        {
            guint16 *output = *data, *input = *data;
            for (i = 0; i < length / 4; i++) {
                guint32 tmp;
                guint16 stmp;
                tmp = GUINT16_FROM_BE(*input);
                input++;
                tmp += GUINT16_FROM_BE(*input);
                input++;
                stmp = tmp / 2;
                *output++ = GUINT16_TO_BE(stmp);
            }
        }
        break;
    case AFMT_S16_LE:
        {
            gint16 *output = *data, *input = *data;
            for (i = 0; i < length / 4; i++) {
                gint32 tmp;
                gint16 stmp;
                tmp = GINT16_FROM_LE(*input);
                input++;
                tmp += GINT16_FROM_LE(*input);
                input++;
                stmp = tmp / 2;
                *output++ = GINT16_TO_LE(stmp);
            }
        }
        break;
    case AFMT_S16_BE:
        {
            gint16 *output = *data, *input = *data;
            for (i = 0; i < length / 4; i++) {
                gint32 tmp;
                gint16 stmp;
                tmp = GINT16_FROM_BE(*input);
                input++;
                tmp += GINT16_FROM_BE(*input);
                input++;
                stmp = tmp / 2;
                *output++ = GINT16_TO_BE(stmp);
            }
        }
        break;
    default:
        g_error("unknown format");
    }

    return length / 2;
}

int (*oss_get_stereo_convert_func(int output, int input)) (void **, int, int) {
    if (output == input)
        return NULL;

    if (input == 1 && output == 2)
        return convert_mono_to_stereo;
    if (input == 2 && output == 1)
        return convert_stereo_to_mono;

    g_warning("Input has %d channels, soundcard uses %d channels\n"
              "No conversion is available", input, output);
    return NULL;
}
