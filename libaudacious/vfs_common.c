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

/* FIXME low performance vfs_getc */
gint vfs_getc(VFSFile *stream)
{
    guchar uc;
    if (vfs_fread(&uc, 1, 1, stream))
	return uc;
    return EOF;
}


gint vfs_fputc(gint c, VFSFile *stream)
{
    guchar uc = (guchar) c;

    if (! vfs_fwrite(&uc, 1, 1, stream)) {
        return EOF;
    }

    return uc;
}

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

int vfs_fputs(const gchar *s, VFSFile *stream)
{
	size_t n = strlen(s);

	return ((vfs_fwrite(s, 1, n, stream) == n) ? n : EOF);
}

int vfs_vfprintf(VFSFile *stream, gchar const *format, va_list args)
{
    gchar *string;
    gint rv = g_vasprintf(&string, format, args);
    if (rv<0) return rv;
    rv = vfs_fputs(string, stream);
    free (string);
    return rv;
}

int vfs_fprintf(VFSFile *stream, gchar const *format, ...)
{
    va_list arg;
    gint rv;

    va_start(arg, format);
    rv = vfs_vfprintf(stream, format, arg);
    va_end(arg);

    return rv;
}
