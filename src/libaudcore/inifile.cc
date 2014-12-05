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

#include <string.h>

#include <glib.h>  /* for g_ascii_isspace */

#include "audstrings.h"
#include "vfs.h"

static char * strskip (char * str, char * end)
{
    while (str < end && g_ascii_isspace (* str))
        str ++;

    return str;
}

static char * strtrim (char * str, char * end)
{
    while (end > str && g_ascii_isspace (end[-1]))
        end --;

    * end = 0;
    return str;
}

EXPORT void IniParser::parse (VFSFile & file)
{
    int size = 512;
    StringBuf buf (size);

    char * pos = buf;
    int len = 0;
    bool eof = false;

    while (1)
    {
        char * newline = (char *) memchr (pos, '\n', len);

        while (! newline && ! eof)
        {
            memmove (buf, pos, len);
            pos = buf;

            if (len >= size - 1)
            {
                size <<= 1;
                buf.resize (size);
                pos = buf;
            }

            len += file.fread (buf + len, 1, size - 1 - len);

            if (len < size - 1)
                eof = true;

            newline = (char *) memchr (pos, '\n', len);
        }

        char * end = newline ? newline : pos + len;
        char * start = strskip (pos, end);
        char * sep;

        if (start < end)
        {
            switch (* start)
            {
            case '#':
            case ';':
                break;

            case '[':
                if ((end = (char *) memchr (start, ']', end - start)))
                    handle_heading (strtrim (strskip (start + 1, end), end));

                break;

            default:
                if ((sep = (char *) memchr (start, '=', end - start)))
                    handle_entry (strtrim (start, sep), strtrim (strskip (sep + 1, end), end));

                break;
            }
        }

        if (! newline)
            break;

        len -= newline + 1 - pos;
        pos = newline + 1;
    }
}

EXPORT bool inifile_write_heading (VFSFile & file, const char * heading)
{
    StringBuf line = str_concat ({"\n[", heading, "]\n"});
    return (file.fwrite (line, 1, line.len ()) == line.len ());
}

EXPORT bool inifile_write_entry (VFSFile & file, const char * key, const char * value)
{
    StringBuf line = str_concat ({key, "=", value, "\n"});
    return (file.fwrite (line, 1, line.len ()) == line.len ());
}
