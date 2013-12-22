/*
 *  vfs_common.c
 *  Copyright 2006-2013 Tony Vroon, William Pitcock, Matti Hämäläinen, and
 *                      John Lindgren
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

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "audstrings.h"
#include "vfs.h"

/**
 * @file vfs_common.c
 * Common code for various VFS-stream related operations.
 * Routines for string reading and writing and functions for
 * reading and writing endianess-dependant integer values.
 */


/**
 * Writes a character to a stream.
 *
 * @param c A character to write to the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return The character on success, or EOF.
 */
EXPORT int vfs_fputc(int c, VFSFile *stream)
{
    unsigned char uc = (unsigned char) c;

    if (!vfs_fwrite(&uc, 1, 1, stream)) {
        return EOF;
    }

    return uc;
}

/**
 * Reads a string of characters from a stream, ending in newline or EOF.
 *
 * @param s A buffer to put the string in.
 * @param n The amount of characters to read.
 * @param stream A #VFSFile object representing the stream.
 * @return The string on success, or NULL.
 */
EXPORT char *vfs_fgets(char *s, int n, VFSFile *stream)
{
    int c;
    register char *p;

    if (n <= 0) return NULL;

    p = s;

    while (--n) {
        if ((c = vfs_getc(stream))== EOF) {
            break;
        }
        if ((*p++ = c) == '\n') {
            break;
        }
    }
    if (p > s) {
        *p = 0;
        return s;
    }

    return NULL;
}

/**
 * Writes a string to a VFS stream.
 *
 * @param s A string to write to the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return The amount of bytes written.
 */
EXPORT int vfs_fputs(const char *s, VFSFile *stream)
{
    int64_t n = strlen(s);

    return ((vfs_fwrite(s, 1, n, stream) == n) ? n : EOF);
}

/**
 * Writes a formatted string to a VFS stream via a va_list of args.
 *
 * @param stream A #VFSFile object representing the stream.
 * @param format A printf-style format string.
 * @param args A va_list of args to use.
 * @return value The amount of bytes written.
 */
EXPORT int vfs_vfprintf(VFSFile *stream, char const *format, va_list args)
{
    VSPRINTF (buf, format, args);
    return vfs_fputs (buf, stream);
}

/**
 * Writes a formatted string to a VFS stream.
 *
 * @param stream A #VFSFile object representing the stream.
 * @param format A printf-style format string.
 * @param ... Optional list of arguments.
 * @return The amount of bytes written.
 */
EXPORT int vfs_fprintf(VFSFile *stream, char const *format, ...)
{
    va_list arg;
    int rv;

    va_start(arg, format);
    rv = vfs_vfprintf(stream, format, arg);
    va_end(arg);

    return rv;
}

EXPORT void vfs_file_read_all (VFSFile * file, void * * bufp, int64_t * sizep)
{
    char * buf = NULL;
    int64_t size = vfs_fsize (file);

    if (size >= 0)
    {
        size = MIN (size, SIZE_MAX - 1);
        buf = g_malloc (size + 1);
        size = vfs_fread (buf, 1, size, file);
    }
    else
    {
        size = 0;

        size_t bufsize = 4096;
        buf = g_malloc (bufsize);

        size_t readsize;
        while ((readsize = vfs_fread (buf + size, 1, bufsize - 1 - size, file)))
        {
            size += readsize;

            if (size == bufsize - 1)
            {
                if (bufsize > SIZE_MAX - 4096)
                    break;

                bufsize += 4096;
                buf = g_realloc (buf, bufsize);
            }
        }
    }

    buf[size] = 0; // nul-terminate

    * bufp = buf;
    if (sizep)
        * sizep = size;
}

/**
 * Gets contents of the file into a buffer. Buffer of filesize bytes
 * is allocated by this function as necessary.
 *
 * @param filename URI of the file to read in.
 * @param buf Pointer to a pointer variable of buffer.
 * @param size Pointer to gsize variable that will hold the amount of
 * read data e.g. filesize.
 */
EXPORT void vfs_file_get_contents (const char * filename, void * * buf, int64_t * size)
{
    * buf = NULL;
    if (size)
        * size = 0;

    VFSFile * file = vfs_fopen (filename, "r");
    if (! file)
        return;

    vfs_file_read_all (file, buf, size);
    vfs_fclose (file);
}
