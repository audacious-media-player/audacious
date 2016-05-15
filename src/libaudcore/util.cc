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

#include "internal.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "runtime.h"

const char * get_home_utf8 ()
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    static char * home_utf8;

    auto init = [] ()
        { home_utf8 = g_filename_to_utf8 (g_get_home_dir (), -1, nullptr, nullptr, nullptr); };

    pthread_once (& once, init);
    return home_utf8;
}

bool dir_foreach (const char * path, DirForeachFunc func, void * user)
{
    GDir * dir = g_dir_open (path, 0, nullptr);
    if (! dir)
        return false;

    const char * name;
    while ((name = g_dir_read_name (dir)))
    {
        if (func (filename_build ({path, name}), name, user))
            break;
    }

    g_dir_close (dir);
    return true;
}

String write_temp_file (const void * data, int64_t len)
{
    StringBuf name = filename_build ({g_get_tmp_dir (), "audacious-temp-XXXXXX"});

    int handle = g_mkstemp (name);
    if (handle < 0)
    {
        AUDERR ("Error creating temporary file: %s\n", strerror (errno));
        return String ();
    }

    while (len)
    {
        int64_t written = write (handle, data, len);
        if (written < 0)
        {
            AUDERR ("Error writing %s: %s\n", (const char *) name, strerror (errno));
            close (handle);
            return String ();
        }

        data = (char *) data + written;
        len -= written;
    }

    if (close (handle) < 0)
    {
        AUDERR ("Error closing %s: %s\n", (const char *) name, strerror (errno));
        return String ();
    }

    return String (name);
}

bool same_basename (const char * a, const char * b)
{
    const char * dot_a = strrchr (a, '.');
    const char * dot_b = strrchr (b, '.');
    int len_a = dot_a ? dot_a - a : strlen (a);
    int len_b = dot_b ? dot_b - b : strlen (b);

    return len_a == len_b && ! strcmp_nocase (a, b, len_a);
}

const char * last_path_element (const char * path)
{
    const char * slash = strrchr (path, G_DIR_SEPARATOR);
    return (slash && slash[1]) ? slash + 1 : nullptr;
}

void cut_path_element (char * path, int pos)
{
#ifdef _WIN32
    if (pos > 3)
#else
    if (pos > 1)
#endif
        path[pos - 1] = 0; /* overwrite slash */
    else
        path[pos] = 0; /* leave [drive letter and] leading slash */
}

bool is_cuesheet_entry (const char * filename)
{
    const char * ext, * sub;
    uri_parse (filename, nullptr, & ext, & sub, nullptr);
    return sub[0] && sub - ext == 4 && ! strcmp_nocase (ext, ".cue", 4);
}

bool is_subtune (const char * filename)
{
    const char * sub;
    uri_parse (filename, nullptr, nullptr, & sub, nullptr);
    return sub[0];
}

StringBuf strip_subtune (const char * filename)
{
    const char * sub;
    uri_parse (filename, nullptr, nullptr, & sub, nullptr);
    return str_copy (filename, sub - filename);
}

/* Thomas Wang's 32-bit mix function.  See:
 * http://web.archive.org/web/20070307172248/http://www.concentric.net/~Ttwang/tech/inthash.htm */

unsigned int32_hash (unsigned val)
{
    val = ~val + (val << 15);
    val = val ^ (val >> 12);
    val = val + (val << 2);
    val = val ^ (val >> 4);
    val = val * 2057;
    val = val ^ (val >> 16);
    return val;
}

unsigned ptr_hash (const void * ptr)
{
    unsigned addr_low = (uint64_t) (uintptr_t) ptr;
    unsigned addr_high = (uint64_t) (uintptr_t) ptr >> 32;
    return int32_hash (addr_low + addr_high);
}
