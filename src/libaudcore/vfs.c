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

#include "vfs.h"
#include "audstrings.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>


/**
 * GList of #VFSConstructor objects holding all the registered
 * VFS transports.
 */
GList *vfs_transports = NULL; /* temporary. -nenolod */


/**
 * Registers a #VFSConstructor vtable with the VFS system.
 *
 * @param vtable The #VFSConstructor vtable to register.
 */
void vfs_register_transport (VFSConstructor * vtable)
{
    vfs_transports = g_list_append (vfs_transports, vtable);
}

static VFSConstructor *
vfs_get_constructor(const gchar *path)
{
    VFSConstructor *vtable = NULL;
    GList *node;

    if (path == NULL)
        return NULL;

    for (node = vfs_transports; node != NULL; node = g_list_next(node))
    {
        VFSConstructor *vtptr = (VFSConstructor *) node->data;

        if (!strncasecmp(path, vtptr->uri_id, strlen(vtptr->uri_id)))
        {
            vtable = vtptr;
            break;
        }
    }

    /* No transport vtable has been registered, bail. */
    if (vtable == NULL)
        g_warning("Could not open '%s', no transport plugin available.", path);

    return vtable;
}

/**
 * Opens a stream from a VFS transport using one of the registered
 * #VFSConstructor handlers.
 *
 * @param path The path or URI to open.
 * @param mode The preferred access privileges (not guaranteed).
 * @return On success, a #VFSFile object representing the stream.
 */
VFSFile *
vfs_fopen(const gchar * path,
          const gchar * mode)
{
    VFSFile *file;
    VFSConstructor *vtable = NULL;

    if (path == NULL || mode == NULL || (vtable = vfs_get_constructor(path)) == NULL)
        return NULL;

    file = vtable->vfs_fopen_impl(path, mode);

    if (file == NULL)
        return NULL;

    file->uri = g_strdup(path);
    file->base = vtable;
    file->ref = 1;

    return file;
}

/**
 * Closes a VFS stream and destroys a #VFSFile object.
 *
 * @param file A #VFSFile object to destroy.
 * @return -1 on failure, 0 on success.
 */
gint
vfs_fclose(VFSFile * file)
{
    gint ret = 0;

    if (file == NULL)
        return -1;

    if (--file->ref > 0)
        return -1;

    if (file->base->vfs_fclose_impl(file) != 0)
        ret = -1;

    g_free(file->uri);
    g_free(file);

    return ret;
}

/**
 * Reads from a VFS stream.
 *
 * @param ptr A pointer to the destination buffer.
 * @param size The size of each element to read.
 * @param nmemb The number of elements to read.
 * @param file #VFSFile object that represents the VFS stream.
 * @return The number of elements succesfully read.
 */
gint64 vfs_fread (void * ptr, gint64 size, gint64 nmemb, VFSFile * file)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fread_impl(ptr, size, nmemb, file);
}

/**
 * Writes to a VFS stream.
 *
 * @param ptr A const pointer to the source buffer.
 * @param size The size of each element to write.
 * @param nmemb The number of elements to write.
 * @param file #VFSFile object that represents the VFS stream.
 * @return The number of elements succesfully written.
 */
gint64 vfs_fwrite (const void * ptr, gint64 size, gint64 nmemb, VFSFile * file)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fwrite_impl(ptr, size, nmemb, file);
}

/**
 * Reads a character from a VFS stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, a character. Otherwise, -1.
 */
gint
vfs_getc(VFSFile *file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_getc_impl(file);
}

/**
 * Pushes a character back to the VFS stream.
 *
 * @param c The character to push back.
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, 0. Otherwise, -1.
 */
gint
vfs_ungetc(gint c, VFSFile *file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_ungetc_impl(c, file);
}

/**
 * Performs a seek in given VFS stream. Standard C-style values
 * of whence can be used to indicate desired action.
 *
 * - SEEK_CUR seeks relative to current stream position.
 * - SEEK_SET seeks to given absolute position (relative to stream beginning).
 * - SEEK_END sets stream position to current file end.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param offset The offset to seek to.
 * @param whence Type of the seek: SEEK_CUR, SEEK_SET or SEEK_END.
 * @return On success, 0. Otherwise, -1.
 */
