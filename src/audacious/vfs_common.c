/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
