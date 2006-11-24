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

gboolean
vfs_register_transport(VFSConstructor *vtable)
{
    vfs_transports = g_list_append(vfs_transports, vtable);

    return TRUE;
}

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
        return NULL;
    }

    file = vtable->vfs_fopen_impl(vec[1] ? vec[1] : vec[0], mode);

    if (file == NULL)
    {
        return NULL;
    }

    file->uri = g_strdup(path);
    file->base = vtable;

    return file;
}

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

gint
vfs_getc(VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_getc_impl(stream);
}

gint
vfs_ungetc(gint c, VFSFile *stream)
{
    if (stream == NULL)
        return -1;

    return stream->base->vfs_ungetc_impl(c, stream);
}

gint
vfs_fseek(VFSFile * file,
          glong offset,
          gint whence)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_fseek_impl(file, offset, whence);
}

void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
        return;

    file->base->vfs_rewind_impl(file);
}

glong
vfs_ftell(VFSFile * file)
{
    if (file == NULL)
        return 0;

    return file->base->vfs_ftell_impl(file);
}

gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
        return FALSE;

    return (gboolean) file->base->vfs_feof_impl(file);
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

    return file->base->vfs_truncate_impl(file, size);
}
