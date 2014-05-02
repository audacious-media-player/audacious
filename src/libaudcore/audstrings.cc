/*
 * audstrings.c
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

#include "audstrings.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "i18n.h"
#include "index.h"
#include "runtime.h"

static const char ascii_to_hex[256] =
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\x0\x0\x0\x0\x0\x0"
    "\x0\xa\xb\xc\xd\xe\xf\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\xa\xb\xc\xd\xe\xf\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0";

static const char hex_to_ascii[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const char uri_legal_table[256] =
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x1\x1"  // '-' '.' '/'
    "\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x0\x0\x0\x0"  // 0-9
    "\x0\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1"  // A-O
    "\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x0\x0\x1"  // P-Z '_'
    "\x0\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1"  // a-o
    "\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x0\x1\x0"; // p-z '~'

static const char swap_case[256] =
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
    "\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0\0\0\0\0";

#define FROM_HEX(c)  (ascii_to_hex[(unsigned char) (c)])
#define TO_HEX(i)    (hex_to_ascii[(i) & 15])
#define IS_LEGAL(c)  (uri_legal_table[(unsigned char) (c)])
#define SWAP_CASE(c) (swap_case[(unsigned char) (c)])

EXPORT String str_nget (const char * str, int len)
{
    if (memchr (str, 0, len))
        return String (str);

    SNCOPY (buf, str, len);
    return String (buf);
}

EXPORT String str_printf (const char * format, ...)
{
    va_list args;
    va_start (args, format);

    String str = str_vprintf (format, args);

    va_end (args);
    return str;
}

EXPORT String str_vprintf (const char * format, va_list args)
{
    VSPRINTF (buf, format, args);
    return String (buf);
}

EXPORT bool_t str_has_prefix_nocase (const char * str, const char * prefix)
{
    return ! g_ascii_strncasecmp (str, prefix, strlen (prefix));
}

EXPORT bool_t str_has_suffix_nocase (const char * str, const char * suffix)
{
    int len1 = strlen (str);
    int len2 = strlen (suffix);

    if (len2 > len1)
        return FALSE;

    return ! g_ascii_strcasecmp (str + len1 - len2, suffix);
}

EXPORT char * strstr_nocase (const char * haystack, const char * needle)
{
    while (1)
    {
        const char * ap = haystack;
        const char * bp = needle;

        while (1)
        {
            char a = * ap ++;
            char b = * bp ++;

            if (! b) /* all of needle matched */
                return (char *) haystack;
            if (! a) /* end of haystack reached */
                return NULL;

            if (a != b && a != SWAP_CASE (b))
                break;
        }

        haystack ++;
    }
}

EXPORT char * strstr_nocase_utf8 (const char * haystack, const char * needle)
{
    while (1)
    {
        const char * ap = haystack;
        const char * bp = needle;

        while (1)
        {
            gunichar a = g_utf8_get_char (ap);
            gunichar b = g_utf8_get_char (bp);

            if (! b) /* all of needle matched */
                return (char *) haystack;
            if (! a) /* end of haystack reached */
                return NULL;

            if (a != b && (a < 128 ? (gunichar) SWAP_CASE (a) != b :
             g_unichar_tolower (a) != g_unichar_tolower (b)))
                break;

            ap = g_utf8_next_char (ap);
            bp = g_utf8_next_char (bp);
        }

        haystack = g_utf8_next_char (haystack);
    }
}

EXPORT String str_tolower_utf8 (const char * str)
{
    char buf[6 * strlen (str) + 1];
    const char * get = str;
    char * set = buf;
    gunichar c;

    while ((c = g_utf8_get_char (get)))
    {
        if (c < 128)
            * set ++ = g_ascii_tolower (c);
        else
            set += g_unichar_to_utf8 (g_unichar_tolower (c), set);

        get = g_utf8_next_char (get);
    }

    * set = 0;

    return String (buf);
}

EXPORT void str_replace_char (char * string, char old_c, char new_c)
{
    while ((string = strchr (string, old_c)))
        * string ++ = new_c;
}

EXPORT void str_itoa (int x, char * buf, int bufsize)
{
    if (! bufsize)
        return;

    if (x < 0)
    {
        if (bufsize > 1)
        {
            * buf ++ = '-';
            bufsize --;
        }

        x = -x;
    }

    char * rev = buf + bufsize - 1;
    * rev = 0;

    while (rev > buf)
    {
        * (-- rev) = '0' + x % 10;
        if (! (x /= 10))
            break;
    }

    while ((* buf ++ = * rev ++));
}

