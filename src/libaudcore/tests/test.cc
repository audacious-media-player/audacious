/*
 * test.cc - Various tests for libaudcore
 * Copyright 2014 John Lindgren
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

#include "audio.h"
#include "audstrings.h"
#include "internal.h"
#include "ringbuf.h"
#include "tuple.h"
#include "tuple-compiler.h"
#include "vfs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_audio_conversion ()
{
    /* single precision float should be lossless for 24-bit audio */
    /* also test that high byte is correctly ignored/zeroed */
    static const int32_t in[10] =
     {0x800000, 0x800001, 0x800002, -2, -1, 0, 1, 2, 0x7ffffe, 0x7fffff};

    float f[10];
    char packed[30];
    int32_t out[10];

    audio_from_int (in, FMT_S24_NE, f, 10);

    for (int format = FMT_S24_3LE; format <= FMT_U24_3BE; format ++)
    {
        memset (packed, 0, sizeof packed);
        audio_to_int (f, packed, format, 10);
        memset (f, 0, sizeof f);
        audio_from_int (packed, format, f, 10);
    }

    audio_to_int (f, out, FMT_S24_NE, 10);

    assert (f[0] == -1.0f);
    assert (f[5] == 0.0f);

    for (int i = 0; i < 10; i ++)
        assert (out[i] == (in[i] & 0xffffff));
}

static void test_case_conversion ()
{
    const char in[]        = "AÄaäEÊeêIÌiìOÕoõUÚuú";
    const char low_ascii[] = "aÄaäeÊeêiÌiìoÕoõuÚuú";
    const char low_utf8[]  = "aäaäeêeêiìiìoõoõuúuú";
    const char hi_ascii[]  = "AÄAäEÊEêIÌIìOÕOõUÚUú";
    const char hi_utf8[]   = "AÄAÄEÊEÊIÌIÌOÕOÕUÚUÚ";

    assert (! strcmp (low_ascii, str_tolower (in)));
    assert (! strcmp (low_utf8, str_tolower_utf8 (in)));
    assert (! strcmp (hi_ascii, str_toupper (in)));
    assert (! strcmp (hi_utf8, str_toupper_utf8 (in)));

    assert (! strcmp_safe ("abc", "abc"));
    assert (! strcmp_safe ("abc", "abcdef", 3));
    assert (strcmp_safe ("abc", "def") < 0);
    assert (strcmp_safe ("def", "abc") > 0);
    assert (! strcmp_safe (nullptr, nullptr));
    assert (strcmp_safe (nullptr, "abc") < 0);
    assert (strcmp_safe ("abc", nullptr) > 0);

    assert (! strcmp_nocase ("abc", "ABC"));
    assert (! strcmp_nocase ("ABC", "abcdef", 3));
    assert (strcmp_nocase ("abc", "DEF") < 0);
    assert (strcmp_nocase ("ABC", "def") < 0);
    assert (strcmp_nocase ("def", "ABC") > 0);
    assert (strcmp_nocase ("DEF", "abc") > 0);
    assert (! strcmp_nocase (nullptr, nullptr));
    assert (strcmp_nocase (nullptr, "abc") < 0);
    assert (strcmp_nocase ("abc", nullptr) > 0);

    assert (! strcmp_nocase (in, low_ascii));
    assert (strcmp_nocase (in, low_utf8));
    assert (! strcmp_nocase (in, hi_ascii));
    assert (strcmp_nocase (in, hi_utf8));

    assert (str_has_prefix_nocase (low_ascii, "AÄaä"));
    assert (! str_has_prefix_nocase (low_utf8, "AÄaä"));
    assert (str_has_prefix_nocase (hi_ascii, "AÄaä"));
    assert (! str_has_prefix_nocase (hi_utf8, "AÄaä"));

    assert (str_has_suffix_nocase (low_ascii, "UÚuú"));
    assert (! str_has_suffix_nocase (low_utf8, "UÚuú"));
    assert (str_has_suffix_nocase (hi_ascii, "UÚuú"));
    assert (! str_has_suffix_nocase (hi_utf8, "UÚuú"));

    assert (! str_has_suffix_nocase ("abc", "abcd"));

    assert (! strcmp (strstr_nocase (low_ascii, "OÕoõ"), "oÕoõuÚuú"));
    assert (strstr_nocase (low_utf8, "OÕoõ") == nullptr);
    assert (! strcmp (strstr_nocase (hi_ascii, "OÕoõ"), "OÕOõUÚUú"));
    assert (strstr_nocase (hi_utf8, "OÕoõ") == nullptr);

    assert (! strcmp (strstr_nocase_utf8 (low_ascii, "OÕoõ"), "oÕoõuÚuú"));
    assert (! strcmp (strstr_nocase_utf8 (low_utf8, "OÕoõ"), "oõoõuúuú"));
    assert (strstr_nocase_utf8 (low_utf8, "OOoo") == nullptr);
    assert (! strcmp (strstr_nocase_utf8 (hi_ascii, "OÕoõ"), "OÕOõUÚUú"));
    assert (! strcmp (strstr_nocase_utf8 (hi_utf8, "OÕoõ"), "OÕOÕUÚUÚ"));
    assert (strstr_nocase_utf8 (hi_utf8, "OOoo") == nullptr);
}

