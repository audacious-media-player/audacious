/*
 * audacious: Cross-platform multimedia player.
 * vfs_buffered_file.c: VFS Buffered I/O support.
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
#include "vfs_buffered_file.h"

VFSFile *
buffered_file_vfs_fopen_impl(const gchar * path,
          const gchar * mode)
{
    return NULL;
}

gint
buffered_file_vfs_fclose_impl(VFSFile * file)
{
    g_return_val_if_fail(file != NULL, -1);

    if (file->handle)
    {
        VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

        if (handle->fd != NULL)
            vfs_fclose(handle->fd);

        vfs_fclose(handle->buffer);
        g_free(handle->mem);
        g_free(handle);
    }

    return 0;
}

size_t
buffered_file_vfs_fread_impl(gpointer i_ptr,
          size_t size,
          size_t nmemb,
          VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    /* is this request within the buffered area, or should we switch to 
     * an FD? --nenolod
     */
    if (handle->which == FALSE && 
	(vfs_ftell(handle->buffer)) + (size * nmemb) > 
	((VFSBuffer *) handle->buffer->handle)->size)
    {
        vfs_fseek(handle->fd, vfs_ftell(handle->buffer), SEEK_SET);
        handle->which = TRUE;
    }

    return vfs_fread(i_ptr, size, nmemb, handle->which == TRUE ? handle->fd : handle->buffer);
}

size_t
buffered_file_vfs_fwrite_impl(gconstpointer i_ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    return vfs_fwrite(i_ptr, size, nmemb, handle->fd);
}

gint
buffered_file_vfs_getc_impl(VFSFile *stream)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) stream->handle;

    /* is this request within the buffered area, or should we switch to 
     * an FD? --nenolod
     */
    if ((vfs_ftell(handle->buffer)) + 1 >
	((VFSBuffer *) handle->buffer->handle)->size)
    {
        vfs_fseek(handle->fd, vfs_ftell(handle->buffer), SEEK_SET);
        handle->which = TRUE;
    }

    return vfs_getc(handle->which == TRUE ? handle->fd : handle->buffer);
}

gint
buffered_file_vfs_ungetc_impl(gint c, VFSFile *stream)
{
    return -1;
}

gint
buffered_file_vfs_fseek_impl(VFSFile * file,
          glong offset,
          gint whence)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    vfs_fseek(handle->buffer, offset, whence);

    /* if we go OOB, switch to live FD */
    if (vfs_ftell(handle->buffer) > ((VFSBuffer *) handle->buffer->handle)->size)
    {
        vfs_rewind(handle->buffer);
        handle->which = TRUE;
        vfs_fseek(handle->buffer, offset, whence);
    }

    return 0;
}

void
buffered_file_vfs_rewind_impl(VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    vfs_rewind(handle->buffer);
    handle->which = FALSE;
}

glong
buffered_file_vfs_ftell_impl(VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    return vfs_ftell(handle->which == TRUE ? handle->fd : handle->buffer);
}

gboolean
buffered_file_vfs_feof_impl(VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    return vfs_feof(handle->which == TRUE ? handle->fd : handle->buffer);
}

gint
buffered_file_vfs_truncate_impl(VFSFile * file, glong size)
{
    return 0;
}

off_t
buffered_file_vfs_fsize_impl(VFSFile * file)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    return vfs_fsize(handle->which == TRUE ? handle->fd : handle->buffer);
}

gchar *
buffered_file_vfs_metadata_impl(VFSFile * file, const gchar * field)
{
    VFSBufferedFile *handle = (VFSBufferedFile *) file->handle;

    return vfs_get_metadata(handle->fd, field);
}

VFSConstructor buffered_file_const = {
	NULL,			// not a normal VFS class
	buffered_file_vfs_fopen_impl,
	buffered_file_vfs_fclose_impl,
	buffered_file_vfs_fread_impl,
	buffered_file_vfs_fwrite_impl,
	buffered_file_vfs_getc_impl,
	buffered_file_vfs_ungetc_impl,
	buffered_file_vfs_fseek_impl,
	buffered_file_vfs_rewind_impl,
	buffered_file_vfs_ftell_impl,
	buffered_file_vfs_feof_impl,
	buffered_file_vfs_truncate_impl,
	buffered_file_vfs_fsize_impl,
	buffered_file_vfs_metadata_impl
};

VFSFile *
vfs_buffered_file_new_from_uri(const gchar *uri)
{
    VFSFile *handle;
    VFSBufferedFile *fd;
    gsize sz;

    g_return_val_if_fail(uri != NULL, NULL);

    handle = g_new0(VFSFile, 1);
    fd = g_new0(VFSBufferedFile, 1);
    fd->mem = g_malloc0(40000);
    fd->fd = vfs_fopen(uri, "rb");

    if (fd->fd == NULL)
    {
	g_free(fd->mem);
	g_free(fd);
	g_free(handle);

	return NULL;
    }

    sz = vfs_fread(fd->mem, 1, 40000, fd->fd);

    if (!sz)
    {
	g_free(fd->mem);
	g_free(fd);
	g_free(handle);

	return NULL;
    }

    fd->buffer = vfs_buffer_new(fd->mem, sz);

    handle->handle = fd;
    handle->base = &buffered_file_const;
    handle->uri = g_strdup(uri);
    handle->ref = 1;

    return handle;
}

VFSFile *
vfs_buffered_file_release_live_fd(VFSFile *fd)
{
    VFSBufferedFile *file = (VFSBufferedFile *) fd;
    VFSFile *out;

    g_return_val_if_fail(file != NULL, NULL);

    out = file->fd;
    file->fd = NULL;

    vfs_fclose(fd);

    return out;
}
