/*  Audacious
 *  Copyright (c) 2006 William Pitcock
 *
 *  This program is free software; you can redistribute it and/or modify
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

static GList *vfs_transports = NULL;

#ifdef VFS_DEBUG
# define DBG(x, args...) g_print(x, ## args);
#else
# define DBG(x, args...)
#endif

/*
 * vfs_register_transport(VFSConstructor *vtable)
 *
 * Registers a VFSConstructor vtable with the VFS system.
 *
 * Inputs:
 *     - a VFSConstructor object
 *
 * Outputs:
 *     - TRUE on success, FALSE on failure.
 *
 * Side Effects:
 *     - none
 */
gboolean
vfs_register_transport(VFSConstructor *vtable)
{
    vfs_transports = g_list_append(vfs_transports, vtable);

    return TRUE;
}

/*
 * vfs_fopen(const gchar * path, const gchar * mode)
 *
 * Opens a stream from a VFS transport using a VFSConstructor.
 *
 * Inputs:
 *     - path or URI to open
 *     - preferred access privileges (not guaranteed)
 *
 * Outputs:
 *     - on success, a VFSFile object representing the VFS stream
 *     - on failure, nothing
 *
 * Side Effects:
 *     - file descriptors are opened or more memory is allocated.
 */
VFSFile *
vfs_fopen(const gchar * path,
          const gchar * mode)
{
    VFSFile *file;
    gchar **vec;
    VFSConstructor *vtable = NULL;
    GList *node;

    if (!path || !mode)
	return NULL;

    vec = g_strsplit(path, "://", 2);

    /* special case: no transport specified, look for the "/" transport */
    if (vec[1] == NULL)
    {
        for (node = vfs_transports; node != NULL; node = g_list_next(node))
        {
            vtable = (VFSConstructor *) node->data;

            if (*vtable->uri_id == '/')
                break;
        }
    }
    else
    {
        for (node = vfs_transports; node != NULL; node = g_list_next(node))
        {
            vtable = (VFSConstructor *) node->data;

            if (!g_strcasecmp(vec[0], vtable->uri_id))
                break;
        }
    }

    /* no transport vtable has been registered, bail. */
    if (vtable == NULL)
    {
        g_strfreev(vec);
        return NULL;
    }

    file = vtable->vfs_fopen_impl(vec[1] ? vec[1] : vec[0], mode);

    if (file == NULL)
    {
        g_strfreev(vec);
        return NULL;
    }

    file->uri = g_strdup(path);
    file->base = vtable;

    g_strfreev(vec);

    return file;
}

/*
 * vfs_fclose(VFSFile * file)
 *
 * Closes a VFS stream and destroys a VFSFile object.
 *
 * Inputs:
 *     - a VFSFile object to destroy
 *
 * Outputs:
 *     - -1 on success, otherwise 0
 *
 * Side Effects:
 *     - a file description is closed or allocated memory is freed
 */
gint
vfs_fclose(VFSFile * file)
{
    gint ret = 0;

    if (file == NULL)
        return -1;

    if (file->base->vfs_fclose_impl(file) != 0)
        ret = -1;

    if (file->uri != NULL)
        g_free(file->uri);

    g_free(file);

    return ret;
}

/*
 * vfs_fread(gpointer ptr, size_t size, size_t nmemb, VFSFile * file)
 *
 * Reads from a VFS stream.
 *
 * Inputs:
 *     - pointer to destination buffer
 *     - size of each element to read
 *     - number of elements to read
 *     - VFSFile object that represents the VFS stream
 *
 * Outputs:
 *     - on success, the amount of elements successfully read
 *     - on failure, -1
 *
 * Side Effects:
 *     - on nonblocking sources, the socket may be unavailable after 
 *       this call.
 */
size_t
vfs_fread(gpointer ptr,
          size_t size,
          size_t nmemb,
          VFSFile * file)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fread_impl(ptr, size, nmemb, file);
}

/*
 * vfs_fwrite(gconstpointer ptr, size_t size, size_t nmemb, VFSFile * file)
 *
 * Writes to a VFS stream.
 *
 * Inputs:
 *     - const pointer to source buffer
 *     - size of each element to write
 *     - number of elements to write
 *     - VFSFile object that represents the VFS stream
 *
 * Outputs:
 *     - on success, the amount of elements successfully written
 *     - on failure, -1
 *
 * Side Effects:
 *     - on nonblocking sources, the socket may be unavailable after 
 *       this call.
 */