/* Percent-decodes up to <len> bytes of <str> to <out>, which must be large
 * enough to hold the decoded string (i.e., (len + 1) bytes).  If <len> is
 * negative, decodes all of <str>. */

EXPORT void str_decode_percent (const char * str, int len, char * out)
{
    const char * nul;

    if (len < 0)
        len = strlen (str);
    else if ((nul = (const char *) memchr (str, 0, len)))
        len = nul - str;

    while (1)
    {
        const char * p = (const char *) memchr (str, '%', len);
        if (! p)
            break;

        int block = p - str;
        memmove (out, str, block);

        str += block;
        out += block;
        len -= block;

        if (len < 3)
            break;

        * out ++ = (FROM_HEX (str[1]) << 4) | FROM_HEX (str[2]);

        str += 3;
        len -= 3;
    }

    memmove (out, str, len);
    out[len] = 0;
}

/* Percent-encodes up to <len> bytes of <str> to <out>, which must be large
 * enough to hold the encoded string (i.e., (3 * len + 1) bytes).  If <len> is
 * negative, decodes all of <str>. */

EXPORT void str_encode_percent (const char * str, int len, char * out)
{
    const char * nul;

    if (len < 0)
        len = strlen (str);
    else if ((nul = (const char *) memchr (str, 0, len)))
        len = nul - str;

    while (len --)
    {
        char c = * str ++;

        if (IS_LEGAL (c))
            * out ++ = c;
        else
        {
            * out ++ = '%';
            * out ++ = TO_HEX ((unsigned char) c >> 4);
            * out ++ = TO_HEX (c & 0xF);
        }
    }

    * out = 0;
}

EXPORT void filename_normalize (char * filename)
{
#ifdef _WIN32
    /* convert slash to backslash on Windows */
    str_replace_char (filename, '/', '\\');
#endif

    /* remove trailing slash */
    int len = strlen (filename);
#ifdef _WIN32
    if (len > 3 && filename[len - 1] == '\\') /* leave "C:\" */
#else
    if (len > 1 && filename[len - 1] == '/') /* leave leading "/" */
#endif
        filename[len - 1] = 0;
}

EXPORT String filename_build (const char * path, const char * name)
{
    int len = strlen (path);

#ifdef _WIN32
    if (! len || path[len - 1] == '/' || path[len - 1] == '\\')
    {
        SCONCAT2 (filename, path, name);
        return String (filename);
    }

    SCONCAT3 (filename, path, "\\", name);
    return String (filename);
#else
    if (! len || path[len - 1] == '/')
    {
        SCONCAT2 (filename, path, name);
        return String (filename);
    }

    SCONCAT3 (filename, path, "/", name);
    return String (filename);
#endif
}

#ifdef _WIN32
#define URI_PREFIX "file:///"
#define URI_PREFIX_LEN 8
#else
#define URI_PREFIX "file://"
#define URI_PREFIX_LEN 7
#endif

/* Like g_filename_to_uri, but converts the filename from the system locale to
 * UTF-8 before percent-encoding (except on Windows, where filenames are assumed
 * to be UTF-8).  On Windows, replaces '\' with '/' and adds a leading '/'. */

EXPORT String filename_to_uri (const char * name)
{
#ifdef _WIN32
    SCOPY (utf8, name);
    str_replace_char (utf8, '\\', '/');
#else
    String utf8 = str_from_locale (name, -1);
    if (! utf8)
        return String ();
#endif

    char enc[URI_PREFIX_LEN + 3 * strlen (utf8) + 1];
    strcpy (enc, URI_PREFIX);
    str_encode_percent (utf8, -1, enc + URI_PREFIX_LEN);
    return String (enc);
}

/* Like g_filename_from_uri, but converts the filename from UTF-8 to the system
 * locale after percent-decoding (except on Windows, where filenames are assumed
 * to be UTF-8).  On Windows, strips the leading '/' and replaces '/' with '\'. */

EXPORT String uri_to_filename (const char * uri)
{
    if (strncmp (uri, URI_PREFIX, URI_PREFIX_LEN))
        return String ();

    char buf[strlen (uri + URI_PREFIX_LEN) + 1];
    str_decode_percent (uri + URI_PREFIX_LEN, -1, buf);

    filename_normalize (buf);

#ifdef _WIN32
    return String (buf);
#else
    return str_to_locale (buf, -1);
#endif
}

/* Formats a URI for human-readable display.  Percent-decodes and, for file://
 * URI's, converts to filename format, but in UTF-8. */

