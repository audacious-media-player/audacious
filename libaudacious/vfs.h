/*
 * Audacious
 * Copyright (c) 2006 Audacious team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
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
typedef struct _VFSConstructor VFSConstructor;

/**
 * VFSFile:
 * @uri: The URI of the stream.
 * @handle: Opaque data used by the transport plugins.
 * @base: The base vtable used for VFS functions.
 *
 * #VFSFile objects describe a VFS stream.
 **/
struct _VFSFile {
	gchar *uri;
	gpointer handle;
	VFSConstructor *base;
};

/**
 * VFSConstructor:
 * @uri_id: The uri identifier, e.g. "file" would handle "file://" streams.
 * @vfs_fopen_impl: A function pointer which points to a fopen implementation.
 * @vfs_fclose_impl: A function pointer which points to a fclose implementation.
 * @vfs_fread_impl: A function pointer which points to a fread implementation.
 * @vfs_fwrite_impl: A function pointer which points to a fwrite implementation.
 * @vfs_getc_impl: A function pointer which points to a getc implementation.
 * @vfs_ungetc_impl: A function pointer which points to an ungetc implementation.
 * @vfs_fseek_impl: A function pointer which points to a fseek implementation.
 * @vfs_rewind_impl: A function pointer which points to a rewind implementation.
 * @vfs_ftell_impl: A function pointer which points to a ftell implementation.
 * @vfs_feof_impl: A function pointer which points to a feof implementation.
 * @vfs_truncate_impl: A function pointer which points to a ftruncate implementation.
 *
 * #VFSConstructor objects contain the base vtables used for extrapolating
 * a VFS stream. #VFSConstructor objects should be considered %virtual in
 * nature. VFS base vtables are registered via vfs_register_transport().
 **/
struct _VFSConstructor {
	gchar *uri_id;
	VFSFile *(*vfs_fopen_impl)(const gchar *path,
		const gchar *mode);
	gint (*vfs_fclose_impl)(VFSFile * file);
	size_t (*vfs_fread_impl)(gpointer ptr, size_t size,
		size_t nmemb, VFSFile *file);
	size_t (*vfs_fwrite_impl)(gconstpointer ptr, size_t size,
		size_t nmemb, VFSFile *file);
	gint (*vfs_getc_impl)(VFSFile *stream);
	gint (*vfs_ungetc_impl)(gint c, VFSFile *stream);
	gint (*vfs_fseek_impl)(VFSFile *file, glong offset, gint whence);
	void (*vfs_rewind_impl)(VFSFile *file);
	glong (*vfs_ftell_impl)(VFSFile *file);
	gboolean (*vfs_feof_impl)(VFSFile *file);
	gboolean (*vfs_truncate_impl)(VFSFile *file, glong length);
};

G_BEGIN_DECLS

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

extern gboolean vfs_register_transport(VFSConstructor *vtable);

G_END_DECLS

#endif /* VFS_H */
