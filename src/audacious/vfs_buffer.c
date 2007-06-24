/*
 * audacious: Cross-platform multimedia player.
 * vfs_buffer.c: VFS operations on blocks of memory.
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

#include <glib.h>
#include <string.h>
#include "vfs.h"
#include "vfs_buffer.h"

VFSFile *
buffer_vfs_fopen_impl(const gchar * path,
          const gchar * mode)
{
    return NULL;
}

gint
buffer_vfs_fclose_impl(VFSFile * file)
{
    g_return_val_if_fail(file != NULL, -1);

    if (file->handle)
        g_free(file->handle);

    return 0;
}

size_t
buffer_vfs_fread_impl(gpointer i_ptr,
          size_t size,
          size_t nmemb,
          VFSFile * file)
{
    VFSBuffer *handle;
    guchar *i;
    size_t read = 0;
    guchar *ptr = (guchar *) i_ptr;

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

size_t
buffer_vfs_fwrite_impl(gconstpointer i_ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    VFSBuffer *handle;
    const guchar *i;
    size_t written = 0;
    const guchar *ptr = (const guchar *) i_ptr;

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

gint
buffer_vfs_getc_impl(VFSFile *stream)
{
  VFSBuffer *handle = (VFSBuffer *) stream->handle;
  gint c;

  c = *handle->iter;
  handle->iter++;

  return c;
}

gint
buffer_vfs_ungetc_impl(gint c, VFSFile *stream)
{
  return -1;
}

gint
buffer_vfs_fseek_impl(VFSFile * file,
          glong offset,
          gint whence)
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

void
buffer_vfs_rewind_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return;

    handle = (VFSBuffer *) file->handle;

    handle->iter = handle->data;
}

glong
buffer_vfs_ftell_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return 0;

    handle = (VFSBuffer *) file->handle;

    return handle->iter - handle->data;
}

gboolean
buffer_vfs_feof_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return FALSE;

    handle = (VFSBuffer *) file->handle;

    return (gboolean) (handle->iter == handle->end);
}

gint
buffer_vfs_truncate_impl(VFSFile * file, glong size)
{
    return 0;
}

off_t
buffer_vfs_fsize_impl(VFSFile * file)
{
    VFSBuffer *handle;

    if (file == NULL)
        return -1;

    handle = (VFSBuffer *) file->handle;

    return (off_t)handle->end;
}

VFSConstructor buffer_const = {
	NULL,			// not a normal VFS class
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
vfs_buffer_new(gpointer data, gsize size)
{
    VFSFile *handle;
    VFSBuffer *buffer;

    g_return_val_if_fail(data != NULL, NULL);
    g_return_val_if_fail(size > 0, NULL);

    handle = g_new0(VFSFile, 1);

    buffer = g_new0(VFSBuffer, 1);
    buffer->data = data;
    buffer->iter = data;
    buffer->end = (guchar *) data + size;
    buffer->size = size;

    handle->handle = buffer;
    handle->base = &buffer_const;
    handle->ref = 1;

    return handle;
}

VFSFile *
vfs_buffer_new_from_string(gchar *str)
{
    g_return_val_if_fail(str != NULL, NULL);

    return vfs_buffer_new(str, strlen(str));
}
