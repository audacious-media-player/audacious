/*
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

gsize
buffered_file_vfs_fread_impl(gpointer i_ptr,
          gsize size,
          gsize nmemb,
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

gsize
buffered_file_vfs_fwrite_impl(gconstpointer i_ptr,
           gsize size,
           gsize nmemb,
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

    switch(whence)
    {
        case SEEK_END:
            handle->which = TRUE;
            vfs_fseek(handle->fd, offset, whence);
            break;
        case SEEK_CUR:
            if (vfs_ftell(handle->buffer) + offset > ((VFSBuffer *) handle->buffer->handle)->size)
            {
                handle->which = TRUE;
                vfs_fseek(handle->fd, offset, whence);
            }                
            break;
        case SEEK_SET:
        default:
            if (offset > ((VFSBuffer *) handle->buffer->handle)->size)
            {
                handle->which = TRUE;
                vfs_fseek(handle->fd, offset, whence);
            }
            else
            {
                handle->which = FALSE;
                vfs_fseek(handle->buffer, offset, whence);
            }
            break;
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

    return vfs_fsize(handle->fd);
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

/**
 * Creates a VFSBufferedFile. VFSBufferedFile is read-only.
 *
 * @param uri URI locator pointing to the file to open.
 * @return A VFSFile handle for the VFSBufferedFile.
 **/
VFSFile *
vfs_buffered_file_new_from_uri(const gchar *uri)
{
    VFSFile *handle;
    VFSBufferedFile *fd;
    gsize sz;

    g_return_val_if_fail(uri != NULL, NULL);

    handle = g_new0(VFSFile, 1);
    fd = g_new0(VFSBufferedFile, 1);
    fd->mem = g_malloc0(128000);
    fd->fd = vfs_fopen(uri, "rb");

    if (fd->fd == NULL)
    {
	g_free(fd->mem);
	g_free(fd);
	g_free(handle);

	return NULL;
    }

    sz = vfs_fread(fd->mem, 1, 128000, fd->fd);
    vfs_rewind(fd->fd);

    if (!sz)
    {
	vfs_fclose(fd->fd);
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

/**
 * @bug Is this function used for anything? -- ccr
 * @deprecated This function probably should not be used.
 */
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
