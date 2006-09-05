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

/* FIXME low performance vfs_getc */
gint vfs_getc(VFSFile *stream)
{
    guchar uc;
    if (vfs_fread(&uc, 1, 1, stream))
	return uc;
    return EOF;
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
