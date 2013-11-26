/*
 * chardet.c
 * Copyright 2006-2011 Yoshiki Yazawa, Matti Hämäläinen, and John Lindgren
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

#include <glib.h>
#include <string.h>
#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "main.h"
#include "misc.h"

static char * cd_chardet_to_utf8 (const char * str, int len,
 int * arg_bytes_read, int * arg_bytes_written);

static char * str_to_utf8_fallback (const char * str)
{
    char * out = g_strconcat (str, _("  (invalid UTF-8)"), NULL);

    for (char * c = out; * c; c ++)
    {
        if (* c & 0x80)
            * c = '?';
    }

    return out;
}

static char * cd_str_to_utf8 (const char * str)
{
    char *out_str;

    if (str == NULL)
        return NULL;

    /* chardet encoding detector */
    if ((out_str = cd_chardet_to_utf8 (str, strlen (str), NULL, NULL)))
        return out_str;

    /* all else fails, we mask off character codes >= 128, replace with '?' */
    return str_to_utf8_fallback(str);
}

static char * cd_chardet_to_utf8 (const char * str, int len,
 int * arg_bytes_read, int * arg_bytes_write)
{
    char *ret = NULL;
    int * bytes_read, * bytes_write;
    int my_bytes_read, my_bytes_write;

    bytes_read = arg_bytes_read != NULL ? arg_bytes_read : &my_bytes_read;
    bytes_write = arg_bytes_write != NULL ? arg_bytes_write : &my_bytes_write;

    g_return_val_if_fail(str != NULL, NULL);

    if (g_utf8_validate(str, len, NULL))
    {
        if (len < 0)
            len = strlen (str);

        ret = g_malloc (len + 1);
        memcpy (ret, str, len);
        ret[len] = 0;

        if (arg_bytes_read != NULL)
            * arg_bytes_read = len;
        if (arg_bytes_write != NULL)
            * arg_bytes_write = len;

        return ret;
    }

    /* If detection failed or was not enabled, try fallbacks (if there are any) */
    if (! ret)
    {
        char * fallbacks = get_string (NULL, "chardet_fallback");
        char * * split = g_strsplit_set (fallbacks, " ,:;|/", -1);

        for (char * * enc = split; * enc; enc ++)
        {
            gsize read_gsize = 0, written_gsize = 0;
            ret = g_convert (str, len, "UTF-8", * enc, & read_gsize, & written_gsize, NULL);
            * bytes_read = read_gsize;
            * bytes_write = written_gsize;

            if (len == *bytes_read)
                break;
            else {
                g_free(ret);
                ret = NULL;
            }
        }

        g_strfreev (split);
        g_free (fallbacks);
    }

    /* First fallback: locale (duh!) */
    if (ret == NULL)
    {
        gsize read_gsize = 0, written_gsize = 0;
        ret = g_locale_to_utf8 (str, len, & read_gsize, & written_gsize, NULL);
        * bytes_read = read_gsize;
        * bytes_write = written_gsize;
    }

    /* The final fallback is ISO-8859-1, if no other is specified or conversions fail */
    if (ret == NULL)
    {
        gsize read_gsize = 0, written_gsize = 0;
        ret = g_convert (str, len, "UTF-8", "ISO-8859-1", & read_gsize, & written_gsize, NULL);
        * bytes_read = read_gsize;
        * bytes_write = written_gsize;
    }

    return ret;
}

void chardet_init (void)
{
    str_set_utf8_impl (cd_str_to_utf8, cd_chardet_to_utf8);
}
