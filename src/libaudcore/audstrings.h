/*
 * audstrings.h
 * Copyright 2009-2012 John Lindgren and William Pitcock
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

#ifndef LIBAUDCORE_STRINGS_H
#define LIBAUDCORE_STRINGS_H

#include <stdarg.h>
#include <stdint.h>

#include <initializer_list>

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

int strcmp_safe (const char * a, const char * b, int len = -1);
int strcmp_nocase (const char * a, const char * b, int len = -1);
int strlen_bounded (const char * s, int len = -1);

StringBuf str_copy (const char * s, int len = -1);
StringBuf str_concat (const std::initializer_list<const char *> & strings);
#ifdef _WIN32
StringBuf str_printf (const char * format, ...) __attribute__ ((__format__ (gnu_printf, 1, 2)));
void str_append_printf (StringBuf & str, const char * format, ...) __attribute__ ((__format__ (gnu_printf, 2, 3)));
#else
StringBuf str_printf (const char * format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void str_append_printf (StringBuf & str, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#endif
StringBuf str_vprintf (const char * format, va_list args);
void str_append_vprintf (StringBuf & str, const char * format, va_list args);

bool str_has_prefix_nocase (const char * str, const char * prefix);
bool str_has_suffix_nocase (const char * str, const char * suffix);

unsigned str_calc_hash (const char * str);

const char * strstr_nocase (const char * haystack, const char * needle);
const char * strstr_nocase_utf8 (const char * haystack, const char * needle);

static inline char * strstr_nocase (char * haystack, const char * needle)
    { return (char *) strstr_nocase ((const char *) haystack, needle); }
static inline char * strstr_nocase_utf8 (char * haystack, const char * needle)
    { return (char *) strstr_nocase_utf8 ((const char *) haystack, needle); }

StringBuf str_tolower (const char * str);
StringBuf str_tolower_utf8 (const char * str);
StringBuf str_toupper (const char * str);
StringBuf str_toupper_utf8 (const char * str);

void str_replace_char (char * string, char old_c, char new_c);

StringBuf str_decode_percent (const char * str, int len = -1);
StringBuf str_encode_percent (const char * str, int len = -1);

StringBuf str_convert (const char * str, int len, const char * from_charset, const char * to_charset);
StringBuf str_from_locale (const char * str, int len = -1);
StringBuf str_to_locale (const char * str, int len = -1);

/* Requires: aud_init() */
StringBuf str_to_utf8 (const char * str, int len); // no "len = -1" to avoid ambiguity
StringBuf str_to_utf8 (StringBuf && str);

StringBuf filename_normalize (StringBuf && filename);
StringBuf filename_contract (StringBuf && filename);
StringBuf filename_expand (StringBuf && filename);
StringBuf filename_get_parent (const char * filename);
StringBuf filename_get_base (const char * filename);
StringBuf filename_build (const std::initializer_list<const char *> & elems);
StringBuf filename_to_uri (const char * filename);
StringBuf uri_to_filename (const char * uri, bool use_locale = true);
StringBuf uri_to_display (const char * uri);

void uri_parse (const char * uri, const char * * base_p, const char * * ext_p,
 const char * * sub_p, int * isub_p);

StringBuf uri_get_scheme (const char * uri);
StringBuf uri_get_extension (const char * uri);

/* Requires: aud_init() */
StringBuf uri_construct (const char * path, const char * reference);
StringBuf uri_deconstruct (const char * uri, const char * reference);

int str_compare (const char * a, const char * b);
int str_compare_encoded (const char * a, const char * b);

Index<String> str_list_to_index (const char * list, const char * delims);
StringBuf index_to_str_list (const Index<String> & index, const char * sep);

int str_to_int (const char * string);
double str_to_double (const char * string);
void str_insert_int (StringBuf & string, int pos, int val);
void str_insert_double (StringBuf & string, int pos, double val);
StringBuf int_to_str (int val);
StringBuf double_to_str (double val);

bool str_to_int_array (const char * string, int * array, int count);
StringBuf int_array_to_str (const int * array, int count);
bool str_to_double_array (const char * string, double * array, int count);
StringBuf double_array_to_str (const double * array, int count);

/* Requires: aud_init() */
StringBuf str_format_time (int64_t milliseconds);

#endif /* LIBAUDCORE_STRINGS_H */
