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

#include <glib.h>

#ifdef _WIN32

#include <windows.h>

void get_argv_utf8 (int * argc, char * * * argv)
{
    wchar_t * combined = GetCommandLineW ();
    wchar_t * * split = CommandLineToArgvW (combined, argc);

    * argv = g_new (char *, argc + 1);

    for (int i = 0; i < * argc; i ++)
        (* argv)[i] = g_utf16_to_utf8 (split[i], -1, NULL, NULL, NULL);

    (* argv)[* argc] = 0;

    LocalFree (split);
}

void free_argv_utf8 (int * argc, char * * * argv)
{
    g_strfreev (* argv);
    * argc = 0;
    * argv = NULL;
}

#endif
