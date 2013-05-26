/*
 * audstrings.c
 * Copyright 2009-2011 John Lindgren
 * Copyright 2010 William Pitcock
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
#include <locale.h>

#include <audacious/i18n.h>

#include "audstrings.h"

#define FROM_HEX(c) ((c) < 'A' ? (c) - '0' : (c) < 'a' ? 10 + (c) - 'A' : 10 + (c) - 'a')
#define TO_HEX(i) ((i) < 10 ? '0' + (i) : 'A' + (i) - 10)
#define IS_LEGAL(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') \
                  || ((c) >= '0' && (c) <= '9') || (strchr ("-_.~/", (c))))

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

static char * (* str_to_utf8_impl) (const char *) = NULL;
static char * (* str_to_utf8_full_impl) (const char *, int, int *, int *) = NULL;

EXPORT void str_set_utf8_impl (char * (* stu_impl) (const char *),
 char * (* stuf_impl) (const char *, int, int *, int *))
{
    str_to_utf8_impl = stu_impl;
    str_to_utf8_full_impl = stuf_impl;
}

EXPORT char * str_to_utf8 (const char * str)
{
    g_return_val_if_fail (str_to_utf8_impl, NULL);
    return str_to_utf8_impl (str);
}

EXPORT char * str_to_utf8_full (const char * str, int len, int * bytes_read, int * bytes_written)
{
    g_return_val_if_fail (str_to_utf8_full_impl, NULL);
    return str_to_utf8_full_impl (str, len, bytes_read, bytes_written);
}

EXPORT void string_replace_char (char * string, char old_c, char new_c)
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
        len = INT_MAX;

    while (len --)
    {
        char c = * str ++;
        if (! c)
            break;

        if (c == '%' && len >= 2 && str[0] && str[1])
        {
            c = (FROM_HEX (str[0]) << 4) | FROM_HEX (str[1]);
            str += 2;
            len -= 2;
        }

        * out ++ = c;
    }

    * out = 0;
}

/* Percent-encodes up to <len> bytes of <str> to <out>, which must be large
 * enough to hold the encoded string (i.e., (3 * len + 1) bytes).  If <len> is
 * negative, decodes all of <str>. */

EXPORT void str_encode_percent (const char * str, int len, char * out)
{
    if (len < 0)
        len = INT_MAX;

    while (len --)
    {
        char c = * str ++;
        if (! c)
            break;

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
    char * utf8 = g_locale_to_utf8 (name, -1, NULL, NULL, NULL);
    if (! utf8)
    {
        const char * locale = setlocale (LC_ALL, NULL);
        fprintf (stderr, "Cannot convert filename from system locale (%s): %s\n", locale, name);
        return NULL;
    }

#ifdef _WIN32
    string_replace_char (utf8, '\\', '/');
#endif
    char enc[3 * strlen (utf8) + 1];
    str_encode_percent (utf8, -1, enc);

    g_free (utf8);

#ifdef _WIN32
    return g_strdup_printf ("file:///%s", enc);
#else
    return g_strdup_printf ("file://%s", enc);
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
    string_replace_char (buf, '/', '\\');
#endif

    char * name = g_locale_from_utf8 (buf, -1, NULL, NULL, NULL);
    if (! name)
    {
        const char * locale = setlocale (LC_ALL, NULL);
        fprintf (stderr, "Cannot convert filename to system locale (%s): %s\n", locale, buf);
    }

    return name;
}

/* Formats a URI for human-readable display.  Percent-decodes and, for file://
 * URI's, converts to filename format, but in UTF-8. */

EXPORT char * uri_to_display (const char * uri)
{
    if (! strncmp (uri, "cdda://?", 8))
        return g_strdup_printf (_("Audio CD, track %s"), uri + 8);

    char buf[strlen (uri) + 1];

#ifdef _WIN32
    if (! strncmp (uri, "file:///", 8))
    {
        str_decode_percent (uri + 8, -1, buf);
        string_replace_char (buf, '/', '\\');
    }
#else
    if (! strncmp (uri, "file://", 7))
        str_decode_percent (uri + 7, -1, buf);
#endif
    else
        str_decode_percent (uri, -1, buf);

    return g_strdup (buf);
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

EXPORT int string_compare (const char * ap, const char * bp)
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

EXPORT int string_compare_encoded (const char * ap, const char * bp)
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

EXPORT char *
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

EXPORT bool_t string_to_int (const char * string, int * addr)
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

EXPORT bool_t string_to_double (const char * string, double * addr)
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

EXPORT char * int_to_string (int val)
{
    g_return_val_if_fail (val >= -1000000000 && val <= 1000000000, NULL);
    return g_strdup_printf ("%d", val);
}

EXPORT char * double_to_string (double val)
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

    char * s = neg ? g_strdup_printf ("-%d.%06d", i, f) : g_strdup_printf ("%d.%06d", i, f);

    char * c = s + strlen (s);
    while (* (c - 1) == '0')
        c --;
    if (* (c - 1) == '.')
        c --;
    * c = 0;

    return s;
}

EXPORT bool_t string_to_int_array (const char * string, int * array, int count)
{
    char * * split = g_strsplit (string, ",", -1);
    if (g_strv_length (split) != count)
        goto ERR;

    for (int i = 0; i < count; i ++)
    {
        if (! string_to_int (split[i], & array[i]))
            goto ERR;
    }

    g_strfreev (split);
    return TRUE;

ERR:
    g_strfreev (split);
    return FALSE;
}

EXPORT char * int_array_to_string (const int * array, int count)
{
    char * * split = g_malloc0 (sizeof (char *) * (count + 1));

    for (int i = 0; i < count; i ++)
    {
        split[i] = int_to_string (array[i]);
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

EXPORT bool_t string_to_double_array (const char * string, double * array, int count)
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

EXPORT char * double_array_to_string (const double * array, int count)
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
