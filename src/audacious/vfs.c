/*
 * audacious: Cross-platform multimedia player.
 * vfs.c: VFS subsystem core commands.
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

#include "vfs.h"
#include "strings.h"
#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>

#include "urldecode.h"

GList *vfs_transports = NULL; /* temporary. -nenolod */

#ifdef VFS_DEBUG
# define DBG(x, args...) g_print(x, ## args);
#else
# define DBG(x, args...)
#endif

/**
 * vfs_register_transport:
 * @vtable: The #VFSConstructor vtable to register.
 *
 * Registers a #VFSConstructor vtable with the VFS system.
 *
 * Return value: TRUE on success, FALSE on failure.
 **/
gboolean
vfs_register_transport(VFSConstructor *vtable)
{
    vfs_transports = g_list_append(vfs_transports, vtable);

    return TRUE;
}

/**
 * vfs_fopen:
 * @path: The path or URI to open.
 * @mode: The preferred access privileges (not guaranteed).
 *
 * Opens a stream from a VFS transport using a #VFSConstructor.
 *
 * Return value: On success, a #VFSFile object representing the stream.
 **/
VFSFile *
vfs_fopen(const gchar * path,
          const gchar * mode)
{
    VFSFile *file;
    VFSConstructor *vtable = NULL;
    GList *node;
    gchar *decpath;

    if (!path || !mode)
	return NULL;

    decpath = g_strdup(path);

    for (node = vfs_transports; node != NULL; node = g_list_next(node))
    {
        VFSConstructor *vtptr = (VFSConstructor *) node->data;

        if (!strncasecmp(decpath, vtptr->uri_id, strlen(vtptr->uri_id)))
        {
            vtable = vtptr;
            break;
        }
    }

    /* no transport vtable has been registered, bail. */
    if (vtable == NULL)
        return NULL;

    file = vtable->vfs_fopen_impl(decpath, mode);

    if (file == NULL)
        return NULL;

    file->uri = g_strdup(path);
    file->base = vtable;
    file->ref = 1;

    g_free(decpath);

    return file;
}

/**
 * vfs_fclose:
 * @file: A #VFSFile object to destroy.
 *
 * Closes a VFS stream and destroys a #VFSFile object.
 *
 * Return value: -1 on failure, 0 on success.
 **/
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

    if (file->uri != NULL)
        g_free(file->uri);

    g_free(file);

    return ret;
}

/**
 * vfs_fread:
 * @ptr: A pointer to the destination buffer.
 * @size: The size of each element to read.
 * @nmemb: The number of elements to read.
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Reads from a VFS stream.
 *
 * Return value: The amount of elements succesfully read.
 **/
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

/**
 * vfs_fwrite:
 * @ptr: A const pointer to the source buffer.
 * @size: The size of each element to write.
 * @nmemb: The number of elements to write.
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Writes to a VFS stream.
 *
 * Return value: The amount of elements succesfully written.
 **/
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

/**
 * vfs_getc:
 * @stream: #VFSFile object that represents the VFS stream.
 *
 * Reads a character from a VFS stream.
 *
 * Return value: On success, a character. Otherwise, -1.
 **/
gint
vfs_getc(VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_getc_impl(stream);
}

/**
 * vfs_ungetc:
 * @c: The character to push back.
 * @stream: #VFSFile object that represents the VFS stream.
 *
 * Pushes a character back to the VFS stream.
 *
 * Return value: On success, 0. Otherwise, -1.
 **/
gint
vfs_ungetc(gint c, VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_ungetc_impl(c, stream);
}

/**
 * vfs_fseek:
 * @file: #VFSFile object that represents the VFS stream.
 * @offset: The offset to seek to.
 * @whence: Whether or not the seek is absolute or not.
 *
 * Seeks through a VFS stream.
 *
 * Return value: On success, 1. Otherwise, 0.
 **/
gint
vfs_fseek(VFSFile * file,
          glong offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fseek_impl(file, offset, whence);
}

/**
 * vfs_rewind:
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Rewinds a VFS stream.
 **/
void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
        return;

    file->base->vfs_rewind_impl(file);
}