static void test_numeric_conversion ()
{
    static const char * in[] = {
        "",
        "x1234",
        "+2147483647",
        "-2147483648",
        "999999999.999999",
        "999999999.9999996",
        "000000000000000000000000100000.000001000000000000000000000000",
        "--5",
        "3.+5",
        "-6.7 dB"
    };

    static const char * out_double[] = {
        "0",
        "0",
        "2147483647",
        "-2147483648",
        "999999999.999999",
        "1000000000",
        "100000.000001",
        "0",
        "3",
        "-6.7"
    };

    static const char * out_int[] = {
        "0",
        "0",
        "2147483647",
        "-2147483648",
        "999999999",
        "999999999",
        "100000",
        "0",
        "3",
        "-6"
    };

    for (int i = 0; i < aud::n_elems (in); i ++)
    {
        double d_val = str_to_double (in[i]);
        int i_val = str_to_int (in[i]);
        StringBuf via_double = double_to_str (d_val);
        StringBuf via_int = int_to_str (i_val);

        if (strcmp (via_double, out_double[i]) || strcmp (via_int, out_int[i]))
        {
            printf ("Converting [%s]\n", in[i]);
            printf ("Expected [%s] and [%s]\n", out_double[i], out_int[i]);
            printf ("Via [%g] and [%d]\n", d_val, i_val);
            printf ("Got [%s] and [%s]\n", (const char *) via_double, (const char *) via_int);
            exit (1);
        }
    }
}

static void test_filename_split ()
{
    /* expected results differ slightly from POSIX dirname/basename */
    static const char * const paths[][3] = {
        {"/usr/lib/aud", "/usr/lib", "aud"},
        {"/usr/lib/", "/usr", "lib"},
        {"/usr/lib", "/usr", "lib"},
        {"/usr/", "/", "usr"},
        {"/usr", "/", "usr"},
        {"/", nullptr, "/"}
    };

    for (int i = 0; i < aud::n_elems (paths); i ++)
    {
        assert (! strcmp_safe (filename_get_parent (paths[i][0]), paths[i][1]));
        assert (! strcmp_safe (filename_get_base (paths[i][0]), paths[i][2]));
    }
}

static void test_tuple_format (const char * format, Tuple & tuple, const char * expected)
{
    TupleCompiler compiler;
    compiler.compile (format);
    compiler.format (tuple);

    String result = tuple.get_str (Tuple::FormattedTitle);
    if (strcmp (result, expected))
    {
        printf ("For format [%s]\n", format);
        printf ("Expected [%s]\n", expected);
        printf ("Got [%s]\n", (const char *) result);
        exit (1);
    }
}

