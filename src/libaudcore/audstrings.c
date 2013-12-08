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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <ctype.h>

#include <audacious/i18n.h>

#include "audstrings.h"
#include "index.h"

static const char ascii_to_hex[256] = {
    ['0'] = 0x0, ['1'] = 0x1, ['2'] = 0x2, ['3'] = 0x3, ['4'] = 0x4,
    ['5'] = 0x5, ['6'] = 0x6, ['7'] = 0x7, ['8'] = 0x8, ['9'] = 0x9,
    ['a'] = 0xa, ['b'] = 0xb, ['c'] = 0xc, ['d'] = 0xd, ['e'] = 0xe, ['f'] = 0xf,
    ['A'] = 0xa, ['B'] = 0xb, ['C'] = 0xc, ['D'] = 0xd, ['E'] = 0xe, ['F'] = 0xf
};

static const char hex_to_ascii[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const char uri_legal_table[128] = {
    ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1,
    ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1,
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, ['g'] = 1,
    ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1,
    ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1,
    ['v'] = 1, ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1,
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1,
    ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1,
    ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1,
    ['V'] = 1, ['W'] = 1, ['X'] = 1, ['Y'] = 1, ['Z'] = 1,
    ['-'] = 1, ['_'] = 1, ['.'] = 1, ['~'] = 1, ['/'] = 1
};

#define FROM_HEX(c)  (ascii_to_hex[(unsigned char) (c)])
#define TO_HEX(i)    (hex_to_ascii[(i) & 15])
#define IS_LEGAL(c)  (uri_legal_table[(unsigned char) (c)])

EXPORT char * str_printf (const char * format, ...)
{
    va_list args;
    va_start (args, format);

    char * str = str_vprintf (format, args);

    va_end (args);
    return str;
}

EXPORT char * str_vprintf (const char * format, va_list args)
{
    VSPRINTF (buf, format, args);
    return str_get (buf);
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

EXPORT void str_replace_char (char * string, char old_c, char new_c)
{
    while ((string = strchr (string, old_c)))
        * string ++ = new_c;
}

/* Percent-decodes up to <len> bytes of <str> to <out>, which must be large
 * enough to hold the decoded string (i.e., (len + 1) bytes).  If <len> is
 * negative, decodes all of <str>. */

EXPORT void str_decode_percent (const char * str, int len, char * out)
{
    if (len < 0)
        len = strlen (str);
    else
        len = strnlen (str, len);

    while (1)
    {
        const char * p = memchr (str, '%', len);
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
    if (len < 0)
        len = strlen (str);
    else
        len = strnlen (str, len);

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

/* Like g_filename_to_uri, but converts the filename from the system locale to
 * UTF-8 before percent-encoding.  On Windows, replaces '\' with '/' and adds a
 * leading '/'. */

EXPORT char * filename_to_uri (const char * name)
{
    char * utf8 = str_from_locale (name, -1);
    if (! utf8)
        return NULL;

#ifdef _WIN32
    str_replace_char (utf8, '\\', '/');
#endif
    char enc[3 * strlen (utf8) + 1];
    str_encode_percent (utf8, -1, enc);

    str_unref (utf8);

#ifdef _WIN32
    return str_printf ("file:///%s", enc);
#else
    return str_printf ("file://%s", enc);
#endif
}

/* Like g_filename_from_uri, but converts the filename from UTF-8 to the system
 * locale after percent-decoding.  On Windows, strips the leading '/' and
 * replaces '/' with '\'. */

EXPORT char * uri_to_filename (const char * uri)
{
#ifdef _WIN32
    g_return_val_if_fail (! strncmp (uri, "file:///", 8), NULL);
    char buf[strlen (uri + 8) + 1];
    str_decode_percent (uri + 8, -1, buf);
#else
    g_return_val_if_fail (! strncmp (uri, "file://", 7), NULL);
    char buf[strlen (uri + 7) + 1];
    str_decode_percent (uri + 7, -1, buf);
#endif
#ifdef _WIN32
    str_replace_char (buf, '/', '\\');
#endif

    return str_to_locale (buf, -1);
}

/* Formats a URI for human-readable display.  Percent-decodes and, for file://
 * URI's, converts to filename format, but in UTF-8. */

EXPORT char * uri_to_display (const char * uri)
{
    if (! strncmp (uri, "cdda://?", 8))
        return str_printf (_("Audio CD, track %s"), uri + 8);

    char buf[strlen (uri) + 1];

#ifdef _WIN32
    if (! strncmp (uri, "file:///", 8))
    {
        str_decode_percent (uri + 8, -1, buf);
        str_replace_char (buf, '/', '\\');
    }
#else
    if (! strncmp (uri, "file://", 7))
        str_decode_percent (uri + 7, -1, buf);
#endif
    else
        str_decode_percent (uri, -1, buf);

    return str_get (buf);
}

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

    char buf[sub - base + 1];
    memcpy (buf, base, sub - base);
    buf[sub - base] = 0;

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

    g_strlcpy (buf, ext + 1, buflen);

    /* remove subtunes and HTTP query strings */
    char * qmark;
    if ((qmark = strchr (buf, '?')))
        * qmark = 0;

    return (buf[0] != 0);
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

EXPORT Index * str_list_to_index (const char * list, const char * delims)
{
    Index * index = index_new ();
    const char * word = NULL;

    for (; * list; list ++)
    {
        if (strchr (delims, * list))
        {
            if (word)
            {
                index_insert (index, -1, str_nget (word, list - word));
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
        index_insert (index, -1, str_get (word));

    return index;
}

EXPORT char * index_to_str_list (Index * index, const char * sep)
{
    int count = index_count (index);
    int seplen = strlen (sep);
    int total = count ? seplen * (count - 1) : 0;
    int lengths[count];

    for (int i = 0; i < count; i ++)
    {
        lengths[i] = strlen (index_get (index, i));
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

        strcpy (buf + pos, index_get (index, i));
        pos += lengths[i];
    }

    buf[pos] = 0;

    return str_get (buf);
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

EXPORT bool_t str_to_int (const char * string, int * addr)
{
    bool_t neg = (string[0] == '-');
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

EXPORT bool_t str_to_double (const char * string, double * addr)
{
    bool_t neg = (string[0] == '-');
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

        if (! str_to_int (buf, & i))
            goto ERR;

        len = strlen (p + 1);
        if (len > 6)
            goto ERR;

        memcpy (buf, p + 1, len);
        memset (buf + len, '0', 6 - len);
        buf[6] = 0;

        if (! str_to_int (buf, & f))
            goto ERR;
    }
    else
    {
        if (! str_to_int (string, & i))
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

EXPORT char * int_to_str (int val)
{
    g_return_val_if_fail (val >= -1000000000 && val <= 1000000000, NULL);
    return str_printf ("%d", val);
}

EXPORT char * double_to_str (double val)
{
    g_return_val_if_fail (val >= -1000000000 && val <= 1000000000, NULL);

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

    return str_get (buf);
}

EXPORT bool_t str_to_int_array (const char * string, int * array, int count)
{
    Index * index = str_list_to_index (string, ", ");
    if (index_count (index) != count)
        goto ERR;

    for (int i = 0; i < count; i ++)
    {
        if (! str_to_int (index_get (index, i), & array[i]))
            goto ERR;
    }

    index_free_full (index, (IndexFreeFunc) str_unref);
    return TRUE;

ERR:
    index_free_full (index, (IndexFreeFunc) str_unref);
    return FALSE;
}

EXPORT char * int_array_to_str (const int * array, int count)
{
    Index * index = index_new ();

    for (int i = 0; i < count; i ++)
    {
        char * value = int_to_str (array[i]);
        if (! value)
            goto ERR;

        index_insert (index, -1, value);
    }

    char * string = index_to_str_list (index, ",");
    index_free_full (index, (IndexFreeFunc) str_unref);
    return string;

ERR:
    index_free_full (index, (IndexFreeFunc) str_unref);
    return NULL;
}

EXPORT bool_t str_to_double_array (const char * string, double * array, int count)
{
    Index * index = str_list_to_index (string, ", ");
    if (index_count (index) != count)
        goto ERR;

    for (int i = 0; i < count; i ++)
    {
        if (! str_to_double (index_get (index, i), & array[i]))
            goto ERR;
    }

    index_free_full (index, (IndexFreeFunc) str_unref);
    return TRUE;

ERR:
    index_free_full (index, (IndexFreeFunc) str_unref);
    return FALSE;
}

EXPORT char * double_array_to_str (const double * array, int count)
{
    Index * index = index_new ();

    for (int i = 0; i < count; i ++)
    {
        char * value = double_to_str (array[i]);
        if (! value)
            goto ERR;

        index_insert (index, -1, value);
    }

    char * string = index_to_str_list (index, ",");
    index_free_full (index, (IndexFreeFunc) str_unref);
    return string;

ERR:
    index_free_full (index, (IndexFreeFunc) str_unref);
    return NULL;
}
