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
#include <stdio.h>
#include <string.h>

#include <libaudcore/core.h>

#define SPRINTF(s, ...) \
 char s[snprintf (NULL, 0, __VA_ARGS__) + 1]; \
 snprintf (s, sizeof s, __VA_ARGS__)

#define VSPRINTF(s, f, v) \
 va_list v##2; \
 va_copy (v##2, v); \
 char s[vsnprintf (NULL, 0, f, v##2) + 1]; \
 va_end (v##2); \
 vsnprintf (s, sizeof s, f, v)

#define SCOPY(s, a) \
 char s[strlen (a) + 1]; \
 strcpy (s, a)

#define SNCOPY(s, a, x) \
 char s[(x) + 1]; \
 strncpy (s, a, sizeof s - 1); \
 s[sizeof s - 1] = 0

#define SCONCAT2(s, a, b) \
 int s##_1 = strlen (a), s##_2 = strlen (b); \
 char s[s##_1 + s##_2 + 1]; \
 memcpy (s, (a), s##_1); \
 strcpy (s + s##_1, (b))

#define SCONCAT3(s, a, b, c) \
 int s##_1 = strlen (a), s##_2 = strlen (b), s##_3 = strlen (c); \
 char s[s##_1 + s##_2 + s##_3 + 1]; \
 memcpy (s, (a), s##_1); \
 memcpy (s + s##_1, (b), s##_2); \
 strcpy (s + s##_1 + s##_2, (c))

#define SCONCAT4(s, a, b, c, d) \
 int s##_1 = strlen (a), s##_2 = strlen (b), s##_3 = strlen (c), s##_4 = strlen (d); \
 char s[s##_1 + s##_2 + s##_3 + s##_4 + 1]; \
 memcpy (s, (a), s##_1); \
 memcpy (s + s##_1, (b), s##_2); \
 memcpy (s + s##_1 + s##_2, (c), s##_3); \
 strcpy (s + s##_1 + s##_2 + s##_3, (d))

struct _Index;

/* all (char *) return values must be freed with str_unref() */

char * str_printf (const char * format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
char * str_vprintf (const char * format, va_list args);

bool_t str_has_prefix_nocase(const char * str, const char * prefix);
bool_t str_has_suffix_nocase(const char * str, const char * suffix);

char * strstr_nocase (const char * haystack, const char * needle);
char * strstr_nocase_utf8 (const char * haystack, const char * needle);

char * str_tolower_utf8 (const char * str);

void str_replace_char (char * string, char old_c, char new_c);

void str_itoa (int x, char * buf, int bufsize);

void str_decode_percent (const char * str, int len, char * out);
void str_encode_percent (const char * str, int len, char * out);

char * str_convert (const char * str, int len, const char * from_charset, const char * to_charset);
char * str_from_locale (const char * str, int len);
char * str_to_locale (const char * str, int len);
char * str_to_utf8 (const char * str, int len);

/* takes ownership of <fallbacks> and the pooled strings in it */
void str_set_charsets (const char * region, struct _Index * fallbacks);

void filename_normalize (char * filename);

char * filename_build (const char * path, const char * name);
char * filename_to_uri (const char * filename);
char * uri_to_filename (const char * uri);
char * uri_to_display (const char * uri);

void uri_parse (const char * uri, const char * * base_p, const char * * ext_p,
 const char * * sub_p, int * isub_p);
bool_t uri_get_extension (const char * uri, char * buf, int buflen);

int str_compare (const char * a, const char * b);
int str_compare_encoded (const char * a, const char * b);

struct _Index * str_list_to_index (const char * list, const char * delims);
char * index_to_str_list (struct _Index * index, const char * sep);

int str_to_int (const char * string);
double str_to_double (const char * string);
char * int_to_str (int val);
char * double_to_str (double val);

bool_t str_to_int_array (const char * string, int * array, int count);
char * int_array_to_str (const int * array, int count);
bool_t str_to_double_array (const char * string, double * array, int count);
char * double_array_to_str (const double * array, int count);

#endif /* LIBAUDCORE_STRINGS_H */
