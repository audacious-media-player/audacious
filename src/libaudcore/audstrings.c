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

#include <math.h>
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
static char *
str_replace_drive_letter(char * str)
{
    char *match, *match_end;

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

char *
str_append(char * str, const char * add_str)
{
    return str_replace(str, g_strconcat(str, add_str, NULL));
}

char *
str_replace(char * str, char * new_str)
{
    g_free(str);
    return new_str;
}

void
str_replace_in(char ** str, char * new_str)
{
    *str = str_replace(*str, new_str);
}

gboolean
str_has_prefix_nocase(const char * str, const char * prefix)
{
    /* strncasecmp causes segfaults when str is NULL*/
    return (str != NULL && (strncasecmp(str, prefix, strlen(prefix)) == 0));
}

gboolean str_has_suffix_nocase (const char * str, const char * suffix)
{
    return (str && strlen (str) >= strlen (suffix) && ! strcasecmp (str + strlen
     (str) - strlen (suffix), suffix));
}

gboolean
str_has_suffixes_nocase(const char * str, char * const *suffixes)
{
    char *const *suffix;

    g_return_val_if_fail(str != NULL, FALSE);
    g_return_val_if_fail(suffixes != NULL, FALSE);

    for (suffix = suffixes; *suffix; suffix++)
        if (str_has_suffix_nocase(str, *suffix))
            return TRUE;

    return FALSE;
}

static char * (* str_to_utf8_impl) (const char *) = NULL;
static char * (* str_to_utf8_full_impl) (const char *, gssize, gsize *,
 gsize *, GError * *) = NULL;

void str_set_utf8_impl (char * (* stu_impl) (const char *),
 char * (* stuf_impl) (const char *, gssize, gsize *, gsize *, GError * *))
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

char * str_to_utf8 (const char * str)
{
    g_return_val_if_fail (str_to_utf8_impl, NULL);
    return str_to_utf8_impl (str);
}

char * str_to_utf8_full (const char * str, gssize len, gsize * bytes_read,
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
char *
str_assert_utf8(const char * str)
{
    /* NULL in NULL out */
    if (str == NULL)
        return NULL;

    /* already UTF-8? */
    if (!g_utf8_validate(str, -1, NULL)) {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_BACKTRACE)
        int i, nsymbols;
        const int nsymmax = 50;
        void *addrbuf[nsymmax];
        char **symbols;
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


const char *
str_skip_chars(const char * str, const char * chars)
{
    while (strchr(chars, *str) != NULL)
        str++;
    return str;
}

char *
convert_dos_path(char * path)
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
char *
filename_get_subtune(const char * filename, int * track)
{
    char *pos;

    if ((pos = strrchr(filename, '?')) != NULL)
    {
        const char *s = pos + 1;
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
char *
filename_split_subtune(const char * filename, int * track)
{
    char *result;
    char *pos;

    g_return_val_if_fail(filename != NULL, NULL);

    result = g_strdup(filename);
    g_return_val_if_fail(result != NULL, NULL);

    if ((pos = filename_get_subtune(result, track)) != NULL)
        *pos = '\0';

    return result;
}

void string_replace_char (char * string, char old_str, char new_str)
{
    while ((string = strchr (string, old_str)) != NULL)
        * string = new_str;
}

static inline char get_hex_digit (const char * * get)
{
    char c = * * get;

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

static void string_decode_percent_2 (const char * from, char * to)
{
    char c;
    while ((c = * from ++))
        * to ++ = (c != '%') ? c : ((get_hex_digit (& from) << 4) | get_hex_digit
         (& from));

    * to = 0;
}

/* Decodes a percent-encoded string in-place. */

void string_decode_percent (char * s)
{
    string_decode_percent_2 (s, s);
}

/* We encode any character except the "unreserved" characters of RFC 3986 and
 * (optionally) the forward slash.  On Windows, we also (optionally) do not
 * encode the colon. */
static gboolean is_legal_char (char c, gboolean is_filename)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <=
     '9') || (strchr ("-_.~", c) != NULL) ||
#ifdef _WIN32
     (is_filename && strchr ("/:", c) != NULL);
#else
     (is_filename && c == '/');
#endif
}

static char make_hex_digit (int i)
{
    if (i < 10)
        return '0' + i;
    else
        return ('A' - 10) + i;
}

/* is_filename specifies whether the forward slash should be left intact */
/* returns string allocated with g_malloc */
char * string_encode_percent (const char * string, gboolean is_filename)
{
    int length = 0;
    const char * get;
    char c;
    char * new, * set;

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
            * set ++ = make_hex_digit (((unsigned char) c) >> 4);
            * set ++ = make_hex_digit (c & 0xF);
        }
    }

    * set = 0;
    return new;
}

/* Determines whether a URI is valid UTF-8.  If not and <warn> is nonzero,
 * prints a warning to stderr. */

gboolean uri_is_utf8 (const char * uri, gboolean warn)
{
    char buf[strlen (uri) + 1];
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

char * uri_to_utf8 (const char * uri)
{
    if (strncmp (uri, "file://", 7))
        return g_strdup (uri);

    /* recover the raw filename */
    char buf[strlen (uri + 7) + 1];
    string_decode_percent_2 (uri + 7, buf);

    /* convert it to a URI again, in UTF-8 if possible */
    return filename_to_uri (buf);
}

/* Check that a URI is valid UTF-8.  If not, prints a warning to stderr if
 * <warn> is nonzero, frees the old URI with g_free, and sets <uri> to the
 * converted URI, which must be freed with g_free when no longer needed. */

void uri_check_utf8 (char * * uri, gboolean warn)
{
    if (uri_is_utf8 (* uri, warn))
        return;

    char * copy = uri_to_utf8 (* uri);
    g_free (* uri);
    * uri = copy;
}

/* Like g_filename_to_uri, but converts the filename from the system locale to
 * UTF-8 before percent-encoding.  On Windows, replaces '\' with '/' and adds a
 * leading '/'. */

char * filename_to_uri (const char * name)
{
    char * utf8 = g_locale_to_utf8 (name, -1, NULL, NULL, NULL);
#ifdef _WIN32
    string_replace_char (utf8, '\\', '/');
#endif
    char * enc = string_encode_percent (utf8 ? utf8 : name, TRUE);
    g_free (utf8);
#ifdef _WIN32
    char * uri = g_strdup_printf ("file:///%s", enc);
#else
    char * uri = g_strdup_printf ("file://%s", enc);
#endif
    g_free (enc);
    return uri;
}

/* Like g_filename_from_uri, but converts the filename from UTF-8 to the system
 * locale after percent-decoding.  On Windows, strips the leading '/' and
 * replaces '/' with '\'. */

char * uri_to_filename (const char * uri)
{
#ifdef _WIN32
    g_return_val_if_fail (! strncmp (uri, "file:///", 8), NULL);
    char buf[strlen (uri + 8) + 1];
    string_decode_percent_2 (uri + 8, buf);
#else
    g_return_val_if_fail (! strncmp (uri, "file://", 7), NULL);
    char buf[strlen (uri + 7) + 1];
    string_decode_percent_2 (uri + 7, buf);
#endif
#ifdef _WIN32
    string_replace_char (buf, '/', '\\');
#endif
    char * name = g_locale_from_utf8 (buf, -1, NULL, NULL, NULL);
    return name ? name : g_strdup (buf);
}

/* Formats a URI for human-readable display.  Percent-decodes and converts to
 * UTF-8 (more aggressively than uri_to_utf8).  For file:// URI's, converts to
 * filename format (but in UTF-8). */

char * uri_to_display (const char * uri)
{
    char buf[strlen (uri) + 1];

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

char * uri_get_extension (const char * uri)
{
    const char * slash = strrchr (uri, '/');
    if (! slash)
        return NULL;

    char * lower = g_ascii_strdown (slash + 1, -1);

    char * qmark = strchr (lower, '?');
    if (qmark)
        * qmark = 0;

    char * dot = strrchr (lower, '.');
    char * ext = dot ? g_strdup (dot + 1) : NULL;

    g_free (lower);
    return ext;
}

void string_cut_extension(char *string)
{
    char *period = strrchr(string, '.');

    if (period != NULL)
        *period = 0;
}

/* Like strcasecmp, but orders numbers correctly (2 before 10). */
/* Non-ASCII characters are treated exactly as is. */
/* Handles NULL gracefully. */

int string_compare (const char * ap, const char * bp)
{
    if (ap == NULL)
        return (bp == NULL) ? 0 : -1;
    if (bp == NULL)
        return 1;

    unsigned char a = * ap ++, b = * bp ++;
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
            int x = a - '0';
            for (; (a = * ap) <= '9' && a >= '0'; ap ++)
                x = 10 * x + (a - '0');

            int y = b - '0';
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

int string_compare_encoded (const char * ap, const char * bp)
{
    if (ap == NULL)
        return (bp == NULL) ? 0 : -1;
    if (bp == NULL)
        return 1;

    unsigned char a = * ap ++, b = * bp ++;
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
            int x = a - '0';
            for (; (a = * ap) <= '9' && a >= '0'; ap ++)
                x = 10 * x + (a - '0');

            int y = b - '0';
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

const void * memfind (const void * mem, int size, const void * token, int
 length)
{
    if (! length)
        return mem;

    size -= length - 1;

    while (size > 0)
    {
        const void * maybe = memchr (mem, * (unsigned char *) token, size);

        if (maybe == NULL)
            return NULL;

        if (! memcmp (maybe, token, length))
            return maybe;

        size -= (unsigned char *) maybe + 1 - (unsigned char *) mem;
        mem = (unsigned char *) maybe + 1;
    }

    return NULL;
}

char *
str_replace_fragment(char *s, int size, const char *old, const char *new)
{
    char *ptr = s;
    int left = strlen(s);
    int avail = size - (left + 1);
    int oldlen = strlen(old);
    int newlen = strlen(new);
    int diff = newlen - oldlen;

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
string_canonize_case(char *str)
{
    while (*str)
    {
        *str = g_ascii_toupper(*str);
        str++;
    }
}

/*
 * Routines to convert numbers between string and binary representations.
 *
 * Goals:
 *
 *  - Accuracy, meaning that we can convert back and forth between string and
 *    binary without the number changing slightly each time.
 *  - Consistency, meaning that we get the same results no matter what
 *    architecture or locale we have to deal with.
 *  - Readability, meaning that the number one is rendered "1", not "1.000".
 *
 * Values are limited between -1,000,000,000 and 1,000,000,000 (inclusive) and
 * have an accuracy of 6 decimal places.
 */

gboolean string_to_int (const char * string, int * addr)
{
    gboolean neg = (string[0] == '-');
    if (neg)
        string ++;

    int val = 0;
    char c;

    while ((c = * string ++))
    {
        if (c < '0' || c > '9' || val > 100000000)
            goto ERR;

        val = val * 10 + (c - '0');
    }

    if (val > 1000000000)
        goto ERR;

    * addr = neg ? -val : val;
    return TRUE;

ERR:
    return FALSE;
}

gboolean string_to_double (const char * string, double * addr)
{
    gboolean neg = (string[0] == '-');
    if (neg)
        string ++;

    const char * p = strchr (string, '.');
    int i, f;

    if (p)
    {
        char buf[11];
        int len;

        len = p - string;
        if (len > 10)
            goto ERR;

        memcpy (buf, string, len);
        buf[len] = 0;

        if (! string_to_int (buf, & i))
            goto ERR;

        len = strlen (p + 1);
        if (len > 6)
            goto ERR;

        memcpy (buf, p + 1, len);
        memset (buf + len, '0', 6 - len);
        buf[6] = 0;

        if (! string_to_int (buf, & f))
            goto ERR;
    }
    else
    {
        if (! string_to_int (string, & i))
            goto ERR;

        f = 0;
    }

    double val = i + (double) f / 1000000;
    if (val > 1000000000)
        goto ERR;

    * addr = neg ? -val : val;
    return TRUE;

ERR:
    return FALSE;
}

char * int_to_string (int val)
{
    g_return_val_if_fail (val >= -1000000000 && val <= 1000000000, NULL);
    return g_strdup_printf ("%d", val);
}

char * double_to_string (double val)
{
    g_return_val_if_fail (val >= -1000000000 && val <= 1000000000, NULL);

    gboolean neg = (val < 0);
    if (neg)
        val = -val;

    int i = floor (val);
    int f = round ((val - i) * 1000000);

    if (f == 1000000)
    {
        i ++;
        f = 0;
    }

    char * s = neg ? g_strdup_printf ("-%d.%06d", i, f) : g_strdup_printf ("%d.%06d", i, f);

    char * c = s + strlen (s);
    while (* (c - 1) == '0')
        c --;
    if (* (c - 1) == '.')
        c --;
    * c = 0;

    return s;
}

gboolean string_to_double_array (const char * string, double * array, int count)
{
    char * * split = g_strsplit (string, ",", -1);
    if (g_strv_length (split) != count)
        goto ERR;

    for (int i = 0; i < count; i ++)
    {
        if (! string_to_double (split[i], & array[i]))
            goto ERR;
    }

    g_strfreev (split);
    return TRUE;

ERR:
    g_strfreev (split);
    return FALSE;
}

char * double_array_to_string (const double * array, int count)
{
    char * * split = g_malloc0 (sizeof (char *) * (count + 1));

    for (int i = 0; i < count; i ++)
    {
        split[i] = double_to_string (array[i]);
        if (! split[i])
            goto ERR;
    }

    char * string = g_strjoinv (",", split);
    g_strfreev (split);
    return string;

ERR:
    g_strfreev (split);
    return NULL;
}
