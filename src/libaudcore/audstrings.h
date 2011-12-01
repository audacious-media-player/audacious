/*  Audacious
 *  Copyright (C) 2005-2007  Audacious development team.
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

char *str_append(char * str, const char * add_str);
char *str_replace(char * str, char * new_str);
void str_replace_in(char ** str, char * new_str);

boolean str_has_prefix_nocase(const char * str, const char * prefix);
boolean str_has_suffix_nocase(const char * str, const char * suffix);
boolean str_has_suffixes_nocase(const char * str, char * const *suffixes);

char *str_assert_utf8(const char *str);

void str_set_utf8_impl (char * (* stu_impl) (const char *),
 char * (* stuf_impl) (const char *, int, int *, int *));
char * str_to_utf8 (const char * str);
char * str_to_utf8_full (const char * str, int len, int * bytes_read, int * bytes_written);

const char *str_skip_chars(const char * str, const char * chars);

char *convert_dos_path(char * text);

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

const void * memfind (const void * mem, int size, const void * token, int
 length);

char *str_replace_fragment(char *s, int size, const char *old_str, const char *new_str);

void string_canonize_case(char *string);

boolean string_to_int (const char * string, int * addr);
boolean string_to_double (const char * string, double * addr);
char * int_to_string (int val);
char * double_to_string (double val);

boolean string_to_double_array (const char * string, double * array, int count);
char * double_array_to_string (const double * array, int count);

#endif /* LIBAUDCORE_STRINGS_H */
