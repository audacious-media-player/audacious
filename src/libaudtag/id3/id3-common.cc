/*
 * id3-common.c
 * Copyright 2010-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include "id3-common.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/runtime.h>

#include "../util.h"

#define ID3_ENCODING_LATIN1    0
#define ID3_ENCODING_UTF16     1
#define ID3_ENCODING_UTF16_BE  2
#define ID3_ENCODING_UTF8      3

static void * memchr16 (const void * mem, int16_t chr, int len)
{
    while (len >= 2)
    {
        if (* (int16_t *) mem == chr)
            return (void *) mem;

        mem = (char *) mem + 2;
        len -= 2;
    }

    return nullptr;
}

static void id3_strnlen (const char * data, int size, int encoding,
 int * bytes_without_nul, int * bytes_with_nul)
{
    bool is16 = (encoding == ID3_ENCODING_UTF16 || encoding == ID3_ENCODING_UTF16_BE);
    char * nul = is16 ? (char *) memchr16 (data, 0, size) : (char *) memchr (data, 0, size);

    if (nul)
    {
        if (bytes_without_nul)
            * bytes_without_nul = nul - data;
        if (bytes_with_nul)
            * bytes_with_nul = is16 ? nul + 2 - data : nul + 1 - data;
    }
    else
    {
        if (bytes_without_nul)
            * bytes_without_nul = size;
        if (bytes_with_nul)
            * bytes_with_nul = size;
    }
}

static StringBuf id3_convert (const char * data, int size, int encoding)
{
    if (encoding == ID3_ENCODING_UTF16)
        return str_convert (data, size, "UTF-16", "UTF-8");
    else if (encoding == ID3_ENCODING_UTF16_BE)
        return str_convert (data, size, "UTF-16BE", "UTF-8");
    else
        return str_to_utf8 (data, size);
}

static StringBuf id3_decode_text (const char * data, int size)
{
    if (size < 1)
        return StringBuf ();

    return id3_convert ((const char *) data + 1, size - 1, data[0]);
}

void id3_associate_string (Tuple & tuple, int field, const char * data, int size)
{
    StringBuf text = id3_decode_text (data, size);

    if (text && text[0])
    {
        AUDDBG ("Field %i = %s.\n", field, (const char *) text);
        tuple.set_str (field, text);
    }
}

void id3_associate_int (Tuple & tuple, int field, const char * data, int size)
{
    StringBuf text = id3_decode_text (data, size);

    /* Ignore zeros here.  In particular, there are many ID3 tags with invalid
     * TLEN fields floating around, and we want to let mpg123 recalculate the
     * length in such cases. */
    if (text && atoi (text) > 0)
    {
        AUDDBG ("Field %i = %s.\n", field, (const char *) text);
        tuple.set_int (field, atoi (text));
    }
}

void id3_decode_genre (Tuple & tuple, const char * data, int size)
{
    StringBuf text = id3_decode_text (data, size);
    int numericgenre;

    if (! text)
        return;

    if (text[0] == '(')
        numericgenre = atoi (text + 1);
    else
        numericgenre = atoi (text);

    if (numericgenre > 0)
        tuple.set_str (FIELD_GENRE, convert_numericgenre_to_text (numericgenre));
    else
        tuple.set_str (FIELD_GENRE, text);
}

void id3_decode_comment (Tuple & tuple, const char * data, int size)
{
    if (size < 4)
        return;

    int before_nul, after_nul;
    id3_strnlen (data + 4, size - 4, data[0], & before_nul, & after_nul);

    const char * lang = data + 1;
    StringBuf type = id3_convert (data + 4, before_nul, data[0]);
    StringBuf value = id3_convert (data + 4 + after_nul, size - 4 - after_nul, data[0]);

    AUDDBG ("Comment: lang = %.3s, type = %s, value = %s.\n", lang,
     (const char *) type, (const char *) value);

    if (type && ! type[0] && value) /* blank type = actual comment */
        tuple.set_str (FIELD_COMMENT, value);
}

