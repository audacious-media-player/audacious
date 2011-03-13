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

#include <inttypes.h>

#include "vfs.h"
#include "audstrings.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <mowgli.h>

/* Audacious core provides us with a function that looks up a VFS transport for
 * a given URI scheme.  Since this function will load plugins as needed, it can
 * only be called from the main thread.  When VFS is used from parallel threads,
 * vfs_prepare must be called from the main thread to look up any needed
 * transports beforehand. */

typedef struct {
    VFSConstructor * transport;
    gboolean prepared;
} LookupNode;

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static GThread * lookup_thread = NULL;
static VFSConstructor * (* lookup_func) (const gchar * scheme) = NULL;
static mowgli_patricia_t * lookup_table = NULL;

void vfs_set_lookup_func (VFSConstructor * (* func) (const gchar * scheme))
{
    g_static_mutex_lock (& mutex);

    lookup_thread = g_thread_self ();
    lookup_func = func;

    if (! lookup_table)
        lookup_table = mowgli_patricia_create (NULL);

    g_static_mutex_unlock (& mutex);
}

static VFSConstructor * do_lookup (const gchar * scheme, gboolean prepare)
{
    g_return_val_if_fail (lookup_thread && lookup_func && lookup_table, NULL);

    LookupNode * node = mowgli_patricia_retrieve (lookup_table, scheme);
    if (! node)
    {
        node = g_slice_new (LookupNode);
        node->transport = NULL;
        node->prepared = FALSE;
        mowgli_patricia_add (lookup_table, scheme, node);
    }

    if (prepare)
        node->prepared = TRUE;

    /* vfs_prepare can only be called from the main thread.  vfs_fopen can only
     * be called from the main thread unless vfs_prepare has been called. */
    if (prepare || ! node->prepared)
        g_return_val_if_fail (g_thread_self () == lookup_thread, NULL);

    /* do the actual lookup if needed, but only in the main thread */
    if (! node->transport && g_thread_self () == lookup_thread)
    {
        node->transport = lookup_func (scheme);
        /* This is normal for custom URI schemes.
        if (! node->transport)
            fprintf (stderr, "No transport plugin found for URI scheme \"%s\".\n", scheme); */
    }

    return node->transport;
}

void vfs_prepare (const gchar * scheme)
{
    g_static_mutex_lock (& mutex);
    do_lookup (scheme, TRUE);
    g_static_mutex_unlock (& mutex);
}

void vfs_prepare_filename (const gchar * path)
{
    const gchar * s = strstr (path, "://");
    g_return_if_fail (s);
    gchar scheme[s - path + 1];
    strncpy (scheme, path, s - path);
    scheme[s - path] = 0;

    vfs_prepare (scheme);
}

static gboolean verbose = FALSE;

void vfs_set_verbose (gboolean set)
{
    verbose = set;
}

static void logger (const gchar * format, ...)
{
    static gchar last[256] = "";
    static gint repeated = 0;

    gchar buf[256];

    va_list args;
    va_start (args, format);
    vsnprintf (buf, sizeof buf, format, args);
    va_end (args);

    if (! strcmp (buf, last))
        repeated ++;
    else
    {
        if (repeated)
        {
            printf ("VFS: (last message repeated %d times)\n", repeated);
            repeated = 0;
        }
        
        fputs (buf, stdout);
        strcpy (last, buf);
    }
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
    g_return_val_if_fail (path && mode, NULL);
    g_return_val_if_fail (lookup_func, NULL);

    VFSFile *file;
    VFSConstructor *vtable = NULL;

    const gchar * s = strstr (path, "://");
    g_return_val_if_fail (s, NULL);
    gchar scheme[s - path + 1];
    strncpy (scheme, path, s - path);
    scheme[s - path] = 0;

    g_static_mutex_lock (& mutex);
    vtable = do_lookup (scheme, FALSE);
    g_static_mutex_unlock (& mutex);

    if (! vtable)
        return NULL;

    file = vtable->vfs_fopen_impl(path, mode);

    if (verbose)
        logger ("VFS: <%p> open (mode %s) %s\n", file, mode, path);

    if (file == NULL)
        return NULL;

    file->uri = g_strdup(path);
    file->base = vtable;
    file->ref = 1;
    file->sig = VFS_SIG;

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
    g_return_val_if_fail (file && file->sig == VFS_SIG, -1);

    if (verbose)
        printf ("VFS: <%p> close\n", file);

    gint ret = 0;

    if (--file->ref > 0)
        return -1;

    if (file->base->vfs_fclose_impl(file) != 0)
        ret = -1;

    g_free(file->uri);

    memset (file, 0, sizeof (VFSFile));
    g_free (file);

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
    g_return_val_if_fail (file && file->sig == VFS_SIG, 0);

    gint64 readed = file->base->vfs_fread_impl (ptr, size, nmemb, file);
    
    if (verbose)
        logger ("VFS: <%p> read %"PRId64" elements of size %"PRId64" = "
         "%"PRId64"\n", file, nmemb, size, readed);

    return readed;
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
    g_return_val_if_fail (file && file->sig == VFS_SIG, 0);

    gint64 written = file->base->vfs_fwrite_impl (ptr, size, nmemb, file);

    if (verbose)
        logger ("VFS: <%p> write %"PRId64" elements of size %"PRId64" = "
         "%"PRId64"\n", file, nmemb, size, written);

    return written;
}

