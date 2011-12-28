/*
 * id3-common.c
 * Copyright 2010-2011 John Lindgren
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

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "../util.h"
#include "id3-common.h"

static void * memchr16 (const void * mem, int16_t chr, int len)
{
    while (len >= 2)
    {
        if (* (int16_t *) mem == chr)
            return (void *) mem;

        mem = (char *) mem + 2;
        len -= 2;
    }

    return NULL;
}

char * convert_text (const char * text, int length, int encoding, bool_t
 nulled, int * _converted, const char * * after)
{
    char * buffer = NULL;
    gsize converted = 0;

    TAGDBG ("length = %d, encoding = %d, nulled = %d\n", length, encoding,
     nulled);

    if (nulled)
    {
        const char * null;

        switch (encoding)
        {
          case 0:
          case 3:
            if ((null = memchr (text, 0, length)) == NULL)
                return NULL;

            length = null - text;
            TAGDBG ("length before null = %d\n", length);

            if (after != NULL)
                * after = null + 1;

            break;
          case 1:
          case 2:
            if ((null = memchr16 (text, 0, length)) == NULL)
                return NULL;

            length = null - text;
            TAGDBG ("length before null = %d\n", length);

            if (after != NULL)
                * after = null + 2;

            break;
        }
    }

    switch (encoding)
    {
      case 0:
      case 3:;
        int converted_int = 0;
        buffer = str_to_utf8_full (text, length, NULL, & converted_int);
        converted = converted_int;
        break;
      case 1:
        if (text[0] == (char) 0xff)
            buffer = g_convert (text + 2, length - 2, "UTF-8", "UTF-16LE", NULL,
             & converted, NULL);
        else
            buffer = g_convert (text + 2, length - 2, "UTF-8", "UTF-16BE", NULL,
             & converted, NULL);

        break;
      case 2:
        buffer = g_convert (text, length, "UTF-8", "UTF-16BE", NULL,
         & converted, NULL);
        break;
    }

    TAGDBG ("length converted: %d\n", (int) converted);
    TAGDBG ("string: %s\n", buffer);

    if (_converted != NULL)
        * _converted = converted;

    return buffer;
}
