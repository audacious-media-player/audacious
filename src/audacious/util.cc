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

#include <new>
#include <string>

#ifdef WORDS_BIGENDIAN
#define UTF16_NATIVE "UTF-16BE"
#else
#define UTF16_NATIVE "UTF-16LE"
#endif

static wchar_t ** get_argvw(int * argc)
{
    wchar_t * cmdline = GetCommandLineW();
    wchar_t ** argvw = CommandLineToArgvW(cmdline, argc);
    if (!argvw)
        throw std::bad_alloc();

    return argvw;
}

Index<String> get_argv_utf8()
{
    int argc = 0;
    auto argvw = get_argvw(&argc);

    Index<String> argv;
    argv.insert(0, argc);

    for (int i = 0; i < argc; i++)
        argv[i] = String(str_convert((char *)argvw[i],
                                     wcslen(argvw[i]) * sizeof(wchar_t),
                                     UTF16_NATIVE, "UTF-8"));

    LocalFree(argvw);
    return argv;
}

int exec_argv0()
{
    int argc = 0;
    auto argvw = get_argvw(&argc);

    std::wstring quoted = L"\"";
    quoted.append(argvw[0]);
    quoted.append(L"\"");

    _wexeclp(argvw[0], quoted.c_str(), (wchar_t*)NULL);

    /* should not get here */
    LocalFree(argvw);
    return -1;
}

#endif
