/*
 * audacious: Cross-platform multimedia player.
 * vfs_common.c: Common VFS functions.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "vfs.h"
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>

/**
 * vfs_fputc:
 * @c: A character to write to the stream.
 * @stream: A #VFSFile object representing the stream.
 *
 * Writes a character to a stream.
 *
 * Return value: The character on success, or EOF.
 **/
gint vfs_fputc(gint c, VFSFile *stream)
{
    guchar uc = (guchar) c;

    if (! vfs_fwrite(&uc, 1, 1, stream)) {
        return EOF;
    }

    return uc;
}

/**
 * vfs_fgets:
 * @s: A buffer to put the string in.
 * @n: The amount of characters to read.
 * @stream: A #VFSFile object representing the stream.
 *
 * Reads a set of characters from a stream.
 *
 * Return value: The string on success, or NULL.
 **/
gchar *vfs_fgets(gchar *s, gint n, VFSFile *stream)
{
    gint c;
    register gchar *p;

    if(n<=0) return NULL;

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
 * vfs_fputc:
 * @s: A string to write to the stream.
 * @stream: A #VFSFile object representing the stream.
 *
 * Writes a string to a VFS stream.
 *
 * Return value: The amount of bytes written.
 **/
int vfs_fputs(const gchar *s, VFSFile *stream)
{
	size_t n = strlen(s);

	return ((vfs_fwrite(s, 1, n, stream) == n) ? n : EOF);
}

/**
 * vfs_vfprintf:
 * @stream: A #VFSFile object representing the stream.
 * @format: A printf-style format string.
 * @args: A va_list of args to use.
 *
 * Writes a formatted string to a VFS stream via a va_list of args.
 *
 * Return value: The amount of bytes written.
 **/
int vfs_vfprintf(VFSFile *stream, gchar const *format, va_list args)
{
    gchar *string;
    gint rv = g_vasprintf(&string, format, args);
    if (rv<0) return rv;
    rv = vfs_fputs(string, stream);
    free (string);
    return rv;
}

/**
 * vfs_fprintf:
 * @stream: A #VFSFile object representing the stream.
 * @format: A printf-style format string.
 * @...: A list of args to use.
 *
 * Writes a formatted string to a VFS stream.
 *
 * Return value: The amount of bytes written.
 **/
int vfs_fprintf(VFSFile *stream, gchar const *format, ...)
{
    va_list arg;
    gint rv;

    va_start(arg, format);
    rv = vfs_vfprintf(stream, format, arg);
    va_end(arg);

    return rv;
}

/**
 * vfs_file_get_contents
 * @filename: the filename to read in
 * @buf: pointer to pointer of buffer
 * @sz: pointer to integer that is the size
 **/
void
vfs_file_get_contents(const gchar *filename, gchar **buf, gsize *size)
{
    VFSFile *fd;

    fd = vfs_fopen(filename, "rb");

    if (fd == NULL)
        return;

    vfs_fseek(fd, 0, SEEK_END);
    *size = vfs_ftell(fd);

    *buf = g_new(gchar, *size);

    if (*buf == NULL)
        return;

    vfs_fseek(fd, 0, SEEK_SET);
    vfs_fread(*buf, 1, *size, fd);

    vfs_fclose(fd);
}
