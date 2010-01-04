/*
 * audio.h
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_AUDIO_H
#define AUDACIOUS_AUDIO_H

#include <glib.h>

/* 24-bit integer samples are padded to 32-bit; high byte is always 0 */
typedef enum
{
    FMT_FLOAT,
    FMT_S8, FMT_U8,
    FMT_S16_LE, FMT_S16_BE, FMT_U16_LE, FMT_U16_BE,
    FMT_S24_LE, FMT_S24_BE, FMT_U24_LE, FMT_U24_BE,
    FMT_S32_LE, FMT_S32_BE, FMT_U32_LE, FMT_U32_BE,
}
AFormat;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define FMT_S16_NE FMT_S16_LE
#define FMT_U16_NE FMT_U16_LE
#define FMT_S24_NE FMT_S24_LE
#define FMT_U24_NE FMT_U24_LE
#define FMT_S32_NE FMT_S32_LE
#define FMT_U32_NE FMT_U32_LE
#else
#define FMT_S16_NE FMT_S16_BE
#define FMT_U16_NE FMT_U16_BE
#define FMT_S24_NE FMT_S24_BE
#define FMT_U24_NE FMT_U24_BE
#define FMT_S32_NE FMT_S32_BE
#define FMT_U32_NE FMT_U32_BE
#endif

#define FMT_SIZEOF(f) \
 (f == FMT_FLOAT ? sizeof (gfloat) : f <= FMT_U8 ? 1 : f <= FMT_U16_BE ? 2 : 4)

void audio_from_int (void * in, AFormat format, gfloat * out, gint samples);
void audio_to_int (gfloat * in, void * out, AFormat format, gint samples);
void audio_amplify (gfloat * data, gint channels, gint frames, gfloat * factors);

#endif
