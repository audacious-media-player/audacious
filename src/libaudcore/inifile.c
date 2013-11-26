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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAXLINE 2000

static char * strskip (char * str)
{
    while (isspace (* str))
        str ++;

    return str;
}

static char * strtrim (char * str)
{
    int len = strlen (str);

    while (len && isspace(str[len - 1]))
        str[-- len] = 0;

    return str;
}

EXPORT void inifile_parse (VFSFile * file,
 void (* handle_heading) (const char * heading, void * data),
 void (* handle_entry) (const char * key, const char * value, void * data),
 void * data)
{
    char buf[MAXLINE + 1];
    char * pos = buf;
    int len = 0;
    bool_t eof = FALSE;

    while (1)
    {
        char * newline = memchr (pos, '\n', len);

        if (! newline && len < MAXLINE && ! eof)
        {
            memmove (buf, pos, len);
            pos = buf;

            len += vfs_fread (buf + len, 1, MAXLINE - len, file);

            if (len < MAXLINE)
                eof = TRUE;

            newline = memchr (pos, '\n', len);
        }

        if (newline)
            * newline = 0;
        else
            pos[len] = 0;

        char * start = strskip (pos);

        switch (* start)
        {
        case 0:
        case '#':
        case ';':
            break;

        case '[':;
            char * end = strchr (start + 1, ']');

            if (! end)
            {
                fprintf (stderr, "While parsing %s:\nMalformed heading: %s\n",
                 vfs_get_filename (file), start);
                break;
            }

            * end = 0;
            handle_heading (strtrim (strskip (start + 1)), data);
            break;

        default:;
            char * sep = strchr (start, '=');

            if (! sep)
            {
                fprintf (stderr, "While parsing %s:\nMalformed entry: %s\n",
                 vfs_get_filename (file), start);
                break;
            }

            * sep = 0;
            handle_entry (strtrim (start), strtrim (strskip (sep + 1)), data);
            break;
        }

        if (! newline)
            break;

        len -= newline + 1 - pos;
        pos = newline + 1;
    }
}

EXPORT bool_t inifile_write_heading (VFSFile * file, const char * heading)
{
    int len = strlen (heading);
    char buf[len + 4];

    buf[0] = '\n';
    buf[1] = '[';
    strcpy (buf + 2, heading);
    buf[len + 2] = ']';
    buf[len + 3] = '\n';

    return (vfs_fwrite (buf, 1, sizeof buf, file) == sizeof buf);
}

EXPORT bool_t inifile_write_entry (VFSFile * file, const char * key, const char * value)
{
    int len1 = strlen (key);
    int len2 = strlen (value);
    char buf[len1 + len2 + 2];

    strcpy (buf, key);
    buf[len1] = '=';
    strcpy (buf + len1 + 1, value);
    buf[len1 + len2 + 1] = '\n';

    return (vfs_fwrite (buf, 1, sizeof buf, file) == sizeof buf);
}