static void test_tuple_formats ()
{
    Tuple tuple;

    /* fallback tests */
    test_tuple_format ("", tuple, "");
    tuple.set_filename ("http://Path%20To/File%20Name");
    test_tuple_format ("", tuple, "File Name");
    tuple.set_str (Tuple::Title, "Song Title");
    test_tuple_format ("", tuple, "Song Title");

    /* basic variable tests */
    test_tuple_format ("$", tuple, "Song Title");
    test_tuple_format ("${", tuple, "Song Title");
    test_tuple_format ("${file-name", tuple, "Song Title");
    test_tuple_format ("${file-name}", tuple, "File Name");
    test_tuple_format ("${file-name}}", tuple, "Song Title");
    test_tuple_format ("${invalid}", tuple, "Song Title");
    test_tuple_format ("${}", tuple, "Song Title");
    test_tuple_format ("\\$\\{\\}", tuple, "${}");
    test_tuple_format ("\\\0" "a", tuple, "Song Title");
    test_tuple_format ("{}", tuple, "Song Title");

    /* integer variable tests */
    test_tuple_format ("${year}", tuple, "Song Title");
    tuple.set_int (Tuple::Year, -1);
    test_tuple_format ("${year}", tuple, "-1");
    tuple.set_int (Tuple::Year, 0);
    test_tuple_format ("${year}", tuple, "0");
    tuple.set_int (Tuple::Year, 1990);
    test_tuple_format ("${year}", tuple, "1990");

    /* filename variable tests */
    test_tuple_format ("${file-path}", tuple, "http://Path To/");
    test_tuple_format ("${file-ext}", tuple, "Song Title");
    tuple.set_filename ("http://Path%20To/File%20Name.Ext?3");
    test_tuple_format ("${file-name}", tuple, "File Name");
    test_tuple_format ("${file-ext}", tuple, "Ext");
    test_tuple_format ("${subsong-id}", tuple, "3");

    /* existence tests */
    test_tuple_format ("x${?invalid:Field Exists}", tuple, "Song Title");
    test_tuple_format ("x${?subsong-id:Field Exists", tuple, "Song Title");
    test_tuple_format ("x${?subsong-id:Field Exists}", tuple, "xField Exists");
    test_tuple_format ("x${?subsong-id:${invalid}}", tuple, "Song Title");
    test_tuple_format ("x${?subsong-id:(${subsong-id})}", tuple, "x(3)");
    test_tuple_format ("x${?track-number:Field Exists}", tuple, "x");
    test_tuple_format ("x${?title:Field Exists}", tuple, "xField Exists");
    test_tuple_format ("x${?artist:Field Exists}", tuple, "x");
    test_tuple_format ("x${?artist}", tuple, "Song Title");

    /* equality tests */
    test_tuple_format ("x${=}", tuple, "Song Title");
    test_tuple_format ("x${==}", tuple, "Song Title");
    test_tuple_format ("x${==a,}", tuple, "Song Title");
    test_tuple_format ("x${==a,a:}", tuple, "Song Title");
    test_tuple_format ("x${==\"a\",a:}", tuple, "Song Title");
    test_tuple_format ("x${==\"a\",\"a:Equal}", tuple, "Song Title");
    test_tuple_format ("x${==\"a\",\"a\":Equal}", tuple, "xEqual");
    test_tuple_format ("x${==\"a\",\"a\"\":Equal}", tuple, "Song Title");
    test_tuple_format ("x${==\"a\",\"b\":Equal}", tuple, "x");
    test_tuple_format ("x${==year,\"a\":Equal}", tuple, "x");
    test_tuple_format ("x${==\"a\",year:Equal}", tuple, "x");
    test_tuple_format ("x${==year,1990:Equal}", tuple, "xEqual");
    test_tuple_format ("x${==1990,year:Equal}", tuple, "xEqual");
    test_tuple_format ("x${==title,\"a\":Equal}", tuple, "x");
    test_tuple_format ("x${==\"a\",title:Equal}", tuple, "x");
    test_tuple_format ("x${==title,\"Song Title\":Equal}", tuple, "xEqual");
    test_tuple_format ("x${==\"Song Title\",title:Equal}", tuple, "xEqual");
    tuple.set_str (Tuple::Artist, "{}");
    test_tuple_format ("x${==artist,\"\\{\\}\":Equal}", tuple, "xEqual");

    /* inequality tests */
    test_tuple_format ("x${!}", tuple, "Song Title");
    test_tuple_format ("x${!=}", tuple, "Song Title");
    test_tuple_format ("x${!=\"a\",\"a\":Unequal}", tuple, "x");
    test_tuple_format ("x${!=\"a\",\"b\":Unequal}", tuple, "xUnequal");
    test_tuple_format ("x${!=year,\"a\":Unequal}", tuple, "xUnequal");
    test_tuple_format ("x${!=\"a\",year:Unequal}", tuple, "xUnequal");
    test_tuple_format ("x${!=year,1990:Unequal}", tuple, "x");
    test_tuple_format ("x${!=1990,year:Unequal}", tuple, "x");
    test_tuple_format ("x${>}", tuple, "Song Title");
    test_tuple_format ("x${>year,1989:Greater}", tuple, "xGreater");
    test_tuple_format ("x${>year,1990:Greater}", tuple, "x");
    test_tuple_format ("x${>=year,1990:NotLess}", tuple, "xNotLess");
    test_tuple_format ("x${>=year,1991:NotLess}", tuple, "x");
    test_tuple_format ("x${<}", tuple, "Song Title");
    test_tuple_format ("x${<year,1991:Less}", tuple, "xLess");
    test_tuple_format ("x${<year,1990:Less}", tuple, "x");
    test_tuple_format ("x${<=year,1990:NotGreater}", tuple, "xNotGreater");
    test_tuple_format ("x${<=year,1989:NotGreater}", tuple, "x");

    /* emptiness tests */
    tuple.set_int (Tuple::Year, 0);
    tuple.set_str (Tuple::Artist, "");
    test_tuple_format ("x${(invalid)}", tuple, "Song Title");
    test_tuple_format ("x${(empty)?invalid:Empty}", tuple, "Song Title");
    test_tuple_format ("x${(empty)?subsong-id:Empty}", tuple, "x");
    test_tuple_format ("x${(empty)?subsong-id:${invalid}}", tuple, "Song Title");
    test_tuple_format ("x${(empty)?year:Empty}", tuple, "x");
    test_tuple_format ("x${(empty)?track-number:Empty}", tuple, "xEmpty");
    test_tuple_format ("x${(empty)?title:Empty}", tuple, "x");
    test_tuple_format ("x${(empty)?artist:Empty}", tuple, "x");
    test_tuple_format ("x${(empty)?album:Empty}", tuple, "xEmpty");
    test_tuple_format ("x${(empty)?\"Literal\":Empty}", tuple, "Song Title");
}

