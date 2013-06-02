/*
 * inifile.c
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

#include "inifile.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

static void strip_string (GString * string)
{
    int start = 0;
    while (start < string->len && string->str[start] == ' ')
        start ++;

    g_string_erase (string, 0, start);

    int end = string->len;
    while (end && string->str[end - 1] == ' ')
        end --;

    g_string_truncate (string, end);
}

EXPORT INIFile * inifile_read (VFSFile * file)
{
    void * buf = 0;
    int64_t bufsize = 0;

    vfs_file_read_all (file, & buf, & bufsize);
    if (! buf)
        return NULL;

    GHashTable * inifile = g_hash_table_new_full (g_str_hash, g_str_equal, free,
     (GDestroyNotify) g_hash_table_destroy);

    GHashTable * section = g_hash_table_new_full (g_str_hash, g_str_equal, free, free);
    g_hash_table_insert (inifile, strdup (""), section);

    const char * parse = buf;
    const char * end = buf + bufsize;

    while (parse < end)
    {
        char c = * parse ++;
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
            continue;

        if (c == '[')
        {
            GString * heading = g_string_new ("");

            while (parse < end)
            {
                c = * parse ++;
                if (c == ']' || c == '\r' || c == '\n')
                    break;

                g_string_append_c (heading, c);
            }

            if (c == ']')
            {
                strip_string (heading);
                g_string_ascii_down (heading);

                if (heading->len)
                {
                    section = g_hash_table_lookup (inifile, heading->str);

                    if (! section)
                    {
                        section = g_hash_table_new_full (g_str_hash, g_str_equal, free, free);
                        g_hash_table_insert (inifile, strdup (heading->str), section);
                    }
                }
            }

            g_string_free (heading, TRUE);
        }
        else
        {
            GString * key = g_string_new ("");
            GString * value = g_string_new ("");

            g_string_append_c (key, c);

            while (parse < end)
            {
                c = * parse ++;
                if (c == '=' || c == '\r' || c == '\n')
                    break;

                g_string_append_c (key, c);
            }

            if (c == '=')
            {
                while (parse < end)
                {
                    c = * parse ++;
                    if (c == '\r' || c == '\n')
                        break;

                    g_string_append_c (value, c);
                }

                strip_string (key);
                strip_string (value);
                g_string_ascii_down (key);

                if (key->len && value->len)
                    g_hash_table_insert (section, strdup (key->str), strdup (value->str));
            }

            g_string_free (key, TRUE);
            g_string_free (value, TRUE);
        }
    }

    free (buf);

    return (INIFile *) inifile;
}

EXPORT void inifile_destroy (INIFile * inifile)
{
    g_hash_table_destroy ((GHashTable *) inifile);
}

EXPORT const char * inifile_lookup (INIFile * inifile, const char * heading, const char * key)
{
    GHashTable * section = g_hash_table_lookup ((GHashTable *) inifile, heading);
    return section ? g_hash_table_lookup (section, key) : NULL;
}
