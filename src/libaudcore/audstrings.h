/*  Audacious
 *  Copyright (C) 2005-2011  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef LIBAUDCORE_STRINGS_H
#define LIBAUDCORE_STRINGS_H

#include <libaudcore/types.h>

boolean str_has_prefix_nocase(const char * str, const char * prefix);
boolean str_has_suffix_nocase(const char * str, const char * suffix);

void str_set_utf8_impl (char * (* stu_impl) (const char *),
 char * (* stuf_impl) (const char *, int, int *, int *));
char * str_to_utf8 (const char * str);
char * str_to_utf8_full (const char * str, int len, int * bytes_read, int * bytes_written);

char *filename_get_subtune(const char * filename, int * track);
char *filename_split_subtune(const char * filename, int * track);

void string_replace_char (char * string, char old_str, char new_str);
void string_decode_percent (char * string);
char * string_encode_percent (const char * string, boolean is_filename);

boolean uri_is_utf8 (const char * uri, boolean warn);
char * uri_to_utf8 (const char * uri);
void uri_check_utf8 (char * * uri, boolean warn);
char * filename_to_uri (const char * filename);
char * uri_to_filename (const char * uri);
char * uri_to_display (const char * uri);

char * uri_get_extension (const char * uri);

void string_cut_extension(char *string);
int string_compare (const char * a, const char * b);
int string_compare_encoded (const char * a, const char * b);

char *str_replace_fragment(char *s, int size, const char *old_str, const char *new_str);

boolean string_to_int (const char * string, int * addr);
boolean string_to_double (const char * string, double * addr);
char * int_to_string (int val);
char * double_to_string (double val);

boolean string_to_double_array (const char * string, double * array, int count);
char * double_array_to_string (const double * array, int count);

#endif /* LIBAUDCORE_STRINGS_H */
