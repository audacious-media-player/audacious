/*
 * Audacious
 * Copyright (c) 2006 Audacious team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef VFS_H
#define VFS_H

#include <glib.h>
#include <stdio.h>

typedef struct _VFSFile VFSFile;

struct _VFSConstructor {
	gchar *uri_id;
	VFSFile *(*vfs_fopen_impl)(const gchar *path,
		const gchar *mode);
	gint (*vfs_fclose_impl)(VFSFile * file);
	size_t (*vfs_fread_impl)(gpointer ptr, size_t size,
		size_t nmemb, VFSFile *file);
	gint (*vfs_ungetc_impl)(gint c, VFSFile *stream);
	gint (*vfs_fseek_impl)(VFSFile *file, glong offset, gint whence);
	void (*vfs_rewind_impl)(VFSFile *file);
	glong (*vfs_ftell_impl)(VFSFile *file);
	gboolean (*vfs_feof_impl)(VFSFile *file);
	gboolean (*vfs_truncate_impl)(VFSFile *file, glong length);
};

typedef struct _VFSConstructor VFSConstructor;

G_BEGIN_DECLS

/* Reserved for private use by BMP */
extern gboolean vfs_init(void);

extern VFSFile * vfs_fopen(const gchar * path,
                    const gchar * mode);
extern gint vfs_fclose(VFSFile * file);

extern size_t vfs_fread(gpointer ptr,
                 size_t size,
                 size_t nmemb,
                 VFSFile * file);
extern size_t vfs_fwrite(gconstpointer ptr,
                  size_t size,
                  size_t nmemb,
                  VFSFile *file);

extern gint vfs_getc(VFSFile *stream);
extern gint vfs_ungetc(gint c,
                       VFSFile *stream);
extern gchar *vfs_fgets(gchar *s,
                        gint n,
                        VFSFile *stream);

extern gint vfs_fseek(VFSFile * file,
               glong offset,
               gint whence);
extern void vfs_rewind(VFSFile * file);
extern glong vfs_ftell(VFSFile * file);
extern gboolean vfs_feof(VFSFile * file);

extern gboolean vfs_file_test(const gchar * path,
                       GFileTest test);

extern gboolean vfs_is_writeable(const gchar * path);

extern gboolean vfs_truncate(VFSFile * file, glong length);

extern int vfs_fprintf(VFSFile *stream, gchar const *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

G_END_DECLS

#endif /* VFS_H */
