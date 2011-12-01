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
#include <stdint.h>
#include <sys/types.h>

#include <libaudcore/types.h>

G_BEGIN_DECLS

/** @struct VFSFile */
typedef struct _VFSFile VFSFile;
/** @struct VFSConstructor */
typedef const struct _VFSConstructor VFSConstructor;

#define VFS_SIG ('V' | ('F' << 8) | ('S' << 16))

/**
 * @struct _VFSFile
 * #VFSFile objects describe an opened VFS stream, basically being
 * similar in purpose as stdio FILE
 */
struct _VFSFile {
    char *uri;             /**< The URI of the stream */
    void * handle;        /**< Opaque data used by the transport plugins */
    VFSConstructor *base;   /**< The base vtable used for VFS functions */
    int ref;               /**< The amount of references that the VFSFile object has */
    int sig;               /**< Used to detect invalid or twice-closed objects */
};

/**
 * @struct _VFSConstructor
 * #VFSConstructor objects contain the base vtables used for extrapolating
 * a VFS stream. #VFSConstructor objects should be considered %virtual in
 * nature. VFS base vtables are registered via vfs_register_transport().
 */
struct _VFSConstructor {
    /** A function pointer which points to a fopen implementation. */
    VFSFile * (* vfs_fopen_impl) (const char * filename, const char * mode);
    /** A function pointer which points to a fclose implementation. */
    int (* vfs_fclose_impl) (VFSFile * file);

    /** A function pointer which points to a fread implementation. */
    int64_t (* vfs_fread_impl) (void * ptr, int64_t size, int64_t nmemb, VFSFile *
     file);
    /** A function pointer which points to a fwrite implementation. */
    int64_t (* vfs_fwrite_impl) (const void * ptr, int64_t size, int64_t nmemb,
     VFSFile * file);

    /** A function pointer which points to a getc implementation. */
    int (* vfs_getc_impl) (VFSFile * stream);
    /** A function pointer which points to an ungetc implementation. */
    int (* vfs_ungetc_impl) (int c, VFSFile * stream);

    /** A function pointer which points to a fseek implementation. */
    int (* vfs_fseek_impl) (VFSFile * file, int64_t offset, int whence);
    /** function pointer which points to a rewind implementation. */
    void (* vfs_rewind_impl) (VFSFile * file);
    /** A function pointer which points to a ftell implementation. */
    int64_t (* vfs_ftell_impl) (VFSFile * file);
    /** A function pointer which points to a feof implementation. */
    boolean (* vfs_feof_impl) (VFSFile * file);
    /** A function pointer which points to a ftruncate implementation. */
    int (* vfs_ftruncate_impl) (VFSFile * file, int64_t length);
    /** A function pointer which points to a fsize implementation. */
    int64_t (* vfs_fsize_impl) (VFSFile * file);

    /** A function pointer which points to a (stream) metadata fetching implementation. */
    char * (* vfs_get_metadata_impl) (VFSFile * file, const char * field);
};

#ifdef __GNUC__
#define WARN_RETURN __attribute__ ((warn_unused_result))
#else
#define WARN_RETURN
#endif

VFSFile * vfs_fopen (const char * path, const char * mode) WARN_RETURN;
VFSFile * vfs_dup (VFSFile * in) WARN_RETURN;
int vfs_fclose (VFSFile * file);

int64_t vfs_fread (void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
 WARN_RETURN;
int64_t vfs_fwrite (const void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
 WARN_RETURN;

int vfs_getc (VFSFile * stream) WARN_RETURN;
int vfs_ungetc (int c, VFSFile * stream) WARN_RETURN;
char * vfs_fgets (char * s, int n, VFSFile * stream) WARN_RETURN;
boolean vfs_feof (VFSFile * file) WARN_RETURN;
int vfs_fprintf (VFSFile * stream, char const * format, ...) __attribute__
 ((__format__ (__printf__, 2, 3)));

int vfs_fseek (VFSFile * file, int64_t offset, int whence) WARN_RETURN;
void vfs_rewind (VFSFile * file);
int64_t vfs_ftell (VFSFile * file) WARN_RETURN;
int64_t vfs_fsize (VFSFile * file) WARN_RETURN;
int vfs_ftruncate (VFSFile * file, int64_t length) WARN_RETURN;

boolean vfs_fget_le16 (uint16_t * value, VFSFile * stream) WARN_RETURN;
boolean vfs_fget_le32 (uint32_t * value, VFSFile * stream) WARN_RETURN;
boolean vfs_fget_le64 (uint64_t * value, VFSFile * stream) WARN_RETURN;
boolean vfs_fget_be16 (uint16_t * value, VFSFile * stream) WARN_RETURN;
boolean vfs_fget_be32 (uint32_t * value, VFSFile * stream) WARN_RETURN;
boolean vfs_fget_be64 (uint64_t * value, VFSFile * stream) WARN_RETURN;

boolean vfs_fput_le16 (uint16_t value, VFSFile * stream) WARN_RETURN;
boolean vfs_fput_le32 (uint32_t value, VFSFile * stream) WARN_RETURN;
boolean vfs_fput_le64 (uint64_t value, VFSFile * stream) WARN_RETURN;
boolean vfs_fput_be16 (uint16_t value, VFSFile * stream) WARN_RETURN;
boolean vfs_fput_be32 (uint32_t value, VFSFile * stream) WARN_RETURN;
boolean vfs_fput_be64 (uint64_t value, VFSFile * stream) WARN_RETURN;

boolean vfs_is_streaming (VFSFile * file) WARN_RETURN;
char * vfs_get_metadata (VFSFile * file, const char * field) WARN_RETURN;

boolean vfs_file_test (const char * path, GFileTest test) WARN_RETURN;
boolean vfs_is_writeable (const char * path) WARN_RETURN;
boolean vfs_is_remote (const char * path) WARN_RETURN;

void vfs_file_get_contents (const char * filename, void * * buf, int64_t *
 size);

void vfs_set_lookup_func (VFSConstructor * (* func) (const char * scheme));
void vfs_set_verbose (boolean verbose);

#undef WARN_RETURN

G_END_DECLS

#endif /* AUDACIOUS_VFS_H */
