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

#include <errno.h>
#include <iconv.h>
#include <string.h>

#include <new>

#include <glib.h>

#include "libguess/libguess.h"

#include "hook.h"
#include "index.h"
#include "runtime.h"
#include "threads.h"

EXPORT StringBuf str_convert(const char * str, int len,
                             const char * from_charset, const char * to_charset)
{
    iconv_t conv = iconv_open(to_charset, from_charset);
    if (conv == (iconv_t)-1)
        return StringBuf();

    if (len < 0)
        len = strlen(str);

    StringBuf buf(-1);

    size_t inbytesleft = len;
    size_t outbytesleft = buf.len();
    ICONV_CONST char * in = (ICONV_CONST char *)str;
    char * out = buf;

    errno = 0;
    size_t ret = iconv(conv, &in, &inbytesleft, &out, &outbytesleft);

    if (ret == (size_t)-1 && errno == E2BIG)
        throw std::bad_alloc();

    iconv_close(conv);

    if (ret == (size_t)-1 || inbytesleft)
        return StringBuf();

    buf.resize(buf.len() - outbytesleft);
    return buf;
}

static void whine_locale(const char * str, int len, const char * dir,
                         const char * charset)
{
    if (len < 0)
        AUDWARN("Cannot convert %s locale (%s): %s\n", dir, charset, str);
    else
        AUDWARN("Cannot convert %s locale (%s): %.*s\n", dir, charset, len,
                str);
}

EXPORT StringBuf str_from_locale(const char * str, int len)
{
    const char * charset;

    if (g_get_charset(&charset))
    {
        /* locale is UTF-8 */
        if (!g_utf8_validate(str, len, nullptr))
        {
            whine_locale(str, len, "from", "UTF-8");
            return StringBuf();
        }

        return str_copy(str, len);
    }
    else
    {
        StringBuf utf8 = str_convert(str, len, charset, "UTF-8");
        if (!utf8)
            whine_locale(str, len, "from", charset);

        return utf8;
    }
}

EXPORT StringBuf str_to_locale(const char * str, int len)
{
    const char * charset;

    if (g_get_charset(&charset))
    {
        /* locale is UTF-8 */
        return str_copy(str, len);
    }
    else
    {
        StringBuf local = str_convert(str, len, "UTF-8", charset);
        if (!local)
            whine_locale(str, len, "to", charset);

        return local;
    }
}

static aud::spinlock_rw settings_lock;
static String detect_region;
static Index<String> fallback_charsets;

static void set_charsets(const char * region, const char * fallbacks)
{
    auto wr = settings_lock.write();

    detect_region = String(region);

    if (fallbacks)
        fallback_charsets = str_list_to_index(fallbacks, ", ");
    else
        fallback_charsets.clear();
}

static StringBuf convert_to_utf8(const char * str, int len)
{
    auto rd = settings_lock.read();

    if (len < 0)
        len = strlen(str);

    if (detect_region)
    {
        /* prefer libguess-detected charset */
        const char * detected =
            libguess_determine_encoding(str, len, detect_region);
        if (detected)
        {
            StringBuf utf8 = str_convert(str, len, detected, "UTF-8");
            if (utf8)
                return utf8;
        }
    }

    /* try user-configured fallbacks */
    for (const String & fallback : fallback_charsets)
    {
        StringBuf utf8 = str_convert(str, len, fallback, "UTF-8");
        if (utf8)
            return utf8;
    }

    /* try system locale last (this one will print a warning if it fails) */
    return str_from_locale(str, len);
}

EXPORT StringBuf str_to_utf8(const char * str, int len)
{
    /* check whether already UTF-8 */
    if (g_utf8_validate(str, len, nullptr))
        return str_copy(str, len);

    return convert_to_utf8(str, len);
}

EXPORT StringBuf str_to_utf8(StringBuf && str)
{
    /* check whether already UTF-8 */
    if (g_utf8_validate(str, str.len(), nullptr))
        return std::move(str);

    str = convert_to_utf8(str, str.len());
    return str.settle();
}

static int utf8_get_char_byte(const char * string, int bytes)
{
    int checklen = -1;

    if ((string[0] & 0x80) == 0x00)
    {
        checklen = 1;
    }
    else if ((string[0] & 0xE0) == 0xC0)
    {
        checklen = 2;
    }
    else if ((string[0] & 0xF0) == 0xE0)
    {
        checklen = 3;
    }
    else if ((string[0] & 0xF8) == 0xF0)
    {
        checklen = 4;
    }
    else if ((string[0] & 0xFC) == 0xF8)
    {
        checklen = 5;
    }
    else if ((string[0] & 0xFE) == 0xFC)
    {
        checklen = 6;
    }
    else
    {
        return -1;
    }

    if (bytes < checklen)
    {
        return -1;
    }

    for (int i = 1; i < checklen; ++i)
    {
        if ((string[i] & 0xC0) != 0x80)
        {
            return -1;
        }
    }

    return checklen;
}

EXPORT int utf8_str_bytes_len(const char * string, int chars)
{
    int total_bytes_len = strlen(string);
    int result = 0;
    int current_chars = 0;

    while ((result < total_bytes_len) && ((chars == -1) || (current_chars < chars)))
    {
        int count = utf8_get_char_byte(string + result, total_bytes_len - result);
        if (count < 0)
        {
            AUDWARN("Encountered invalid utf-8 sequence at string %s\n", string);
            return -1;
        }

        result += count;
        ++current_chars;
    }

    return result;
}

EXPORT int utf8_str_chars_len(const char * string, int bytes)
{
    int result = 0;
    int processed_bytes = 0;

    if (bytes < 0)
    {
        bytes = strlen(string);
    }

    while (processed_bytes < bytes)
    {
        int count = utf8_get_char_byte(string + processed_bytes, bytes - processed_bytes);
        if (count < 0)
        {
            AUDWARN("Encountered invalid utf-8 sequence at string %s\n", string);
            return -1;
        }

        processed_bytes += count;
        ++result;
    }

    return result;
}

static void chardet_update(void * = nullptr, void * = nullptr)
{
    String region = aud_get_str("chardet_detector");
    String fallbacks = aud_get_str("chardet_fallback");

    set_charsets(region[0] ? (const char *)region : nullptr, fallbacks);
}

void chardet_init()
{
    chardet_update();

    hook_associate("set chardet_detector", chardet_update, nullptr);
    hook_associate("set chardet_fallback", chardet_update, nullptr);
}

void chardet_cleanup()
{
    hook_dissociate("set chardet_detector", chardet_update);
    hook_dissociate("set chardet_fallback", chardet_update);

    set_charsets(nullptr, nullptr);
}