/**
 * vfs_ftell:
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Returns the current position in the VFS stream's buffer.
 *
 * Return value: On success, the current position. Otherwise, -1.
 **/
glong
vfs_ftell(VFSFile * file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_ftell_impl(file);
}

/**
 * vfs_feof:
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Returns whether or not the VFS stream has reached EOF.
 *
 * Return value: On success, whether or not the VFS stream is at EOF. Otherwise, FALSE.
 **/
gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) file->base->vfs_feof_impl(file);
}

/**
 * vfs_truncate:
 * @file: #VFSFile object that represents the VFS stream.
 * @length: The length to truncate at.
 *
 * Truncates a VFS stream to a certain size.
 *
 * Return value: On success, 0. Otherwise, -1.
 **/
gint
vfs_truncate(VFSFile * file, glong length)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_truncate_impl(file, length);
}

/**
 * vfs_fsize:
 * @file: #VFSFile object that represents the VFS stream.
 *
 * Returns te size of the file
 *
 * Return value: On success, the size of the file in bytes.
 * Otherwise, -1.
 */
off_t
vfs_fsize(VFSFile * file)
{
    if (file == NULL)
        return -1;

    return file->base->vfs_fsize_impl(file);
}

/**
 * vfs_get_metadata:
 * @file: #VFSFile object that represents the VFS stream.
 * @field: The string constant field name to get.
 *
 * Returns metadata about the stream.
 *
 * Return value: On success, a copy of the value of the
 * field. Otherwise, NULL.
 **/
gchar *
vfs_get_metadata(VFSFile * file, const gchar * field)
{
    if (file == NULL)
        return NULL;
  
    if (file->base->vfs_get_metadata_impl)
        return file->base->vfs_get_metadata_impl(file, field);
    return NULL;
}

static gchar *
_vfs_urldecode_basic_path(const gchar * encoded_path)
{
    const gchar *cur, *ext;
    gchar *path, *tmp;
    gint realchar;

    if (!encoded_path)
        return NULL;

    if (!str_has_prefix_nocase(encoded_path, "file:"))
        return NULL;

    cur = encoded_path + 5;

    if (str_has_prefix_nocase(cur, "//localhost"))
        cur += 11;

    if (*cur == '/')
        while (cur[1] == '/')
            cur++;

    tmp = g_malloc0(strlen(cur) + 1);

    while ((ext = strchr(cur, '%')) != NULL) {
        strncat(tmp, cur, ext - cur);
        ext++;
        cur = ext + 2;
        if (!sscanf(ext, "%2x", &realchar)) {
            /* Assume it is a literal '%'.  Several file
             * managers send unencoded file: urls on drag
             * and drop. */
            realchar = '%';
            cur -= 2;
        }
        tmp[strlen(tmp)] = realchar;
    }

    path = g_strconcat(tmp, cur, NULL);
    g_free(tmp);
    return path;
}

/**
 * vfs_file_test:
 * @path: A path to test.
 * @test: A GFileTest to run.
 *
 * Wrapper for g_file_test().
 *
 * Return value: The result of g_file_test().
 **/
gboolean
vfs_file_test(const gchar * path, GFileTest test)
{
    gchar *path2;
    gboolean ret;

    path2 = _vfs_urldecode_basic_path(path);

    if (path2 == NULL)
        path2 = g_strdup(path);

    ret = g_file_test(path2, test);

    g_free(path2);

    return ret;
}

/**
 * vfs_is_writeable:
 * @path: A path to test.
 *
 * Tests if a file is writeable.
 *
 * Return value: TRUE if the file is writeable, otherwise FALSE.
 **/
gboolean
vfs_is_writeable(const gchar * path)
{
    struct stat info;

    if (stat(path, &info) == -1)
        return FALSE;

    return (info.st_mode & S_IWUSR);
}

/**
 * vfs_dup:
 * @in: The VFSFile handle to mark as duplicated.
 *
 * Increments the amount of references that are using this FD.
 * References are removed by calling vfs_fclose on the handle returned
 * from this function.
 * If the amount of references reaches zero, then the file will be
 * closed.
 **/
VFSFile *
vfs_dup(VFSFile *in)
{
    g_return_val_if_fail(in != NULL, NULL);

    in->ref++;

    return in;
}
