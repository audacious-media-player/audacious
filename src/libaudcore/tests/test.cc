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

#include "tuple.h"
#include "tuple-compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* stubs */
bool aud_get_bool (const char *, const char *)
    { return false; }
String aud_get_str (const char *, const char *)
    { return String (""); }
String vfs_get_metadata (VFSFile *, const char *)
    { return String (); }

static void test_tuple_format (const char * format, const Tuple & tuple, const char * expected)
{
    TupleCompiler compiler;
    compiler.compile (format);
    String result = compiler.evaluate (tuple);

    if (strcmp (result, expected))
    {
        printf ("For format [%s]\n", format);
        printf ("Expected [%s]\n", expected);
        printf ("Got [%s]\n", (const char *) result);
        exit (1);
    }
}

static void test_tuple_formats (void)
{
    Tuple tuple;

    /* fallback tests */
    test_tuple_format ("", tuple, "");
    tuple.set_filename ("http://Path%20To/File%20Name");
    test_tuple_format ("", tuple, "File Name");
    tuple.set_str (FIELD_TITLE, "Song Title");
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
    tuple.set_int (FIELD_YEAR, -1);
    test_tuple_format ("${year}", tuple, "-1");
    tuple.set_int (FIELD_YEAR, 0);
    test_tuple_format ("${year}", tuple, "0");
    tuple.set_int (FIELD_YEAR, 1990);
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
    tuple.set_str (FIELD_ARTIST, "{}");
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
    tuple.set_int (FIELD_YEAR, 0);
    tuple.set_str (FIELD_ARTIST, "");
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

int main (void)
{
    test_tuple_formats ();

    return 0;
}
