/*
 * id3-common.c
 * Copyright 2010 John Lindgren
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

#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "../util.h"
#include "id3-common.h"

gchar * convert_text (const gchar * text, gint length, gint encoding, gboolean
 nulled, gint * _converted, const gchar * * after)
{
    gchar * buffer = NULL;
    gsize converted = 0;

    AUDDBG ("length = %d, encoding = %d, nulled = %d\n", length, encoding,
     nulled);

    if (nulled)
    {
        const guchar null16[] = {0, 0};
        const gchar * null;

        switch (encoding)
        {
          case 0:
          case 3:
            if ((null = memchr (text, 0, length)) == NULL)
                return NULL;

            length = null - text;
            AUDDBG ("length before null = %d\n", length);

            if (after != NULL)
                * after = null + 1;

            break;
          case 1:
          case 2:
            if ((null = memfind (text, length, null16, 2)) == NULL)
                return NULL;

            length = null - text;
            AUDDBG ("length before null = %d\n", length);

            if (after != NULL)
                * after = null + 2;

            break;
        }
    }

    switch (encoding)
    {
      case 0:
      case 3:
        buffer = chardet_to_utf8 (text, length, NULL, & converted, NULL);
        break;
      case 1:
        if (text[0] == (gchar) 0xff)
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

    AUDDBG ("length converted: %d\n", (gint) converted);
    AUDDBG ("string: %s\n", buffer);

    if (_converted != NULL)
        * _converted = converted;

    return buffer;
}