size_t
vfs_fwrite(gconstpointer ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fwrite_impl(ptr, size, nmemb, file);
}

/*
 * vfs_getc(VFSFile *stream)
 *
 * Reads a character from a VFS stream.
 *
 * Inputs:
 *     - a VFSFile object representing a VFS stream.
 *
 * Outputs:
 *     - on success, a character
 *     - on failure, -1
 *
 * Side Effects:
 *     - on nonblocking sources, the socket may be unavailable after 
 *       this call.
 */
gint
vfs_getc(VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_getc_impl(stream);
}

/*
 * vfs_ungetc(gint c, VFSFile *stream)
 *
 * Pushes a character back to the VFS stream.
 *
 * Inputs:
 *     - a character to push back
 *     - a VFSFile object representing a VFS stream.
 *
 * Outputs:
 *     - on success, 0
 *     - on failure, -1
 *
 * Side Effects:
 *     - on nonblocking sources, the socket may be unavailable after 
 *       this call.
 */
gint
vfs_ungetc(gint c, VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_ungetc_impl(c, stream);
}

/*
 * vfs_fseek(VFSFile * file, gint offset, gint whence)
 *
 * Seeks through a VFS stream.
 *
 * Inputs:
 *     - a VFSFile object which represents a VFS stream
 *     - the offset to seek
 *     - whether or not the seek is absolute or non-absolute
 *
 * Outputs:
 *     - on success, 1
 *     - on failure, 0
 *
 * Side Effects:
 *     - on nonblocking sources, this is not guaranteed to work
 */
gint
vfs_fseek(VFSFile * file,
          glong offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fseek_impl(file, offset, whence);
}

/*
 * vfs_rewind(VFSFile * file)
 *
 * Rewinds a VFS stream.
 *
 * Inputs:
 *     - a VFSFile object which represents a VFS stream
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on nonblocking sources, this is not guaranteed to work
 */
void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
        return;

    file->base->vfs_rewind_impl(file);
}

/*
 * vfs_ftell(VFSFile * file)
 *
 * Returns the position of a VFS stream.
 *
 * Inputs:
 *     - a VFSFile object which represents a VFS stream
 *
 * Outputs:
 *     - on failure, -1.
 *     - on success, the stream's position
 *
 * Side Effects:
 *     - on nonblocking sources, this is not guaranteed to work
 */
glong
vfs_ftell(VFSFile * file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_ftell_impl(file);
}

/*
 * vfs_feof(VFSFile * file)
 *
 * Returns whether or not the VFS stream has reached EOF.
 *
 * Inputs:
 *     - a VFSFile object which represents a VFS stream
 *
 * Outputs:
 *     - on failure, FALSE.
 *     - on success, whether or not the VFS stream is at EOF.
 *
 * Side Effects:
 *     - none
 */
gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) file->base->vfs_feof_impl(file);
}

/*
 * vfs_truncate(VFSFile * file, glong size)
 *
 * Truncates a VFS stream to a certain size.
 *
 * Inputs:
 *     - a VFS stream to truncate
 *     - length to truncate at
 *
 * Outputs:
 *     - -1 on failure
 *     - 0 on success
 *
 * Side Effects:
 *     - this is not guaranteed to work on non-blocking
 *       sources
 */
gint
vfs_truncate(VFSFile * file, glong size)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_truncate_impl(file, size);
}

/*
 * vfs_file_test(const gchar * path, GFileTest test)
 *
 * Wrapper for g_file_test().
 *
 * Inputs:
 *     - a path to test
 *     - a GFileTest to run
 *
 * Outputs:
 *     - the result of g_file_test().
 *
 * Side Effects:
 *     - g_file_test() is called.
 */
gboolean
vfs_file_test(const gchar * path, GFileTest test)
{
    return g_file_test(path, test);
}

/*
 * vfs_is_writable(const gchar * path)
 *
 * Tests if a file is writable.
 *
 * Inputs:
 *     - a path to test
 *
 * Outputs:
 *     - FALSE if the file is not writable
 *     - TRUE if the file is writable
 *
 * Side Effects:
 *     - stat() is called.
 *
 * Bugs:
 *     - stat() is not considered part of stdio
 */
gboolean
vfs_is_writeable(const gchar * path)
{
    struct stat info;

    if (stat(path, &info) == -1)
        return FALSE;

    return (info.st_mode & S_IWUSR);
}