EXPORT String uri_to_display (const char * uri)
{
    if (! strncmp (uri, "cdda://?", 8))
        return str_printf (_("Audio CD, track %s"), uri + 8);

    char buf[strlen (uri) + 1];

    if (! strncmp (uri, URI_PREFIX, URI_PREFIX_LEN))
    {
        str_decode_percent (uri + URI_PREFIX_LEN, -1, buf);
#ifdef _WIN32
        str_replace_char (buf, '/', '\\');
#endif
    }
    else
        str_decode_percent (uri, -1, buf);

    return String (buf);
}

#undef URI_PREFIX
#undef URI_PREFIX_LEN

EXPORT void uri_parse (const char * uri, const char * * base_p, const char * * ext_p,
 const char * * sub_p, int * isub_p)
{
    const char * end = uri + strlen (uri);
    const char * base, * ext, * sub, * c;
    int isub = 0;
    char junk;

    if ((c = strrchr (uri, '/')))
        base = c + 1;
    else
        base = end;

    if ((c = strrchr (base, '?')) && sscanf (c + 1, "%d%c", & isub, & junk) == 1)
        sub = c;
    else
        sub = end;

    SNCOPY (buf, base, sub - base);

    if ((c = strrchr (buf, '.')))
        ext = base + (c - buf);
    else
        ext = sub;

    if (base_p)
        * base_p = base;
    if (ext_p)
        * ext_p = ext;
    if (sub_p)
        * sub_p = sub;
    if (isub_p)
        * isub_p = isub;
}

EXPORT bool_t uri_get_extension (const char * uri, char * buf, int buflen)
{
    const char * ext;
    uri_parse (uri, NULL, & ext, NULL, NULL);

    if (ext[0] != '.')
        return FALSE;

    strncpy (buf, ext + 1, buflen - 1);
    buf[buflen - 1] = 0;

    /* remove subtunes and HTTP query strings */
    char * qmark;
    if ((qmark = strchr (buf, '?')))
        * qmark = 0;

    return (buf[0] != 0);
}

/* Constructs a full URI given:
 *   1. path: one of the following:
 *     a. a full URI (returned unchanged)
 *     b. an absolute filename (in the system locale)
 *     c. a relative path (character set detected according to user settings)
 *   2. reference: the full URI of the playlist containing <path> */

EXPORT String uri_construct (const char * path, const char * reference)
{
    /* URI */
    if (strstr (path, "://"))
        return String (path);

    /* absolute filename */
#ifdef _WIN32
    if (path[0] && path[1] == ':' && path[2] == '\\')
#else
    if (path[0] == '/')
#endif
        return filename_to_uri (path);

    /* relative path */
    const char * slash = strrchr (reference, '/');
    if (! slash)
        return String ();

    String utf8 = str_to_utf8 (path, -1);
    if (! utf8)
        return String ();

    int pathlen = slash + 1 - reference;

    char buf[pathlen + 3 * strlen (utf8) + 1];
    memcpy (buf, reference, pathlen);

    if (aud_get_bool (NULL, "convert_backslash"))
    {
        SCOPY (tmp, utf8);
        str_replace_char (tmp, '\\', '/');
        str_encode_percent (tmp, -1, buf + pathlen);
    }
    else
        str_encode_percent (utf8, -1, buf + pathlen);

    return String (buf);
}

/* Like strcasecmp, but orders numbers correctly (2 before 10). */
/* Non-ASCII characters are treated exactly as is. */
/* Handles NULL gracefully. */

EXPORT int str_compare (const char * ap, const char * bp)
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

/* Decodes percent-encoded strings, then compares then with str_compare. */