gint
vfs_fseek(VFSFile * file,
          gint64 offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fseek_impl(file, offset, whence);
}

/**
 * Rewinds a VFS stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 */
void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
        return;

    file->base->vfs_rewind_impl(file);
}

/**
 * Returns the current position in the VFS stream's buffer.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the current position. Otherwise, -1.
 */
glong
vfs_ftell(VFSFile * file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_ftell_impl(file);
}

/**
 * Returns whether or not the VFS stream has reached EOF.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, whether or not the VFS stream is at EOF. Otherwise, FALSE.
 */
gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) file->base->vfs_feof_impl(file);
}

/**
 * Truncates a VFS stream to a certain size.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param length The length to truncate at.
 * @return On success, 0. Otherwise, -1.
 */
gint vfs_ftruncate (VFSFile * file, gint64 length)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_ftruncate_impl(file, length);
}

/**
 * Returns size of the file.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the size of the file in bytes. Otherwise, -1.
 */
gint64
vfs_fsize(VFSFile * file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_fsize_impl(file);
}

/**
 * Returns metadata about the stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param field The string constant field name to get.
 * @return On success, a copy of the value of the field. Otherwise, NULL.
 */
gchar *
vfs_get_metadata(VFSFile * file, const gchar * field)
{
    if (file == NULL)
        return NULL;

    if (file->base->vfs_get_metadata_impl)
        return file->base->vfs_get_metadata_impl(file, field);
    return NULL;
}

/**
 * Wrapper for g_file_test().
 *
 * @param path A path to test.
 * @param test A GFileTest to run.
 * @return The result of g_file_test().
 */
gboolean
vfs_file_test(const gchar * path, GFileTest test)
{
    gchar *path2;
    gboolean ret;

    path2 = g_filename_from_uri(path, NULL, NULL);

    if (path2 == NULL)
        path2 = g_strdup(path);

    ret = g_file_test(path2, test);

    g_free(path2);

    return ret;
}

/**
 * Tests if a file is writeable.
 *
 * @param path A path to test.
 * @return TRUE if the file is writeable, otherwise FALSE.
 */
gboolean
vfs_is_writeable(const gchar * path)
{
    struct stat info;
    gchar *realfn = g_filename_from_uri(path, NULL, NULL);

    if (stat(realfn, &info) == -1)
        return FALSE;

    g_free(realfn);

    return (info.st_mode & S_IWUSR);
}

/**
 * Increments the amount of references that are using this FD.
 * References are removed by calling vfs_fclose on the handle returned
 * from this function. If the amount of references reaches zero, then
 * the file will be closed.
 *
 * @param in The VFSFile handle to mark as duplicated.
 * @return VFSFile handle, which is same as given input.
 */
VFSFile *
vfs_dup(VFSFile *in)
{
    g_return_val_if_fail(in != NULL, NULL);

    in->ref++;

    return in;
}

/**
 * Tests if a path is remote uri.
 *
 * @param path A path to test.
 * @return TRUE if the file is remote, otherwise FALSE.
 */
gboolean
vfs_is_remote(const gchar * path)
{
    VFSConstructor *vtable = NULL;

    if (path == NULL || (vtable = vfs_get_constructor(path)) == NULL)
        return FALSE;

    /* check if vtable->uri_id is file:// or not, for now. */
    if (!strncasecmp("file://", vtable->uri_id, strlen(vtable->uri_id)))
        return FALSE;
    else
        return TRUE;
}

/**
 * Tests if a file is associated to streaming.
 *
 * @param file A #VFSFile object to test.
 * @return TRUE if the file is streaming, otherwise FALSE.
 */
gboolean
vfs_is_streaming(VFSFile *file)
{
    off_t size = 0;

    if (file == NULL)
        return FALSE;

    size = file->base->vfs_fsize_impl(file);

    if (size == -1)
        return TRUE;
    else
        return FALSE;
}
