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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "vfs.h"
#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

VFSFile *
stdio_vfs_fopen_impl(const gchar * path,
          const gchar * mode)
{
    VFSFile *file;

    if (!path || !mode)
	return NULL;

    file = g_new(VFSFile, 1);

    file->handle = fopen(path, mode);

    if (file->handle == NULL) {
        g_free(file);
        file = NULL;
    }

    return file;
}

gint
stdio_vfs_fclose_impl(VFSFile * file)
{
    gint ret = 0;

    if (file == NULL)
        return -1;

    if (file->handle) {
        if (fclose(file->handle) != 0)
            ret = -1;
    }

    return ret;
}

size_t
stdio_vfs_fread_impl(gpointer ptr,
          size_t size,
          size_t nmemb,
          VFSFile * file)
{
    if (file == NULL)
        return 0;

    return fread(ptr, size, nmemb, file->handle);
}

size_t
stdio_vfs_fwrite_impl(gconstpointer ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    if (file == NULL)
        return 0;

    return fwrite(ptr, size, nmemb, file->handle);
}

gint
stdio_vfs_getc_impl(VFSFile *stream)
{
  return getc( stream->handle );
}

gint
stdio_vfs_ungetc_impl(gint c, VFSFile *stream)
{
  return ungetc( c , stream->handle );
}

gint
stdio_vfs_fseek_impl(VFSFile * file,
          glong offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return fseek(file->handle, offset, whence);
}

void
stdio_vfs_rewind_impl(VFSFile * file)
{
    if (file == NULL)
        return;

    rewind(file->handle);
}

glong
stdio_vfs_ftell_impl(VFSFile * file)
{
    if (file == NULL)
        return 0;

    return ftell(file->handle);
}

gboolean
stdio_vfs_feof_impl(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) feof(file->handle);
}

gint
stdio_vfs_truncate_impl(VFSFile * file, glong size)
{
    if (file == NULL)
        return -1;

    return ftruncate(fileno(file->handle), size);
}

VFSConstructor file_const = {
	"file://",
	stdio_vfs_fopen_impl,
	stdio_vfs_fclose_impl,
	stdio_vfs_fread_impl,
	stdio_vfs_fwrite_impl,
	stdio_vfs_getc_impl,
	stdio_vfs_ungetc_impl,
	stdio_vfs_fseek_impl,
	stdio_vfs_rewind_impl,
	stdio_vfs_ftell_impl,
	stdio_vfs_feof_impl,
	stdio_vfs_truncate_impl
};

VFSConstructor default_const = {
	"/",
	stdio_vfs_fopen_impl,
	stdio_vfs_fclose_impl,
	stdio_vfs_fread_impl,
	stdio_vfs_fwrite_impl,
	stdio_vfs_getc_impl,
	stdio_vfs_ungetc_impl,
	stdio_vfs_fseek_impl,
	stdio_vfs_rewind_impl,
	stdio_vfs_ftell_impl,
	stdio_vfs_feof_impl,
	stdio_vfs_truncate_impl
};

gboolean
vfs_init(void)
{
    vfs_register_transport(&default_const);
    vfs_register_transport(&file_const);
    return TRUE;
}