static void test_ringbuf ()
{
    String nums[10];
    for (int i = 0; i < 10; i ++)
        nums[i] = String (int_to_str (i));

    RingBuf<String> ring;

    ring.alloc (7);

    for (int i = 0; i < 7; i ++)
        assert (ring.push (nums[i]) == nums[i]);

    for (int i = 0; i < 5; i ++)
    {
        assert (ring.head () == nums[i]);
        ring.pop ();
    }

    for (int i = 7; i < 10; i ++)
        assert (ring.push (nums[i]) == nums[i]);

    assert (ring.size () == 7);
    assert (ring.len () == 5);
    assert (ring.linear () == 2);
    assert (ring.space () == 2);

    ring.alloc (5);

    for (int i = 0; i < 5; i ++)
        assert (ring[i] == nums[5 + i]);

    assert (ring.size () == 5);
    assert (ring.len () == 5);
    assert (ring.linear () == 2);
    assert (ring.space () == 0);

    ring.alloc (10);

    for (int i = 0; i < 5; i ++)
        assert (ring[i] == nums[5 + i]);

    assert (ring.size () == 10);
    assert (ring.len () == 5);
    assert (ring.linear () == 2);
    assert (ring.space () == 5);

    for (int i = 0; i < 5; i ++)
        assert (ring[i] == nums[5 + i]);

    for (int i = 5; i --; )
        assert (ring.push (nums[i]) == nums[i]);

    for (int i = 0; i < 5; i ++)
    {
        assert (ring.head () == nums[5 + i]);
        ring.pop ();
    }

    for (int i = 0; i < 5; i ++)
    {
        assert (ring.head () == nums[4 - i]);
        ring.pop ();
    }

    ring.copy_in (& nums[5], 5);
    ring.copy_in (& nums[0], 5);

    for (int i = 0; i < 5; i ++)
    {
        assert (ring.head () == nums[5 + i]);
        ring.pop ();
    }

    for (int i = 0; i < 5; i ++)
    {
        assert (ring.head () == nums[i]);
        ring.pop ();
    }

    ring.move_in (nums, 10);

    for (int i = 0; i < 10; i ++)
    {
        assert (! nums[i]);
        assert (ring[i] == String (int_to_str (i)));
    }

    ring.move_out (& nums[5], 5);
    ring.move_out (& nums[0], 5);

    for (int i = 0; i < 10; i ++)
        assert (nums[i] == String (int_to_str ((5 + i) % 10)));

    ring.move_in (nums, 10);

    Index<String> index;
    ring.move_out (index, -1, 5);

    assert (ring.len () == 5);
    assert (index.len () == 5);

    ring.move_out (index, 0, -1);

    assert (ring.len () == 0);
    assert (index.len () == 10);

    for (int i = 0; i < 10; i ++)
        assert (index[i] == String (int_to_str (i)));

    ring.move_in (index, 5, 5);

    assert (ring.len () == 5);
    assert (index.len () == 5);

    ring.move_in (index, 0, -1);

    assert (ring.len () == 10);
    assert (index.len () == 0);

    for (int i = 0; i < 10; i ++)
        assert (ring[i] == String (int_to_str ((5 + i) % 10)));

    ring.discard (5);
    assert (ring.len () == 5);

    ring.discard ();
    assert (ring.len () == 0);

    string_leak_check ();
}

int main ()
{
    test_audio_conversion ();
    test_case_conversion ();
    test_numeric_conversion ();
    test_filename_split ();
    test_tuple_formats ();
    test_ringbuf ();

    return 0;
}
