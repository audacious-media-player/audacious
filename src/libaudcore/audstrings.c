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
#include <audacious/i18n.h>
#include <string.h>
#include <ctype.h>

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

gboolean str_has_suffix_nocase (const gchar * str, const gchar * suffix)
{
    return (str && strlen (str) >= strlen (suffix) && ! strcasecmp (str + strlen
     (str) - strlen (suffix), suffix));
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

static gchar * (* str_to_utf8_impl) (const gchar *) = NULL;
static gchar * (* str_to_utf8_full_impl) (const gchar *, gssize, gsize *,
 gsize *, GError * *) = NULL;

void str_set_utf8_impl (gchar * (* stu_impl) (const gchar *),
 gchar * (* stuf_impl) (const gchar *, gssize, gsize *, gsize *, GError * *))
{
    str_to_utf8_impl = stu_impl;
    str_to_utf8_full_impl = stuf_impl;
}

/**
 * Convert given string from nearly any encoding to UTF-8 encoding.
 *
 * @param str Local filename/path to convert.
 * @return String in UTF-8 encoding. Must be freed with g_free().
 */

gchar * str_to_utf8 (const gchar * str)
{
    g_return_val_if_fail (str_to_utf8_impl, NULL);
    return str_to_utf8_impl (str);
}

gchar * str_to_utf8_full (const gchar * str, gssize len, gsize * bytes_read,
 gsize * bytes_written, GError * * err)
{
    g_return_val_if_fail (str_to_utf8_full_impl, NULL);
    return str_to_utf8_full_impl (str, len, bytes_read, bytes_written, err);
}

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
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
#ifdef HAVE_EXECINFO_H
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
    string_replace_char (path, '\\', '/');

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

void string_replace_char (gchar * string, gchar old_str, gchar new_str)
{
    while ((string = strchr (string, old_str)) != NULL)
        * string = new_str;
}

static inline gchar get_hex_digit (const gchar * * get)
{
    gchar c = * * get;

    if (! c)
        return 0;

    (* get) ++;

    if (c < 'A')
        return c - '0';
    if (c < 'a')
        return c - 'A' + 10;

    return c - 'a' + 10;
}

/* Requires that the destination be large enough to hold the decoded string.
 * The source and destination may be the same string.  USE EXTREME CAUTION. */

static void string_decode_percent_2 (const gchar * from, gchar * to)
{
    gchar c;
    while ((c = * from ++))
        * to ++ = (c != '%') ? c : ((get_hex_digit (& from) << 4) | get_hex_digit
         (& from));

    * to = 0;
}

/* Decodes a percent-encoded string in-place. */

void string_decode_percent (gchar * s)
{
    string_decode_percent_2 (s, s);
}

/* We encode any character except the "unreserved" characters of RFC 3986 and
 * (optionally) the forward slash.  On Windows, we also (optionally) do not
 * encode the colon. */
static gboolean is_legal_char (gchar c, gboolean is_filename)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <=
     '9') || (strchr ("-_.~", c) != NULL) ||
#ifdef _WIN32
     (is_filename && strchr ("/:", c) != NULL);
#else
     (is_filename && c == '/');
#endif
}

static gchar make_hex_digit (gint i)
{
    if (i < 10)
        return '0' + i;
    else
        return ('A' - 10) + i;
}

/* is_filename specifies whether the forward slash should be left intact */
/* returns string allocated with g_malloc */
gchar * string_encode_percent (const gchar * string, gboolean is_filename)
{
    gint length = 0;
    const gchar * get;
    gchar c;
    gchar * new, * set;

    for (get = string; (c = * get); get ++)
    {
        if (is_legal_char (c, is_filename))
            length ++;
        else
            length += 3;
    }

    new = g_malloc (length + 1);
    set = new;

    for (get = string; (c = * get); get ++)
    {
        if (is_legal_char (c, is_filename))
            * set ++ = c;
        else
        {
            * set ++ = '%';
            * set ++ = make_hex_digit (((guchar) c) >> 4);
            * set ++ = make_hex_digit (c & 0xF);
        }
    }

    * set = 0;
    return new;
}

/* Determines whether a URI is valid UTF-8.  If not and <warn> is nonzero,
 * prints a warning to stderr. */

gboolean uri_is_utf8 (const gchar * uri, gboolean warn)
{
    gchar buf[strlen (uri) + 1];
    string_decode_percent_2 (uri, buf);

    if (g_utf8_validate (buf, -1, NULL))
        return TRUE;

    if (warn)
        fprintf (stderr, "URI is not UTF-8: %s.\n", buf);

    return FALSE;
}

