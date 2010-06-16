/*
 * id3-common.h
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

#ifndef AUDTAG_ID3_COMMON_H
#define AUDTAG_ID3_COMMON_H

#include <glib.h>

/*
 * text: A pointer to the text data to be converted.
 * length: The size in bytes of the text data.
 * encoding: 0 = ISO-8859-1 or a similar 8-bit character set, 1 = UTF-16 with
 *  BOM, 2 = UTF-16BE without BOM, 3 = UTF-8.
 * nulled: Whether the text data is null-terminated.  If this flag is set and no
 *  null character is found, an error occurs.  If this flag is not set, any null
 *  characters found are included in the converted text.
 * converted: A location in which to store the number of bytes converted, or
 *  NULL.  This count does not include the null character appended to the
 *  converted text, but may include null characters present in the text data.
 *  This count is undefined if an error occurs.
 * after: A location in which to store a pointer to the next character after a
 *  terminating null character, or NULL.  This pointer is undefined if the
 *  "nulled" flag is not used or if an error occurs.
 * returns: A pointer to the converted text with a null character appended, or
 *  NULL if an error occurs.  The text must be freed when no longer needed.
 */
gchar * convert_text (const gchar * text, gint length, gint encoding, gboolean
 nulled, gint * _converted, const gchar * * after);

#endif
