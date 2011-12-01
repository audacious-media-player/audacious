/*  Audacious
 *  Copyright (c) 2006-2007 William Pitcock
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include <string.h>
#include "vfs.h"
#include "vfs_buffer.h"


static VFSFile *
buffer_vfs_fopen_impl(const char * path,
          const char * mode)
{
    return NULL;
}

static int
buffer_vfs_fclose_impl(VFSFile * file)
{
    g_return_val_if_fail(file != NULL, -1);

    g_free(file->handle);

    return 0;
}

static int64_t buffer_vfs_fread_impl (void * i_ptr, int64_t size, int64_t nmemb,
 VFSFile * file)
{
    VFSBuffer *handle;
    unsigned char *i;
    gsize read = 0;
    unsigned char *ptr = (unsigned char *) i_ptr;

    if (file == NULL)
        return 0;

    handle = (VFSBuffer *) file->handle;

    for (i = ptr; (gsize) (i - ptr) < nmemb * size &&
	 (gsize) (i - ptr) <= handle->size;
	 i++, handle->iter++)
    {
       *i = *handle->iter;
       read++;
    }

    return (read / size);
}

static int64_t buffer_vfs_fwrite_impl (const void * i_ptr, int64_t size, int64_t
 nmemb, VFSFile * file)
{
    VFSBuffer *handle;
    const unsigned char *i;
    gsize written = 0;
    const unsigned char *ptr = (const unsigned char *) i_ptr;

    if (file == NULL)
        return 0;

    handle = (VFSBuffer *) file->handle;

    for (i = ptr; (gsize) (i - ptr) < nmemb * size &&
	 (gsize) (i - ptr) <= handle->size;
	 i++, handle->iter++)
    {
       *handle->iter = *i;
       written++;
    }

    return (written / size);
}

static int
buffer_vfs_getc_impl(VFSFile *stream)
{
    VFSBuffer *handle = (VFSBuffer *) stream->handle;
    int c;

    c = *handle->iter;
    handle->iter++;

    return c;
}

static int
buffer_vfs_ungetc_impl(int c, VFSFile *stream)
{
    return -1;
}

static int
buffer_vfs_fseek_impl(VFSFile * file,
          int64_t offset,
          int whence)
{
    VFSBuffer *handle;

    if (file == NULL)
        return 0;

    handle = (VFSBuffer *) file->handle;

    switch(whence)
    {
       case SEEK_CUR:
          handle->iter = handle->iter + offset;
          break;
       case SEEK_END:
          handle->iter = handle->end;
          break;
       case SEEK_SET:
       default:
          handle->iter = handle->data + offset;
          break;
    }

    return 0;
}

static void
buffer_vfs_rewind_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return;

    handle = (VFSBuffer *) file->handle;

    handle->iter = handle->data;
}

static int64_t
buffer_vfs_ftell_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return 0;

    handle = (VFSBuffer *) file->handle;

    return handle->iter - handle->data;
}

static bool
buffer_vfs_feof_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return FALSE;

    handle = (VFSBuffer *) file->handle;

    return (bool) (handle->iter == handle->end);
}

static int
buffer_vfs_truncate_impl (VFSFile * file, int64_t size)
{
    return 0;
}

static int64_t
buffer_vfs_fsize_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return -1;

    handle = (VFSBuffer *) file->handle;

    return (off_t) (handle->end - handle->data);
}

static VFSConstructor buffer_const = {
	buffer_vfs_fopen_impl,
	buffer_vfs_fclose_impl,
	buffer_vfs_fread_impl,
	buffer_vfs_fwrite_impl,
	buffer_vfs_getc_impl,
	buffer_vfs_ungetc_impl,
	buffer_vfs_fseek_impl,
	buffer_vfs_rewind_impl,
	buffer_vfs_ftell_impl,
	buffer_vfs_feof_impl,
	buffer_vfs_truncate_impl,
	buffer_vfs_fsize_impl,
	NULL
};

VFSFile *
vfs_buffer_new(void * data, gsize size)
{
    VFSFile *handle;
    VFSBuffer *buffer;

    g_return_val_if_fail(data != NULL, NULL);
    g_return_val_if_fail(size > 0, NULL);

    if ((handle = g_new0(VFSFile, 1)) == NULL)
        return NULL;

    if ((buffer = g_new0(VFSBuffer, 1)) == NULL)
    {
        g_free(handle);
        return NULL;
    }

    buffer->data = data;
    buffer->iter = data;
    buffer->end = (unsigned char *) data + size;
    buffer->size = size;

    handle->handle = buffer;
    handle->base = &buffer_const;
    handle->ref = 1;
    handle->sig = VFS_SIG;

    return handle;
}

VFSFile *
vfs_buffer_new_from_string(char *str)
{
    g_return_val_if_fail(str != NULL, NULL);

    return vfs_buffer_new(str, strlen(str));
}
