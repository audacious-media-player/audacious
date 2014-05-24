/*
 * util.c
 * Copyright 2009-2013 John Lindgren and Michał Lipski
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

#include "util.h"

#ifdef _WIN32
#include <windows.h>

#ifdef WORDS_BIGENDIAN
#define UTF16_NATIVE "UTF-16BE"
#else
#define UTF16_NATIVE "UTF-16LE"
#endif

Index<String> get_argv_utf8 ()
{
    int argc;
    wchar_t * combined = GetCommandLineW ();
    wchar_t * * split = CommandLineToArgvW (combined, & argc);

    Index<String> argv;
    argv.insert (0, argc);

    for (int i = 0; i < argc; i ++)
        argv[i] = String (str_convert ((char *) split[i],
         wcslen (split[i]) * sizeof (wchar_t), UTF16_NATIVE, "UTF-8"));

    LocalFree (split);
    return argv;
}

#endif