/* Converts a URI to UTF-8 encoding.  The returned URI must be freed with g_free.
 *
 * Note: The function intentionally converts only URI's that are encoded in the
 * system locale and refer to local files.
 *
 * Rationale:
 *
 * 1. Local files.  The URI was probably created by percent-encoding a raw
 *    filename.
 *    a. If that filename was in the system locale, then we can convert the URI
 *       to a UTF-8 one, allowing us to display the name correctly and to access
 *       the file by converting back to the system locale.
 *    b. If that filename was in a different locale (perhaps copied from another
 *       machine), then we do not want to convert it to UTF-8 (even assuming we
 *       can do so correctly), because we will not know what encoding to convert
 *       back to when we want to access the file.
 * 2. Remote files.  The URI was probably created by percent-encoding a raw
 *    filename in whatever locale the remote system is using.  We do not want
 *    to convert it to UTF-8 because we do not know whether the remote system
 *    can handle UTF-8 requests. */

gchar * uri_to_utf8 (const gchar * uri)
{
    if (strncmp (uri, "file://", 7))
        return g_strdup (uri);

    /* recover the raw filename */
    gchar buf[strlen (uri + 7) + 1];
    string_decode_percent_2 (uri + 7, buf);

    /* convert it to a URI again, in UTF-8 if possible */
    return filename_to_uri (buf);
}

/* Check that a URI is valid UTF-8.  If not, prints a warning to stderr if
 * <warn> is nonzero, frees the old URI with g_free, and sets <uri> to the
 * converted URI, which must be freed with g_free when no longer needed. */

void uri_check_utf8 (gchar * * uri, gboolean warn)
{
    if (uri_is_utf8 (* uri, warn))
        return;

    gchar * copy = uri_to_utf8 (* uri);
    g_free (* uri);
    * uri = copy;
}

/* Like g_filename_to_uri, but converts the filename from the system locale to
 * UTF-8 before percent-encoding.  On Windows, replaces '\' with '/' and adds a
 * leading '/'. */

gchar * filename_to_uri (const gchar * name)
{
    gchar * utf8 = g_locale_to_utf8 (name, -1, NULL, NULL, NULL);
#ifdef _WIN32
    string_replace_char (utf8, '\\', '/');
#endif
    gchar * enc = string_encode_percent (utf8 ? utf8 : name, TRUE);
    g_free (utf8);
#ifdef _WIN32
    gchar * uri = g_strdup_printf ("file:///%s", enc);
#else
    gchar * uri = g_strdup_printf ("file://%s", enc);
#endif
    g_free (enc);
    return uri;
}

/* Like g_filename_from_uri, but converts the filename from UTF-8 to the system
 * locale after percent-decoding.  On Windows, strips the leading '/' and
 * replaces '/' with '\'. */

gchar * uri_to_filename (const gchar * uri)
{
#ifdef _WIN32
    g_return_val_if_fail (! strncmp (uri, "file:///", 8), NULL);
    gchar buf[strlen (uri + 8) + 1];
    string_decode_percent_2 (uri + 8, buf);
#else
    g_return_val_if_fail (! strncmp (uri, "file://", 7), NULL);
    gchar buf[strlen (uri + 7) + 1];
    string_decode_percent_2 (uri + 7, buf);
#endif
#ifdef _WIN32
    string_replace_char (buf, '/', '\\');
#endif
    gchar * name = g_locale_from_utf8 (buf, -1, NULL, NULL, NULL);
    return name ? name : g_strdup (buf);
}

/* Formats a URI for human-readable display.  Percent-decodes and converts to
 * UTF-8 (more aggressively than uri_to_utf8).  For file:// URI's, converts to
 * filename format (but in UTF-8). */

gchar * uri_to_display (const gchar * uri)
{
    gchar buf[strlen (uri) + 1];

#ifdef _WIN32
    if (! strncmp (uri, "file:///", 8))
    {
        string_decode_percent_2 (uri + 8, buf);
        string_replace_char (buf, '/', '\\');
    }
#else
    if (! strncmp (uri, "file://", 7))
        string_decode_percent_2 (uri + 7, buf);
#endif
    else
        string_decode_percent_2 (uri, buf);

    return str_to_utf8 (buf);
}

