/*
 * id3-common.h
 * Copyright 2010 John Lindgren
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

#ifndef AUDTAG_ID3_COMMON_H
#define AUDTAG_ID3_COMMON_H

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
char * convert_text (const char * text, int length, int encoding, bool_t
 nulled, int * _converted, const char * * after);

#endif
