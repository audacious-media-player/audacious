/*  Audacious
 *  Copyright (C) 2005-2009  Audacious development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "audstrings.h"

#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <ctype.h>

/**
 * Escapes characters that are special to the shell inside double quotes.
 *
 * @param string String to be escaped.
 * @return Given string with special characters escaped. Must be freed with g_free().
 */
gchar *
escape_shell_chars(const gchar * string)
{
    const gchar *special = "$`\"\\";    /* Characters to escape */
    const gchar *in = string;
    gchar *out, *escaped;
    gint num = 0;

    while (*in != '\0')
        if (strchr(special, *in++))
            num++;

    escaped = g_malloc(strlen(string) + num + 1);

    in = string;
    out = escaped;

    while (*in != '\0') {
        if (strchr(special, *in))
            *out++ = '\\';
        *out++ = *in++;
    }
    *out = '\0';

    return escaped;
}

/**
 * Performs in place replacement of Windows-style drive letter with '/' (slash).
 *
 * @param str String to be manipulated.
 * @return Pointer to the string if succesful, NULL if failed or if input was NULL.
 */
static gchar *
str_replace_drive_letter(gchar * str)
{
    gchar *match, *match_end;

    g_return_val_if_fail(str != NULL, NULL);

    while ((match = strstr(str, ":\\")) != NULL) {
        match--;
        match_end = match + 3;
        *match++ = '/';
        while (*match_end)
            *match++ = *match_end++;
        *match = 0; /* the end of line */
    }

    return str;
}

static gchar *
str_replace_char(gchar * str, gchar o, gchar n)
{
    gchar *match;

    g_return_val_if_fail(str != NULL, NULL);

    match = str;
    while ((match = strchr(match, o)) != NULL)
        *match = n;

    return str;
}

gchar *
str_append(gchar * str, const gchar * add_str)
{
    return str_replace(str, g_strconcat(str, add_str, NULL));
}

gchar *
str_replace(gchar * str, gchar * new_str)
{
    g_free(str);
    return new_str;
}

void
str_replace_in(gchar ** str, gchar * new_str)
{
    *str = str_replace(*str, new_str);
}

gboolean
str_has_prefix_nocase(const gchar * str, const gchar * prefix)
{
    /* strncasecmp causes segfaults when str is NULL*/
    return (str != NULL && (strncasecmp(str, prefix, strlen(prefix)) == 0));
}

gboolean
str_has_suffix_nocase(const gchar * str, const gchar * suffix)
{
    return (str != NULL && strcasecmp(str + strlen(str) - strlen(suffix), suffix) == 0);
}

gboolean
str_has_suffixes_nocase(const gchar * str, gchar * const *suffixes)
{
    gchar *const *suffix;

    g_return_val_if_fail(str != NULL, FALSE);
    g_return_val_if_fail(suffixes != NULL, FALSE);

    for (suffix = suffixes; *suffix; suffix++)
        if (str_has_suffix_nocase(str, *suffix))
            return TRUE;

    return FALSE;
}

gchar *
str_to_utf8_fallback(const gchar * str)
{
    gchar *out_str, *convert_str, *chr;

    if (!str)
        return NULL;

    convert_str = g_strdup(str);
    for (chr = convert_str; *chr; chr++) {
        if (*chr & 0x80)
            *chr = '?';
    }

    out_str = g_strconcat(convert_str, _("  (invalid UTF-8)"), NULL);
    g_free(convert_str);

    return out_str;
}

/**
 * Convert given string from nearly any encoding to UTF-8 encoding.
 *
 * @param str Local filename/path to convert.
 * @return String in UTF-8 encoding. Must be freed with g_free().
 */
gchar *(*str_to_utf8)(const gchar * str) = str_to_utf8_fallback;

gchar *(*chardet_to_utf8)(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write,
                       GError **arg_error) = NULL;

/**
 * Convert name of absolute path in local file system encoding
 * into UTF-8 string.
 *
 * @param filename Local filename/path to convert.
 * @return Filename converted to UTF-8 encoding.
 */
gchar *
filename_to_utf8(const gchar * filename)
{
    gchar *out_str;

    /* NULL in NULL out */
    if (filename == NULL)
        return NULL;

    if ((out_str = g_filename_to_utf8(filename, -1, NULL, NULL, NULL)))
        return out_str;

    return str_to_utf8_fallback(filename);
}

/* derives basename from uri. basename is in utf8 */
gchar *
uri_to_display_basename(const gchar * uri)
{
    gchar *realfn, *utf8fn, *basename;

    g_return_val_if_fail(uri, NULL);

    realfn = g_filename_from_uri(uri, NULL, NULL);
    utf8fn = g_filename_display_name(realfn ? realfn : uri); // guaranteed to be non-NULL
    basename = g_path_get_basename(utf8fn);

    g_free(realfn);
    g_free(utf8fn);

    return basename;
}

/* derives dirname from uri. dirname is in utf8 */
gchar *
uri_to_display_dirname(const gchar * uri)
{
    gchar *realfn, *utf8fn, *dirname;

    g_return_val_if_fail(uri, NULL);

    realfn = g_filename_from_uri(uri, NULL, NULL);
    utf8fn = g_filename_display_name(realfn ? realfn : uri);  // guaranteed to be non-NULL
    dirname = g_path_get_dirname(utf8fn);

    g_free(realfn);
    g_free(utf8fn);

    return dirname;
}