gchar * uri_get_extension (const gchar * uri)
{
    const gchar * slash = strrchr (uri, '/');
    if (! slash)
        return NULL;
    
    gchar * lower = g_ascii_strdown (slash + 1, -1);

    gchar * qmark = strchr (lower, '?');
    if (qmark)
        * qmark = 0;

    gchar * dot = strrchr (lower, '.');
    gchar * ext = dot ? g_strdup (dot + 1) : NULL;
    
    g_free (lower);
    return ext;
}

void string_cut_extension(gchar *string)
{
    gchar *period = strrchr(string, '.');

    if (period != NULL)
        *period = 0;
}

/* Like strcasecmp, but orders numbers correctly (2 before 10). */
/* Non-ASCII characters are treated exactly as is. */
/* Handles NULL gracefully. */

gint string_compare (const gchar * ap, const gchar * bp)
{
    if (ap == NULL)
        return (bp == NULL) ? 0 : -1;
    if (bp == NULL)
        return 1;

    guchar a = * ap ++, b = * bp ++;
    for (; a || b; a = * ap ++, b = * bp ++)
    {
        if (a > '9' || b > '9' || a < '0' || b < '0')
        {
            if (a <= 'Z' && a >= 'A')
                a += 'a' - 'A';
            if (b <= 'Z' && b >= 'A')
                b += 'a' - 'A';

            if (a > b)
                return 1;
            if (a < b)
                return -1;
        }
        else
        {
            gint x = a - '0';
            for (; (a = * ap) <= '9' && a >= '0'; ap ++)
                x = 10 * x + (a - '0');

            gint y = b - '0';
            for (; (b = * bp) >= '0' && b <= '9'; bp ++)
                y = 10 * y + (b - '0');

            if (x > y)
                return 1;
            if (x < y)
                return -1;
        }
    }

    return 0;
}

/* Decodes percent-encoded strings, then compares then with string_compare. */

gint string_compare_encoded (const gchar * ap, const gchar * bp)
{
    if (ap == NULL)
        return (bp == NULL) ? 0 : -1;
    if (bp == NULL)
        return 1;

    guchar a = * ap ++, b = * bp ++;
    for (; a || b; a = * ap ++, b = * bp ++)
    {
        if (a == '%')
            a = (get_hex_digit (& ap) << 4) | get_hex_digit (& ap);
        if (b == '%')
            b = (get_hex_digit (& bp) << 4) | get_hex_digit (& bp);

        if (a > '9' || b > '9' || a < '0' || b < '0')
        {
            if (a <= 'Z' && a >= 'A')
                a += 'a' - 'A';
            if (b <= 'Z' && b >= 'A')
                b += 'a' - 'A';

            if (a > b)
                return 1;
            if (a < b)
                return -1;
        }
        else
        {
            gint x = a - '0';
            for (; (a = * ap) <= '9' && a >= '0'; ap ++)
                x = 10 * x + (a - '0');

            gint y = b - '0';
            for (; (b = * bp) >= '0' && b <= '9'; bp ++)
                y = 10 * y + (b - '0');

            if (x > y)
                return 1;
            if (x < y)
                return -1;
        }
    }

    return 0;
}

const void * memfind (const void * mem, gint size, const void * token, gint
 length)
{
    if (! length)
        return mem;

    size -= length - 1;

    while (size > 0)
    {
        const void * maybe = memchr (mem, * (guchar *) token, size);

        if (maybe == NULL)
            return NULL;

        if (! memcmp (maybe, token, length))
            return maybe;

        size -= (guchar *) maybe + 1 - (guchar *) mem;
        mem = (guchar *) maybe + 1;
    }

    return NULL;
}

gchar *
str_replace_fragment(gchar *s, gint size, const gchar *old, const gchar *new)
{
    gchar *ptr = s;
    gint left = strlen(s);
    gint avail = size - (left + 1);
    gint oldlen = strlen(old);
    gint newlen = strlen(new);
    gint diff = newlen - oldlen;

    while (left >= oldlen)
    {
        if (strncmp(ptr, old, oldlen))
        {
            left--;
            ptr++;
            continue;
        }

        if (diff > avail)
            break;

        if (diff != 0)
            memmove(ptr + oldlen + diff, ptr + oldlen, left + 1 - oldlen);

        memcpy(ptr, new, newlen);
        ptr += newlen;
        left -= oldlen;
    }

    return s;
}

void
string_canonize_case(gchar *str)
{
    while (*str)
    {
        *str = g_ascii_toupper(*str);
        str++;
    }
}
