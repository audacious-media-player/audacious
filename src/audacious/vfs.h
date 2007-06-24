/*
 * audacious: Cross-platform multimedia player.
 * vfs.h: VFS subsystem core commands.                 
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

#ifndef VFS_H
#define VFS_H

#include <glib.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct _VFSFile VFSFile;
typedef struct _VFSConstructor VFSConstructor;

/**
 * VFSFile:
 * @uri: The URI of the stream.
 * @handle: Opaque data used by the transport plugins.
 * @base: The base vtable used for VFS functions.
 * @ref: The amount of references that the VFSFile object has.
 *
 * #VFSFile objects describe a VFS stream.
 **/
struct _VFSFile {
	gchar *uri;
	gpointer handle;
	VFSConstructor *base;
	gint ref;
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
	off_t (*vfs_fsize_impl)(VFSFile *file);
	gchar *(*vfs_get_metadata_impl)(VFSFile *file, const gchar * field);
};

G_BEGIN_DECLS

extern VFSFile * vfs_fopen(const gchar * path,
                    const gchar * mode);
extern gint vfs_fclose(VFSFile * file);

extern VFSFile * vfs_dup(VFSFile *in);

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

extern off_t vfs_fsize(VFSFile * file);

extern gchar *vfs_get_metadata(VFSFile * file, const gchar * field);

extern int vfs_fprintf(VFSFile *stream, gchar const *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

extern gboolean vfs_register_transport(VFSConstructor *vtable);

extern void vfs_file_get_contents(const gchar *filename, gchar **buf, gsize *size);

G_END_DECLS

#endif /* VFS_H */
