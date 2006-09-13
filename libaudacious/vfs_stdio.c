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


struct _VFSFile {
    FILE *handle;
};


gboolean
vfs_init(void)
{
    return TRUE;
}

VFSFile *
vfs_fopen(const gchar * path,
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
vfs_fclose(VFSFile * file)
{
    gint ret = 0;

    if (file == NULL)
        return -1;

    if (file->handle) {
        if (fclose(file->handle) != 0)
            ret = -1;
    }

    g_free(file);

    return ret;
}

size_t
vfs_fread(gpointer ptr,
          size_t size,
          size_t nmemb,
          VFSFile * file)
{
    if (file == NULL)
        return 0;

    return fread(ptr, size, nmemb, file->handle);
}

size_t
vfs_fwrite(gconstpointer ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    if (file == NULL)
        return 0;

    return fwrite(ptr, size, nmemb, file->handle);
}

gint
vfs_getc(VFSFile *stream)
{
  return getc( stream->handle );
}

gint
vfs_ungetc(gint c, VFSFile *stream)
{
  return ungetc( c , stream->handle );
}

gint
vfs_fseek(VFSFile * file,
          glong offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return fseek(file->handle, offset, whence);
}

void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
        return;

    rewind(file->handle);
}

glong
vfs_ftell(VFSFile * file)
{
    if (file == NULL)
        return 0;

    return ftell(file->handle);
}

gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) feof(file->handle);
}

gboolean
vfs_file_test(const gchar * path, GFileTest test)
{
    return g_file_test(path, test);
}

/* NOTE: stat() is not part of stdio */
gboolean
vfs_is_writeable(const gchar * path)
{
    struct stat info;

    if (stat(path, &info) == -1)
        return FALSE;

    return (info.st_mode & S_IWUSR);
}

gint
vfs_truncate(VFSFile * file, glong size)
{
    if (file == NULL)
        return -1;

    return ftruncate(fileno(file->handle), size);
}