static bool decode_rva_block (const char * * _data, int * _size,
 int * channel, int * adjustment, int * adjustment_unit, int * peak,
 int * peak_unit)
{
    const char * data = * _data;
    int size = * _size;
    int peak_bits;

    if (size < 4)
        return false;

    * channel = data[0];
    * adjustment = (char) data[1]; /* first byte is signed */
    * adjustment = (* adjustment << 8) | data[2];
    * adjustment_unit = 512;
    peak_bits = data[3];

    data += 4;
    size -= 4;

    AUDDBG ("RVA block: channel = %d, adjustment = %d/%d, peak bits = %d\n",
     * channel, * adjustment, * adjustment_unit, peak_bits);

    if (peak_bits > 0 && peak_bits < (int) sizeof (int) * 8)
    {
        int bytes = (peak_bits + 7) / 8;
        int count;

        if (bytes > size)
            return false;

        * peak = 0;
        * peak_unit = 1 << peak_bits;

        for (count = 0; count < bytes; count ++)
            * peak = (* peak << 8) | data[count];

        data += bytes;
        size -= count;

        AUDDBG ("RVA block: peak = %d/%d\n", * peak, * peak_unit);
    }
    else
    {
        * peak = 0;
        * peak_unit = 0;
    }

    * _data = data;
    * _size = size;
    return true;
}

void id3_decode_rva (Tuple & tuple, const char * data, int size)
{
    const char * domain;
    int channel, adjustment, adjustment_unit, peak, peak_unit;

    if (memchr (data, 0, size) == nullptr)
        return;

    domain = data;

    AUDDBG ("RVA domain: %s\n", domain);

    size -= strlen (domain) + 1;
    data += strlen (domain) + 1;

    while (size > 0)
    {
        if (! decode_rva_block (& data, & size, & channel, & adjustment,
         & adjustment_unit, & peak, & peak_unit))
            break;

        if (channel != 1) /* specific channel? */
            continue;

        if (tuple.get_value_type (FIELD_GAIN_GAIN_UNIT) == TUPLE_INT)
            adjustment = adjustment * (int64_t) tuple.get_int
             (FIELD_GAIN_GAIN_UNIT) / adjustment_unit;
        else
            tuple.set_int (FIELD_GAIN_GAIN_UNIT, adjustment_unit);

        if (peak_unit)
        {
            if (tuple.get_value_type (FIELD_GAIN_PEAK_UNIT) == TUPLE_INT)
                peak = peak * (int64_t) tuple.get_int (FIELD_GAIN_PEAK_UNIT) / peak_unit;
            else
                tuple.set_int (FIELD_GAIN_PEAK_UNIT, peak_unit);
        }

        if (! g_ascii_strcasecmp (domain, "album"))
        {
            tuple.set_int (FIELD_GAIN_ALBUM_GAIN, adjustment);

            if (peak_unit)
                tuple.set_int (FIELD_GAIN_ALBUM_PEAK, peak);
        }
        else if (! g_ascii_strcasecmp (domain, "track"))
        {
            tuple.set_int (FIELD_GAIN_TRACK_GAIN, adjustment);

            if (peak_unit)
                tuple.set_int (FIELD_GAIN_TRACK_PEAK, peak);
        }
    }
}

bool id3_decode_picture (const char * data, int size, int * type,
 void * * image_data, int64_t * image_size)
{
    const char * nul;
    if (size < 2 || ! (nul = (char *) memchr (data + 1, 0, size - 2)))
        return false;

    const char * body = nul + 2;
    int body_size = data + size - body;

    int before_nul2, after_nul2;
    id3_strnlen (body, body_size, data[0], & before_nul2, & after_nul2);

    const char * mime = data + 1;
    StringBuf desc = id3_convert (body, before_nul2, data[0]);

    * type = nul[1];
    * image_size = body_size - after_nul2;
    * image_data = g_memdup (body + after_nul2, * image_size);

    AUDDBG ("Picture: mime = %s, type = %d, desc = %s, size = %d.\n", mime,
     * type, (const char *) desc, (int) * image_size);

    return true;
}
