/*
 * Audacious
 * Copyright (c) 2006-2007 Audacious team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */
/**
 * @file vfs.h
 * Main API header for accessing Audacious VFS functionality.
 * Provides functions for VFS transport registration and
 * file access.
 */

#ifndef AUDACIOUS_VFS_H
#define AUDACIOUS_VFS_H

#include <glib.h>
#include <stdio.h>
#include <sys/types.h>

G_BEGIN_DECLS

/** @struct VFSFile */
typedef struct _VFSFile VFSFile;
/** @struct VFSConstructor */
typedef struct _VFSConstructor VFSConstructor;

/**
 * @struct _VFSFile
 * #VFSFile objects describe an opened VFS stream, basically being
 * similar in purpose as stdio FILE
 */
struct _VFSFile {
	gchar *uri;             /**< The URI of the stream */
	gpointer handle;        /**< Opaque data used by the transport plugins */
	VFSConstructor *base;   /**< The base vtable used for VFS functions */
	gint ref;               /**< The amount of references that the VFSFile object has */
};

/** 
 * @struct _VFSConstructor
 * #VFSConstructor objects contain the base vtables used for extrapolating
 * a VFS stream. #VFSConstructor objects should be considered %virtual in
 * nature. VFS base vtables are registered via vfs_register_transport().
 */
struct _VFSConstructor {
    /** The URI identifier, e.g. "file" would handle "file://" streams. */
	gchar *uri_id;
	/** A function pointer which points to a fopen implementation. */
	VFSFile *(*vfs_fopen_impl)(const gchar *path,
		const gchar *mode);
    /** A function pointer which points to a fclose implementation. */
	gint (*vfs_fclose_impl)(VFSFile * file);
	/** A function pointer which points to a fread implementation. */
	gsize (*vfs_fread_impl)(gpointer ptr, gsize size,
		gsize nmemb, VFSFile *file);
    /** A function pointer which points to a fwrite implementation. */
	gsize (*vfs_fwrite_impl)(gconstpointer ptr, gsize size,
		gsize nmemb, VFSFile *file);
    /** A function pointer which points to a getc implementation. */
	gint (*vfs_getc_impl)(VFSFile *stream);
    /** A function pointer which points to an ungetc implementation. */
	gint (*vfs_ungetc_impl)(gint c, VFSFile *stream);
    /** A function pointer which points to a fseek implementation. */
	gint (*vfs_fseek_impl)(VFSFile *file, glong offset, gint whence);
	/** function pointer which points to a rewind implementation. */
	void (*vfs_rewind_impl)(VFSFile *file);
    /** A function pointer which points to a ftell implementation. */
	glong (*vfs_ftell_impl)(VFSFile *file);
	/** A function pointer which points to a feof implementation. */
	gboolean (*vfs_feof_impl)(VFSFile *file);
	/** A function pointer which points to a ftruncate implementation. */
	gboolean (*vfs_truncate_impl)(VFSFile *file, glong length);
	/** A function pointer which points to a fsize implementation. */
	off_t (*vfs_fsize_impl)(VFSFile *file);
	/** A function pointer which points to a (stream) metadata fetching implementation. */
	gchar *(*vfs_get_metadata_impl)(VFSFile *file, const gchar * field);
};


extern VFSFile * vfs_fopen(const gchar * path,
                    const gchar * mode);
extern gint vfs_fclose(VFSFile * file);

extern VFSFile * vfs_dup(VFSFile *in);

extern gsize vfs_fread(gpointer ptr,
                 gsize size,
                 gsize nmemb,
                 VFSFile * file);
extern gsize vfs_fwrite(gconstpointer ptr,
                  gsize size,
                  gsize nmemb,
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

extern off_t vfs_fsize(VFSFile * file);

extern gchar *vfs_get_metadata(VFSFile * file, const gchar * field);

extern gint vfs_fprintf(VFSFile *stream, gchar const *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

extern gboolean vfs_register_transport(VFSConstructor *vtable);

extern void vfs_file_get_contents(const gchar *filename, gchar **buf, gsize *size);

extern gboolean vfs_is_remote(const gchar * path);

extern gboolean vfs_is_streaming(VFSFile *file);

extern gboolean vfs_fget_le16(guint16 *value, VFSFile *stream);
extern gboolean vfs_fget_le32(guint32 *value, VFSFile *stream);
extern gboolean vfs_fget_le64(guint64 *value, VFSFile *stream);
extern gboolean vfs_fget_be16(guint16 *value, VFSFile *stream);
extern gboolean vfs_fget_be32(guint32 *value, VFSFile *stream);
extern gboolean vfs_fget_be64(guint64 *value, VFSFile *stream);

extern gboolean vfs_fput_le16(guint16 value, VFSFile *stream);
extern gboolean vfs_fput_le32(guint32 value, VFSFile *stream);
extern gboolean vfs_fput_le64(guint64 value, VFSFile *stream);
extern gboolean vfs_fput_be16(guint16 value, VFSFile *stream);
extern gboolean vfs_fput_be32(guint32 value, VFSFile *stream);
extern gboolean vfs_fput_be64(guint64 value, VFSFile *stream);

G_END_DECLS

#endif /* AUDACIOUS_VFS_H */