#if defined(__GLIBC__) && (__GLIBC__ >= 2)
#define HAVE_EXECINFO 1
#include <execinfo.h>
#endif

/**
 * This function can be used to assert that a given string is valid UTF-8.
 * If it is, a copy of the string is returned. However, if the string is NOT
 * valid UTF-8, a warning and a callstack backtrace is printed in order to
 * see where the problem occured.
 *
 * This is a temporary measure for removing useless str_to_utf8 etc. calls
 * and will be eventually removed. This function should be used in place of
 * #str_to_utf8() calls when it can be reasonably assumed that the input
 * should already be in unicode encoding.
 *
 * @param str String to be tested and converted to UTF-8 encoding.
 * @return String in UTF-8 encoding, or NULL if conversion failed or input was NULL.
 */
gchar *
str_assert_utf8(const gchar * str)
{
    /* NULL in NULL out */
    if (str == NULL)
        return NULL;

    /* already UTF-8? */
    if (!g_utf8_validate(str, -1, NULL)) {
#ifdef HAVE_EXECINFO
		gint i, nsymbols;
		const gint nsymmax = 50;
		void *addrbuf[nsymmax];
		gchar **symbols;
		nsymbols = backtrace(addrbuf, nsymmax);
		symbols = backtrace_symbols(addrbuf, nsymbols);

		fprintf(stderr, "String '%s' was not UTF-8! Backtrace (%d):\n", str, nsymbols);

		for (i = 0; i < nsymbols; i++)
			fprintf(stderr, "  #%d: %s\n", i, symbols[i]);

		free(symbols);
#else
		g_warning("String '%s' was not UTF-8!", str);
#endif
		return str_to_utf8(str);
    } else
    	return g_strdup(str);
}


const gchar *
str_skip_chars(const gchar * str, const gchar * chars)
{
    while (strchr(chars, *str) != NULL)
        str++;
    return str;
}

gchar *
convert_dos_path(gchar * path)
{
    g_return_val_if_fail(path != NULL, NULL);

    /* replace drive letter with '/' */
    str_replace_drive_letter(path);

    /* replace '\' with '/' */
    str_replace_char(path, '\\', '/');

    return path;
}

/**
 * Checks if given URI contains a subtune indicator/number.
 * If it does, track is set to to it, and position of subtune
 * separator in the URI string is returned.
 *
 * @param filename Filename/URI to split.
 * @param track Pointer to variable where subtune number should be
 * assigned or NULL if it is not needed.
 * @return Position of subtune separator character in filename
 * or NULL if none found. Notice that this value should NOT be modified,
 * even though it is not declared const for technical reasons.
 */
gchar *
filename_get_subtune(const gchar * filename, gint * track)
{
    gchar *pos;

    if ((pos = strrchr(filename, '?')) != NULL)
    {
        const gchar *s = pos + 1;
        while (*s != '\0' && g_ascii_isdigit(*s)) s++;
        if (*s == '\0') {
            if (track != NULL)
                *track = atoi(pos + 1);
            return pos;
        }
    }

    return NULL;
}

/**
 * Given file path/URI contains ending indicating subtune number
 * "?<number>", splits the string into filename without subtune value.
 * If given track pointer is non-NULL, subtune number is assigned into it.
 *
 * @param filename Filename/URI to split.
 * @param track Pointer to variable where subtune number should be
 * assigned or NULL if it is not needed.
 * @return Newly allocated splitted filename without the subtune indicator.
 * This string must be freed with g_free(). NULL will be returned if
 * there was any failure.
 */
gchar *
filename_split_subtune(const gchar * filename, gint * track)
{
    gchar *result;
    gchar *pos;

    g_return_val_if_fail(filename != NULL, NULL);

    result = g_strdup(filename);
    g_return_val_if_fail(result != NULL, NULL);

    if ((pos = filename_get_subtune(result, track)) != NULL)
        *pos = '\0';

    return result;
}

static gchar get_hex_digit(gchar **get)
{
    gchar c = **get;

    if (! c)
        return 0;

    (*get)++;

    if (c >= 'a')
        return c - 'a' + 10;
    if (c >= 'A')
        return c - 'A' + 10;

    return c - '0';
}

void string_decode_percent(gchar *string)
{
    gchar *get = string;
    gchar *set = string;
    gchar c;

    while ((c = *get++))
    {
        if (c == '%')
            *set++ = (get_hex_digit(&get) << 4) | get_hex_digit(&get);
        else
            *set++ = c;
    }

    *set = 0;
}

void string_cut_extension(gchar *string)
{
    gchar *period = strrchr(string, '.');

    if (period != NULL)
        *period = 0;
}

/* Like strcmp, but orders numbers correctly: "2" before "10" */

gint string_compare (const gchar * a, const gchar * b)
{
    while (* a || * b)
    {
        if (* a >= '0' && * a <= '9' && * b >= '0' && * b <= '9')
        {
            gint x = 0, y = 0;

            while (* a >= '0' && * a <= '9')
            {
                x = x * 10 + * a - '0';
                a ++;
            }

            while (* b >= '0' && * b <= '9')
            {
                y = y * 10 + * b - '0';
                b ++;
            }

            if (x > y)
                return 1;

            if (x < y)
                return -1;
        }
        else
        {
            gchar la = tolower (* a), lb = tolower (* b);

            if (la > lb)
                return 1;

            if (la < lb)
                return -1;

            a ++;
            b ++;
        }
    }

    return 0;
}
