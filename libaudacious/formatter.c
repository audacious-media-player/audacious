/*  Audacious
 *  Copyright (C) 2005-2007  Audacious team
 *
 *  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include "formatter.h"

/**
 * xmms_formatter_new:
 *
 * Factory for #Formatter objects.
 **/
Formatter *
xmms_formatter_new(void)
{
    Formatter *formatter = g_new0(Formatter, 1);

    xmms_formatter_associate(formatter, '%', "%");
    return formatter;
}

/**
 * xmms_formatter_destroy:
 * @formatter: A #Formatter object to destroy.
 *
 * Destroys #Formatter objects.
 **/
void
xmms_formatter_destroy(Formatter * formatter)
{
    int i;

    for (i = 0; i < 256; i++)
        if (formatter->values[i])
            g_free(formatter->values[i]);
    g_free(formatter);
}

/**
 * xmms_formatter_associate:
 * @formatter: A #Formatter object to use.
 * @id: The character to use for replacement.
 * @value: The value to replace with.
 *
 * Adds a id->replacement set to the formatter's stack.
 **/
void
xmms_formatter_associate(Formatter * formatter, guchar id, char *value)
{
    xmms_formatter_dissociate(formatter, id);
    formatter->values[id] = g_strdup(value);
}

/**
 * xmms_formatter_dissociate:
 * @formatter: A #Formatter object to use.
 * @id: The id to remove the id->replacement mapping for.
 *
 * Removes an id->replacement mapping from the formatter's stack.
 **/
void
xmms_formatter_dissociate(Formatter * formatter, guchar id)
{
    if (formatter->values[id])
        g_free(formatter->values[id]);
    formatter->values[id] = 0;
}

/**
 * xmms_formatter_format:
 * @formatter: A #Formatter object to use.
 * @format: A string to format.
 *
 * Performs id->replacement substitution on a string.
 *
 * Returns: The formatted string.
 **/
gchar *
xmms_formatter_format(Formatter * formatter, char *format)
{
    char *p, *q, *buffer;
    int len;

    for (p = format, len = 0; *p; p++)
        if (*p == '%') {
            if (formatter->values[(int) *++p])
                len += strlen(formatter->values[(int) *p]);
            else if (!*p) {
                len += 1;
                p--;
            }
            else
                len += 2;
        }
        else
            len++;
    buffer = g_malloc(len + 1);
    for (p = format, q = buffer; *p; p++)
        if (*p == '%') {
            if (formatter->values[(int) *++p]) {
                strcpy(q, formatter->values[(int) *p]);
                q += strlen(q);
            }
            else {
                *q++ = '%';
                if (*p != '\0')
                    *q++ = *p;
                else
                    p--;
            }
        }
        else
            *q++ = *p;
    *q = 0;
    return buffer;
}
