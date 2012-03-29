/*
 *  vfs_common.c
 *  Copyright 2006-2010 Tony Vroon, William Pitcock, Maciej Grela,
 *                      Matti Hämäläinen, and John Lindgren
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

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
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
    gsize n = strlen(s);

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
    char *string;
    int rv = g_vasprintf(&string, format, args);
    if (rv < 0) return rv;
    rv = vfs_fputs(string, stream);
    g_free(string);
    return rv;
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
    * size = 0;

    VFSFile *fd;
    gsize filled_size = 0, buf_size = 4096;
    unsigned char * ptr;

    if ((fd = vfs_fopen(filename, "rb")) == NULL)
        return;

    if ((* size = vfs_fsize (fd)) >= 0)
    {
        * buf = g_malloc (* size);
        * size = vfs_fread (* buf, 1, * size, fd);
        goto close_handle;
    }

    if ((*buf = g_malloc(buf_size)) == NULL)
        goto close_handle;

    ptr = *buf;
    while (TRUE) {
        gsize read_size = vfs_fread(ptr, 1, buf_size - filled_size, fd);
        if (read_size == 0) break;

        filled_size += read_size;
        ptr += read_size;

        if (filled_size == buf_size) {
            buf_size += 4096;

            *buf = g_realloc(*buf, buf_size);

            if (*buf == NULL)
                goto close_handle;

            ptr = (unsigned char *) (* buf) + filled_size;
        }
    }

    *size = filled_size;

close_handle:
    vfs_fclose(fd);
}


/**
 * Reads an unsigned 16-bit Little Endian value from the stream
 * into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_le16(uint16_t *value, VFSFile *stream)
{
    uint16_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT16_FROM_LE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 32-bit Little Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_le32(uint32_t *value, VFSFile *stream)
{
    uint32_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT32_FROM_LE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 64-bit Little Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_le64(uint64_t *value, VFSFile *stream)
{
    uint64_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT64_FROM_LE(tmp);
    return TRUE;
}


/**
 * Reads an unsigned 16-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_be16(uint16_t *value, VFSFile *stream)
{
    uint16_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT16_FROM_BE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 32-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_be32(uint32_t *value, VFSFile *stream)
{
    uint32_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT32_FROM_BE(tmp);
    return TRUE;
}

/**
 * Reads an unsigned 64-bit Big Endian value from the stream into native endian format.
 *
 * @param value Pointer to the variable to read the value into.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fget_be64(uint64_t *value, VFSFile *stream)
{
    uint64_t tmp;
    if (vfs_fread(&tmp, sizeof(tmp), 1, stream) != 1)
        return FALSE;
    *value = GUINT64_FROM_BE(tmp);
    return TRUE;
}

/**
 * Writes an unsigned 16-bit native endian value into the stream as a
 * Little Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_le16(uint16_t value, VFSFile *stream)
{
    uint16_t tmp = GUINT16_TO_LE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}

/**
 * Writes an unsigned 32-bit native endian value into the stream as a
 * Big Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_le32(uint32_t value, VFSFile *stream)
{
    uint32_t tmp = GUINT32_TO_LE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}

/**
 * Writes an unsigned 64-bit native endian value into the stream as a
 * Big Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_le64(uint64_t value, VFSFile *stream)
{
    uint64_t tmp = GUINT64_TO_LE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}

/**
 * Writes an unsigned 16-bit native endian value into the stream as a
 * Big Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_be16(uint16_t value, VFSFile *stream)
{
    uint16_t tmp = GUINT16_TO_BE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}

/**
 * Writes an unsigned 32-bit native endian value into the stream as a
 * Big Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_be32(uint32_t value, VFSFile *stream)
{
    uint32_t tmp = GUINT32_TO_BE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}

/**
 * Writes an unsigned 64-bit native endian value into the stream as a
 * Big Endian value.
 *
 * @param value Value to write into the stream.
 * @param stream A #VFSFile object representing the stream.
 * @return TRUE if read was succesful, FALSE if there was an error.
 */
EXPORT bool_t vfs_fput_be64(uint64_t value, VFSFile *stream)
{
    uint64_t tmp = GUINT64_TO_BE(value);
    return vfs_fwrite(&tmp, sizeof(tmp), 1, stream) == 1;
}
