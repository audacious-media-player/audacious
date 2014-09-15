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

#include <string.h>

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
 * @return The character on success, or -1.
 */
EXPORT int vfs_fputc(int c, VFSFile *stream)
{
    unsigned char uc = (unsigned char) c;

    if (!vfs_fwrite(&uc, 1, 1, stream)) {
        return -1;
    }

    return uc;
}

/**
 * Reads a string of characters from a stream, ending in newline or EOF.
 *
 * @param s A buffer to put the string in.
 * @param n The amount of characters to read.
 * @param stream A #VFSFile object representing the stream.
 * @return The string on success, or nullptr.
 */
EXPORT char *vfs_fgets(char *s, int n, VFSFile *stream)
{
    int c;
    char *p;

    if (n <= 0) return nullptr;

    p = s;

    while (--n) {
        if ((c = vfs_getc(stream)) < 0)
            break;
        if ((*p++ = c) == '\n')
            break;
    }
    if (p > s) {
        *p = 0;
        return s;
    }

    return nullptr;
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

    return ((vfs_fwrite(s, 1, n, stream) == n) ? n : -1);
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
    return vfs_fputs (str_vprintf (format, args), stream);
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

EXPORT Index<char> vfs_file_read_all (VFSFile * file)
{
    constexpr int maxbuf = 16777216;
    constexpr int pagesize = 4096;

    Index<char> buf;
    int64_t size = file->fsize ();

    if (size >= 0)
    {
        buf.insert (0, aud::min (size, (int64_t) maxbuf));
        size = file->fread (buf.begin (), 1, buf.len ());
    }
    else
    {
        size = 0;

        buf.insert (0, pagesize);

        int64_t readsize;
        while ((readsize = file->fread (& buf[size], 1, buf.len () - size)))
        {
            size += readsize;

            if (size == buf.len ())
            {
                if (buf.len () > maxbuf - pagesize)
                    break;

                buf.insert (-1, pagesize);
            }
        }
    }

    buf.remove (size, -1);

    return buf;
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
EXPORT Index<char> vfs_file_get_contents(const char * filename)
{
    Index<char> buf;
    VFSFile * file = VFSFile::fopen (filename, "r");

    if (file)
        buf = vfs_file_read_all (file);

    delete file;
    return buf;
}