EXPORT int str_compare_encoded (const char * ap, const char * bp)
{
    if (ap == NULL)
        return (bp == NULL) ? 0 : -1;
    if (bp == NULL)
        return 1;

    unsigned char a = * ap ++, b = * bp ++;
    for (; a || b; a = * ap ++, b = * bp ++)
    {
        if (a == '%' && ap[0] && ap[1])
        {
            a = (FROM_HEX (ap[0]) << 4) | FROM_HEX (ap[1]);
            ap += 2;
        }
        if (b == '%' && bp[0] && bp[1])
        {
            b = (FROM_HEX (bp[0]) << 4) | FROM_HEX (bp[1]);
            bp += 2;
        }

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

EXPORT Index<String> str_list_to_index (const char * list, const char * delims)
{
    char dmap[256] = {0};

    for (; * delims; delims ++)
        dmap[(unsigned char) (* delims)] = 1;

    Index<String> index;
    const char * word = NULL;

    for (; * list; list ++)
    {
        if (dmap[(unsigned char) (* list)])
        {
            if (word)
            {
                index.append (str_nget (word, list - word));
                word = NULL;
            }
        }
        else
        {
            if (! word)
            {
                word = list;
            }
        }
    }

    if (word)
        index.append (String (word));

    return index;
}

EXPORT String index_to_str_list (const Index<String> & index, const char * sep)
{
    int count = index.len ();
    int seplen = strlen (sep);
    int total = count ? seplen * (count - 1) : 0;
    int lengths[count];

    for (int i = 0; i < count; i ++)
    {
        lengths[i] = strlen (index[i]);
        total += lengths[i];
    }

    char buf[total + 1];
    int pos = 0;

    for (int i = 0; i < count; i ++)
    {
        if (i)
        {
            strcpy (buf + pos, sep);
            pos += seplen;
        }

        strcpy (buf + pos, index[i]);
        pos += lengths[i];
    }

    buf[pos] = 0;

    return String (buf);
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
 * Values between -1,000,000,000 and 1,000,000,000 (inclusive) are guaranteed to
 * have an accuracy of 6 decimal places.
 */

EXPORT int str_to_int (const char * string)
{
    bool_t neg = (string[0] == '-');
    if (neg)
        string ++;

    int val = 0;
    char c;

    while ((c = * string ++) && c >= '0' && c <= '9')
        val = val * 10 + (c - '0');

    return neg ? -val : val;
}

EXPORT double str_to_double (const char * string)
{
    bool_t neg = (string[0] == '-');
    if (neg)
        string ++;

    double val = str_to_int (string);
    const char * p = strchr (string, '.');

    if (p)
    {
        char buf[7] = "000000";
        const char * nul = (const char *) memchr (p + 1, 0, 6);
        memcpy (buf, p + 1, nul ? nul - (p + 1) : 6);
        val += (double) str_to_int (buf) / 1000000;
    }

    return neg ? -val : val;
}

EXPORT String int_to_str (int val)
{
    char buf[16];
    str_itoa (val, buf, sizeof buf);
    return String (buf);
}

EXPORT String double_to_str (double val)
{
    bool_t neg = (val < 0);
    if (neg)
        val = -val;

    int i = floor (val);
    int f = round ((val - i) * 1000000);

    if (f == 1000000)
    {
        i ++;
        f = 0;
    }

    SPRINTF (buf, "%s%d.%06d", neg ? "-" : "", i, f);

    char * c = buf + strlen (buf);
    while (* (c - 1) == '0')
        c --;
    if (* (c - 1) == '.')
        c --;
    * c = 0;

    return String (buf);
}

EXPORT bool_t str_to_int_array (const char * string, int * array, int count)
{
    Index<String> index = str_list_to_index (string, ", ");

    if (index.len () != count)
        return FALSE;

    for (int i = 0; i < count; i ++)
        array[i] = str_to_int (index[i]);

    return TRUE;
}

EXPORT String int_array_to_str (const int * array, int count)
{
    Index<String> index;

    for (int i = 0; i < count; i ++)
    {
        String value = int_to_str (array[i]);
        if (! value)
            return String ();

        index.append (value);
    }

    return index_to_str_list (index, ",");
}

EXPORT bool_t str_to_double_array (const char * string, double * array, int count)
{
    Index<String> index = str_list_to_index (string, ", ");

    if (index.len () != count)
        return FALSE;

    for (int i = 0; i < count; i ++)
        array[i] = str_to_double (index[i]);

    return TRUE;
}

EXPORT String double_array_to_str (const double * array, int count)
{
    Index<String> index;

    for (int i = 0; i < count; i ++)
    {
        String value = double_to_str (array[i]);
        if (! value)
            return String ();

        index.append (value);
    }

    return index_to_str_list (index, ",");
}

EXPORT void str_format_time (char * buf, int bufsize, int64_t milliseconds)
{
    int hours = milliseconds / 3600000;
    int minutes = (milliseconds / 60000) % 60;
    int seconds = (milliseconds / 1000) % 60;

    if (hours)
        snprintf (buf, bufsize, "%d:%02d:%02d", hours, minutes, seconds);
    else
    {
        bool_t zero = aud_get_bool (NULL, "leading_zero");
        snprintf (buf, bufsize, zero ? "%02d:%02d" : "%d:%02d", minutes, seconds);
    }
}
