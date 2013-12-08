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

#include <libaudcore/core.h>

/* all (char *) return values must be freed with str_unref() */

struct _Index;
typedef struct _Index Index;

bool_t str_has_prefix_nocase(const char * str, const char * prefix);
bool_t str_has_suffix_nocase(const char * str, const char * suffix);

void str_replace_char (char * string, char old_c, char new_c);

void str_decode_percent (const char * str, int len, char * out);
void str_encode_percent (const char * str, int len, char * out);

char * str_convert (const char * str, int len, const char * from_charset, const char * to_charset);
char * str_from_locale (const char * str, int len);
char * str_to_locale (const char * str, int len);
char * str_to_utf8 (const char * str, int len);

/* takes ownership of <fallbacks> and the pooled strings in it */
void str_set_charsets (const char * region, Index * fallbacks);

char * filename_to_uri (const char * filename);
char * uri_to_filename (const char * uri);
char * uri_to_display (const char * uri);

void uri_parse (const char * uri, const char * * base_p, const char * * ext_p,
 const char * * sub_p, int * isub_p);
bool_t uri_get_extension (const char * uri, char * buf, int buflen);

int str_compare (const char * a, const char * b);
int str_compare_encoded (const char * a, const char * b);

Index * str_list_to_index (const char * list, const char * delims);
char * index_to_str_list (Index * index, const char * sep);

bool_t str_to_int (const char * string, int * addr);
bool_t str_to_double (const char * string, double * addr);
char * int_to_str (int val);
char * double_to_str (double val);

bool_t str_to_int_array (const char * string, int * array, int count);
char * int_array_to_str (const int * array, int count);
bool_t str_to_double_array (const char * string, double * array, int count);
char * double_array_to_str (const double * array, int count);

#endif /* LIBAUDCORE_STRINGS_H */
