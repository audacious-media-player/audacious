/*
 * history.c
 * Copyright 2011 John Lindgren
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
#include "runtime.h"

#include <string.h>

#define MAX_ENTRIES 30

EXPORT String aud_history_get (int entry)
{
    StringBuf name = str_printf ("entry%d", entry);
    String path = aud_get_str ("history", name);
    return (path[0] ? path : String ());
}

EXPORT void aud_history_add (const char * path)
{
    String add = String (path);

    for (int i = 0; i < MAX_ENTRIES; i ++)
    {
        StringBuf name = str_printf ("entry%d", i);
        String old = aud_get_str ("history", name);
        aud_set_str ("history", name, add);

        if (! strcmp (old, path))
            break;

        add = old;
    }
}
