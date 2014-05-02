/*
 * charset.c
 * Copyright 2013 John Lindgren
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
#include "internal.h"

#include <iconv.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#ifdef USE_CHARDET
extern "C" {
#include <libguess/libguess.h>
}
#endif

#include "hook.h"
#include "index.h"
#include "runtime.h"
#include "tinylock.h"

EXPORT String str_convert (const char * str, int len, const char * from_charset,
 const char * to_charset)
{
    iconv_t conv = iconv_open (to_charset, from_charset);
    if (conv == (iconv_t) -1)
        return String ();

    if (len < 0)
        len = strlen (str);

    // liberal estimate of how much space we will need
    // are there obscure cases that require even more?
    int maxlen = 4 * len;

    char buf[maxlen + 1];
    String result;

    size_t inbytes = len;
    size_t outbytes = maxlen;
    char * in = (char *) str;
    char * out = buf;

    if (iconv (conv, & in, & inbytes, & out, & outbytes) != (size_t) -1 && ! inbytes)
    {
        buf[maxlen - outbytes] = 0;
        result = String (buf);
    }

    iconv_close (conv);
    return result;
}

static void whine_locale (const char * str, int len, const char * dir, const char * charset)
{
    if (len < 0)
        fprintf (stderr, "Cannot convert %s locale (%s): %s\n", dir, charset, str);
    else
        fprintf (stderr, "Cannot convert %s locale (%s): %.*s\n", dir, charset, len, str);
}

EXPORT String str_from_locale (const char * str, int len)
{
    const char * charset;

    if (g_get_charset (& charset))
    {
        /* locale is UTF-8 */
        if (! g_utf8_validate (str, len, NULL))
        {
            whine_locale (str, len, "from", "UTF-8");
            return String ();
        }

        return (len < 0) ? String (str) : str_nget (str, len);
    }
    else
    {
        String utf8 = str_convert (str, len, charset, "UTF-8");
        if (! utf8)
            whine_locale (str, len, "from", charset);

        return utf8;
    }
}

EXPORT String str_to_locale (const char * str, int len)
{
    const char * charset;

    if (g_get_charset (& charset))
    {
        /* locale is UTF-8 */
        return (len < 0) ? String (str) : str_nget (str, len);
    }
    else
    {
        String local = str_convert (str, len, "UTF-8", charset);
        if (! local)
            whine_locale (str, len, "to", charset);

        return local;
    }
}

static TinyRWLock settings_lock;
static String detect_region;
static Index<String> fallback_charsets;

static void set_charsets (const char * region, const char * fallbacks)
{
    tiny_lock_write (& settings_lock);

    detect_region = String (region);

#ifdef USE_CHARDET
    if (detect_region)
        libguess_init ();
#endif

    if (fallbacks)
        fallback_charsets = str_list_to_index (fallbacks, ", ");
    else
        fallback_charsets.clear ();

    tiny_unlock_write (& settings_lock);
}

EXPORT String str_to_utf8 (const char * str, int len)
{
    /* check whether already UTF-8 */
    if (g_utf8_validate (str, len, NULL))
        return (len < 0) ? String (str) : str_nget (str, len);

    if (len < 0)
        len = strlen (str);

    String utf8;
    tiny_lock_read (& settings_lock);

#ifdef USE_CHARDET
    if (detect_region)
    {
        /* prefer libguess-detected charset */
        const char * detected = libguess_determine_encoding (str, len, detect_region);
        if (detected)
        {
            utf8 = str_convert (str, len, detected, "UTF-8");
            if (utf8)
                goto DONE;
        }
    }
#endif

    /* try user-configured fallbacks */
    for (const String & fallback : fallback_charsets)
    {
        utf8 = str_convert (str, len, fallback, "UTF-8");
        if (utf8)
            goto DONE;
    }

    /* try system locale last (this one will print a warning if it fails) */
    utf8 = str_from_locale (str, len);

DONE:
    tiny_unlock_read (& settings_lock);
    return utf8;
}

static void chardet_update (void)
{
    String region = aud_get_str (NULL, "chardet_detector");
    String fallbacks = aud_get_str (NULL, "chardet_fallback");

    set_charsets (region[0] ? (const char *) region : NULL, fallbacks);
}

void chardet_init (void)
{
    chardet_update ();

    hook_associate ("set chardet_detector", (HookFunction) chardet_update, NULL);
    hook_associate ("set chardet_fallback", (HookFunction) chardet_update, NULL);
}

void chardet_cleanup (void)
{
    hook_dissociate ("set chardet_detector", (HookFunction) chardet_update);
    hook_dissociate ("set chardet_fallback", (HookFunction) chardet_update);

    set_charsets (NULL, NULL);
}