/**
 * Reads a character from a VFS stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, a character. Otherwise, EOF.
 */
gint
vfs_getc(VFSFile *file)
{
    g_return_val_if_fail (file && file->sig == VFS_SIG, EOF);

    if (verbose)
        logger ("VFS: <%p> getc\n", file);

    return file->base->vfs_getc_impl(file);
}

/**
 * Pushes a character back to the VFS stream.
 *
 * @param c The character to push back.
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, 0. Otherwise, EOF.
 */
gint
vfs_ungetc(gint c, VFSFile *file)
{
    g_return_val_if_fail (file && file->sig == VFS_SIG, EOF);

    if (verbose)
        logger ("VFS: <%p> ungetc\n", file);

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
    g_return_val_if_fail (file && file->sig == VFS_SIG, -1);

    if (verbose)
        logger ("VFS: <%p> seek to %"PRId64" from %s\n", file, offset, whence ==
         SEEK_CUR ? "current" : whence == SEEK_SET ? "beginning" : whence ==
         SEEK_END ? "end" : "invalid");

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
    g_return_if_fail (file && file->sig == VFS_SIG);

    if (verbose)
        logger ("VFS: <%p> rewind\n", file);

    file->base->vfs_rewind_impl(file);
}

/**
 * Returns the current position in the VFS stream's buffer.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the current position. Otherwise, -1.
 */
gint64
vfs_ftell(VFSFile * file)
{
    g_return_val_if_fail (file && file->sig == VFS_SIG, -1);

    gint64 told = file->base->vfs_ftell_impl (file);

    if (verbose)
        logger ("VFS: <%p> tell = %"PRId64"\n", file, told);

    return told;
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
    g_return_val_if_fail (file && file->sig == VFS_SIG, TRUE);

    gboolean eof = file->base->vfs_feof_impl (file);

    if (verbose)
        logger ("VFS: <%p> eof = %s\n", file, eof ? "yes" : "no");

    return eof;
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
    g_return_val_if_fail (file && file->sig == VFS_SIG, -1);

    if (verbose)
        logger ("VFS: <%p> truncate to %"PRId64"\n", file, length);

    return file->base->vfs_ftruncate_impl(file, length);
}

/**
 * Returns size of the file.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the size of the file in bytes. Otherwise, -1.
 */
gint64 vfs_fsize (VFSFile * file)
{
    g_return_val_if_fail (file && file->sig == VFS_SIG, -1);

    gint64 size = file->base->vfs_fsize_impl (file);

    if (verbose)
        logger ("VFS: <%p> size = %"PRId64"\n", file, size);

    return size;
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
    if (strncmp (path, "file://", 7))
        return FALSE; /* only local files are handled */

    gchar * path2 = uri_to_filename (path);

    if (path2 == NULL)
        path2 = g_strdup(path);

    gboolean ret = g_file_test (path2, test);

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
    gchar * realfn = uri_to_filename (path);

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
    return strncasecmp (path, "file://", 7) ? TRUE : FALSE;
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
