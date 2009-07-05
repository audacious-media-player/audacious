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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "audstrings.h"

#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <ctype.h>

/*
 * escape_shell_chars()
 *
 * Escapes characters that are special to the shell inside double quotes.
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

/* replace %20 with ' ' in place */
static gchar *
str_twenty_to_space(gchar * str)
{
    gchar *match, *match_end;

    g_return_val_if_fail(str != NULL, NULL);

    while ((match = strstr(str, "%20"))) {
        match_end = match + 3;
        *match++ = ' ';
        while (*match_end)
            *match++ = *match_end++;
        *match = 0;
    }

    return str;
}

/* replace drive letter with '/' in place */
static gchar *
str_replace_drive_letter(gchar * str)
{
    gchar *match, *match_end;

    g_return_val_if_fail(str != NULL, NULL);

    while ((match = strstr(str, ":\\"))) {
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
str_replace_char(gchar * str, gchar old, gchar new)
{
    gchar *match;

    g_return_val_if_fail(str != NULL, NULL);

    match = str;
    while ((match = strchr(match, old)))
        *match = new;

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

    /* NULL in NULL out */
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

gchar *(*str_to_utf8)(const gchar * str) = str_to_utf8_fallback;
gchar *(*chardet_to_utf8)(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write,
                       GError **arg_error) = NULL;

/* convert name of absolute path in local file system encoding into utf8 string */
gchar *
filename_to_utf8(const gchar * filename)
{
    gchar *out_str;

    /* NULL in NULL out */
    if (!filename)
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

    g_free(realfn); g_free(utf8fn);

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

    g_free(realfn); g_free(utf8fn);

    return dirname;
}

/* This function is here to ASSERT that a given string IS valid UTF-8.
 * If it is, a copy of the string is returned (use g_free() to deallocate it.)
 *
 * However, if the string is NOT valid UTF-8, a warning is printed and a
 * callstack backtrace is printed in order to see where the problem occured.
 *
 * This is a temporary measure for removing useless str_to_utf8 etc. calls
 * and will be eventually removed...
 * -- ccr
 */
#if defined(__GLIBC__) && (__GLIBC__ >= 2)
#define HAVE_EXECINFO 1
#include <execinfo.h>
#endif

gchar *
str_assert_utf8(const gchar * str)
{
    /* NULL in NULL out */
    if (!str)
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

		fprintf(stderr, "WARNING! String '%s' was not UTF-8! Backtrace (%d):\n", str, nsymbols);

		for (i = 0; i < nsymbols; i++)
			fprintf(stderr, "#%d > %s\n", i, symbols[i]);
		
		free(symbols);
#endif
		return str_to_utf8(str);
    } else
    	return g_strdup(str);
}


const gchar *
str_skip_chars(const gchar * str, const gchar * chars)
{
    while (strchr(chars, *str))
        str++;
    return str;
}

gchar *
convert_title_text(gchar * title)
{
    g_return_val_if_fail(title != NULL, NULL);

    str_replace_char(title, '\\', '/');
    str_replace_char(title, '_', ' ');
    str_twenty_to_space(title);

    return title;
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
