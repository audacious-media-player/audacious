/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "vfs.h"
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>


struct _VFSFile
{
    GnomeVFSHandle *handle;
    gboolean eof;
};


static void mode_to_gnome_vfs(const gchar * mode,
                              GnomeVFSOpenMode * g_mode,
                              gboolean * truncate,
                              gboolean * append);

gboolean
vfs_init(void)
{
    if (!gnome_vfs_init())
	return FALSE;

    g_atexit(gnome_vfs_shutdown);
    return TRUE;
}

VFSFile *
vfs_fopen(const gchar * path,
          const gchar * mode)
{
    VFSFile *file;
    GnomeVFSResult g_result;
    GnomeVFSOpenMode g_mode;
    gboolean truncate, append;

    if (!path || !mode)
	return NULL;

    file = g_new(VFSFile, 1);
    file->eof = FALSE;

    mode_to_gnome_vfs(mode, &g_mode, &truncate, &append);
    gchar *escaped_file = gnome_vfs_escape_path_string(path);

    if (!truncate) {
        g_result = gnome_vfs_open(&(file->handle), escaped_file, g_mode);
        if (append && g_result == GNOME_VFS_ERROR_NOT_FOUND) {
            g_result = gnome_vfs_create(&(file->handle),
                                        path, g_mode, TRUE,
                                        S_IRUSR | S_IWUSR |
                                        S_IRGRP | S_IWGRP |
                                        S_IROTH | S_IWOTH);
        }

        if (append && g_result == GNOME_VFS_OK) {
            g_result = gnome_vfs_seek(file->handle, GNOME_VFS_SEEK_END, 0);
            if (g_result != GNOME_VFS_OK)
                gnome_vfs_close(file->handle);
        }
    }
    else {
        g_result = gnome_vfs_create(&(file->handle),
                                    escaped_file, g_mode, FALSE,
                                    S_IRUSR | S_IWUSR |
                                    S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    }

    if (g_result != GNOME_VFS_OK)
        file->handle = NULL;

    if (file->handle == NULL) {
        g_free(file);
        file = NULL;
    }

    g_free(escaped_file);

    return file;
}

gint
vfs_fclose(VFSFile * file)
{
    gint ret = 0;

    if (file == NULL)
	return 0;

    if (file->handle) {
        if (gnome_vfs_close(file->handle) != GNOME_VFS_OK)
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
    GnomeVFSResult result;
    GnomeVFSFileSize bytes_read;

    if (file == NULL)
	return 0;

    result = gnome_vfs_read(file->handle, ptr, size * nmemb, &bytes_read);
    if (result == GNOME_VFS_OK)
        return bytes_read;

    if (result == GNOME_VFS_ERROR_EOF) {
        file->eof = TRUE;
        return 0;
    }

    return -1;
}

size_t
vfs_fwrite(gconstpointer ptr,
           size_t size,
           size_t nmemb,
           VFSFile * file)
{
    GnomeVFSResult result;
    GnomeVFSFileSize bytes_written;

    if (file == NULL)
	return 0;

    result = gnome_vfs_write(file->handle, ptr, size * nmemb, &bytes_written);
    if (result == GNOME_VFS_OK)
        return bytes_written;
    else
        return -1;
}

gint
vfs_fseek(VFSFile * file,
          glong offset,
          gint whence)
{
    GnomeVFSResult result;
    GnomeVFSSeekPosition g_whence;

    if (file == NULL)
	return 0;

    switch (whence) {
    case SEEK_SET:
        g_whence = GNOME_VFS_SEEK_START;
        break;
    case SEEK_CUR:
        g_whence = GNOME_VFS_SEEK_CURRENT;
        break;
    case SEEK_END:
        g_whence = GNOME_VFS_SEEK_END;
        break;
    default:
        g_warning("vfs_fseek: invalid whence value");
        return -1;
    }

    result = gnome_vfs_seek(file->handle, g_whence, offset);

    if (result == GNOME_VFS_OK)
        return 0;
    else
        return -1;
}

void
vfs_rewind(VFSFile * file)
{
    if (file == NULL)
	return;

    vfs_fseek(file, 0L, SEEK_SET);
}

glong
vfs_ftell(VFSFile * file)
{
    GnomeVFSResult result;
    GnomeVFSFileSize position;

    if (file == NULL)
	return 0;

    result = gnome_vfs_tell(file->handle, &position);

    if (result == GNOME_VFS_OK)
        return position;
    else
        return -1;
}

gboolean
vfs_feof(VFSFile * file)
{
    if (file == NULL)
	return FALSE;

    return file->eof;
}

gboolean
vfs_file_test(const gchar * path,
              GFileTest test)
{
    GnomeVFSResult result;
    GnomeVFSFileInfo info;
    GFileTest file_test;

    result = gnome_vfs_get_file_info(path, &info,
                                     GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);

    if (test == G_FILE_TEST_EXISTS)
        return (result == GNOME_VFS_OK) ? TRUE : FALSE;
    else if (test == G_FILE_TEST_IS_EXECUTABLE)
        return (info.permissions & GNOME_VFS_PERM_ACCESS_EXECUTABLE)
            ? TRUE : FALSE;

    switch (info.type) {
    case GNOME_VFS_FILE_TYPE_REGULAR:
        file_test = G_FILE_TEST_IS_REGULAR;
        break;
    case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
        file_test = G_FILE_TEST_IS_SYMLINK;
        break;
    case GNOME_VFS_FILE_TYPE_DIRECTORY:
        file_test = G_FILE_TEST_IS_DIR;
    default:
        return FALSE;
    }

    if (test == file_test)
        return TRUE;
    else
        return FALSE;
}

gboolean
vfs_is_writeable(const gchar * path)
{
    GnomeVFSFileInfo info;

    if (gnome_vfs_get_file_info(path, &info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS) 
        != GNOME_VFS_OK) {
        return FALSE;
    }

    return (info.permissions & GNOME_VFS_PERM_ACCESS_WRITABLE ? TRUE : FALSE);
}

gint
vfs_truncate(VFSFile * file,
             glong length)
{
    GnomeVFSResult result;

    if (file == NULL)
	return -1;

    result = gnome_vfs_truncate_handle(file->handle, (GnomeVFSFileSize) length);

    if (result == GNOME_VFS_OK)
        return 0;
    else
        return -1;
}

static gchar *strdup_exclude_chars(const gchar * s,
                                   const gchar * c);

static void
mode_to_gnome_vfs(const gchar * mode,
                  GnomeVFSOpenMode * g_mode,
                  gboolean * truncate,
                  gboolean * append)
{
    gchar *s;

    *g_mode = GNOME_VFS_OPEN_RANDOM;
    *truncate = *append = FALSE;

    s = strdup_exclude_chars(mode, "bt");
    switch (s[0]) {
    case 'r':
        *g_mode |= GNOME_VFS_OPEN_READ;

        if (s[1] == '+')
            *g_mode |= GNOME_VFS_OPEN_WRITE;

        break;
    case 'w':
    case 'a':
        *g_mode |= GNOME_VFS_OPEN_WRITE;

        if (s[0] == 'w')
            *truncate = TRUE;
        else
            *append = TRUE;

        if (s[1] == '+')
            *g_mode |= GNOME_VFS_OPEN_READ;

        break;
    default:
        g_warning("mode_to_gnome_vfs: unhandled mode character");
    }
    g_free(s);
}

static gchar *
strdup_exclude_chars(const gchar * s,
                     const gchar * c)
{
    gint i, j, k;
    gint newlen = 0;
    gchar *newstr;
    gboolean found;

    /* Calculate number of chars in new string */
    for (i = 0; s[i] != '\0'; i++) {
        found = FALSE;

        for (j = 0; j < strlen(c) && !found; j++)
            if (s[i] == c[j])
                found = TRUE;

        if (!found)
            newlen++;
    }

    newstr = g_malloc(newlen + 1);

    /* Copy valid chars to new string */
    for (i = k = 0; s[i] != '\0'; i++) {
        found = FALSE;

        for (j = 0; j < strlen(c) && !found; j++)
            if (s[i] == c[j])
                found = TRUE;

        if (!found)
            newstr[k++] = s[i];
    }

    newstr[k] = '\0';

    return newstr;
}
