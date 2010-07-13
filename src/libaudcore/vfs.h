/*
 * Audacious
 * Copyright (c) 2006-2010 Audacious team
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
    gchar * uri_id;

    /** A function pointer which points to a fopen implementation. */
    VFSFile * (* vfs_fopen_impl) (const gchar * filename, const gchar * mode);
    /** A function pointer which points to a fclose implementation. */
    gint (* vfs_fclose_impl) (VFSFile * file);

    /** A function pointer which points to a fread implementation. */
    gint64 (* vfs_fread_impl) (void * ptr, gint64 size, gint64 nmemb, VFSFile *
     file);
    /** A function pointer which points to a fwrite implementation. */
    gint64 (* vfs_fwrite_impl) (const void * ptr, gint64 size, gint64 nmemb,
     VFSFile * file);

    /** A function pointer which points to a getc implementation. */
    gint (* vfs_getc_impl) (VFSFile * stream);
    /** A function pointer which points to an ungetc implementation. */
    gint (* vfs_ungetc_impl) (gint c, VFSFile * stream);

    /** A function pointer which points to a fseek implementation. */
    gint (* vfs_fseek_impl) (VFSFile * file, gint64 offset, gint whence);
    /** function pointer which points to a rewind implementation. */
    void (* vfs_rewind_impl) (VFSFile * file);
    /** A function pointer which points to a ftell implementation. */
    gint64 (* vfs_ftell_impl) (VFSFile * file);
    /** A function pointer which points to a feof implementation. */
    gboolean (* vfs_feof_impl) (VFSFile * file);
    /** A function pointer which points to a ftruncate implementation. */
    gint (* vfs_ftruncate_impl) (VFSFile * file, gint64 length);
    /** A function pointer which points to a fsize implementation. */
    gint64 (* vfs_fsize_impl) (VFSFile * file);

    /** A function pointer which points to a (stream) metadata fetching implementation. */
    gchar * (* vfs_get_metadata_impl) (VFSFile * file, const gchar * field);
};

#ifdef __GNUC__
#define WARN_RETURN /* __attribute__ ((warn_unused_result)) */
#else
#define WARN_RETURN
#endif

VFSFile * vfs_fopen (const gchar * path, const gchar * mode) WARN_RETURN;
VFSFile * vfs_dup (VFSFile * in) WARN_RETURN;
gint vfs_fclose (VFSFile * file);

gint64 vfs_fread (void * ptr, gint64 size, gint64 nmemb, VFSFile * file)
 WARN_RETURN;
gint64 vfs_fwrite (const void * ptr, gint64 size, gint64 nmemb, VFSFile * file)
 WARN_RETURN;

gint vfs_getc (VFSFile * stream) WARN_RETURN;
gint vfs_ungetc (gint c, VFSFile * stream) WARN_RETURN;
gchar * vfs_fgets (gchar * s, gint n, VFSFile * stream) WARN_RETURN;
gboolean vfs_feof (VFSFile * file) WARN_RETURN;
gint vfs_fprintf (VFSFile * stream, gchar const * format, ...) __attribute__
 ((__format__ (__printf__, 2, 3)));

gint vfs_fseek (VFSFile * file, gint64 offset, gint whence) WARN_RETURN;
void vfs_rewind (VFSFile * file);
glong vfs_ftell (VFSFile * file) WARN_RETURN;
gint64 vfs_fsize (VFSFile * file) WARN_RETURN;
gint vfs_ftruncate (VFSFile * file, gint64 length) WARN_RETURN;

gboolean vfs_fget_le16 (guint16 * value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fget_le32 (guint32 * value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fget_le64 (guint64 * value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fget_be16 (guint16 * value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fget_be32 (guint32 * value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fget_be64 (guint64 * value, VFSFile * stream) WARN_RETURN;

gboolean vfs_fput_le16 (guint16 value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fput_le32 (guint32 value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fput_le64 (guint64 value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fput_be16 (guint16 value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fput_be32 (guint32 value, VFSFile * stream) WARN_RETURN;
gboolean vfs_fput_be64 (guint64 value, VFSFile * stream) WARN_RETURN;

gboolean vfs_is_streaming (VFSFile * file) WARN_RETURN;
gchar * vfs_get_metadata (VFSFile * file, const gchar * field) WARN_RETURN;

gboolean vfs_file_test (const gchar * path, GFileTest test) WARN_RETURN;
gboolean vfs_is_writeable (const gchar * path) WARN_RETURN;
gboolean vfs_is_remote (const gchar * path) WARN_RETURN;

void vfs_file_get_contents (const gchar * filename, void * * buf, gint64 *
 size);

void vfs_register_transport (VFSConstructor * vtable);

#undef WARN_RETURN

G_END_DECLS

#endif /* AUDACIOUS_VFS_H */
