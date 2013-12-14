/*
 * vfs.h
 * Copyright 2006-2013 William Pitcock, Daniel Barkalow, Ralf Ertzinger,
 *                     Yoshiki Yazawa, Matti Hämäläinen, and John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */
/**
 * @file vfs.h
 * Main API header for accessing Audacious VFS functionality.
 * Provides functions for VFS transport registration and
 * file access.
 */

#ifndef LIBAUDCORE_VFS_H
#define LIBAUDCORE_VFS_H

#include <stdint.h>

#include <libaudcore/core.h>

/* equivalent to G_FILE_TEST_XXX */
#define VFS_IS_REGULAR    (1 << 0)
#define VFS_IS_SYMLINK    (1 << 1)
#define VFS_IS_DIR        (1 << 2)
#define VFS_IS_EXECUTABLE (1 << 3)
#define VFS_EXISTS        (1 << 4)

/** @struct VFSFile */
typedef struct _VFSFile VFSFile;
/** @struct VFSConstructor */
typedef const struct _VFSConstructor VFSConstructor;

/**
 * @struct _VFSConstructor
 * #VFSConstructor objects contain the base vtables used for extrapolating
 * a VFS stream. #VFSConstructor objects should be considered %virtual in
 * nature. VFS base vtables are registered via vfs_register_transport().
 */
struct _VFSConstructor {
    /** A function pointer which points to a fopen implementation. */
    void * (* vfs_fopen_impl) (const char * filename, const char * mode);
    /** A function pointer which points to a fclose implementation. */
    int (* vfs_fclose_impl) (VFSFile * file);

    /** A function pointer which points to a fread implementation. */
    int64_t (* vfs_fread_impl) (void * ptr, int64_t size, int64_t nmemb, VFSFile *
     file);
    /** A function pointer which points to a fwrite implementation. */
    int64_t (* vfs_fwrite_impl) (const void * ptr, int64_t size, int64_t nmemb,
     VFSFile * file);

    void (* obs_getc) (void); // obsolete
    void (* obs_ungetc) (void); // obsolete

    /** A function pointer which points to a fseek implementation. */
    int (* vfs_fseek_impl) (VFSFile * file, int64_t offset, int whence);

    void (* obs_rewind) (void); // obsolete

    /** A function pointer which points to a ftell implementation. */
    int64_t (* vfs_ftell_impl) (VFSFile * file);
    /** A function pointer which points to a feof implementation. */
    bool_t (* vfs_feof_impl) (VFSFile * file);
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

VFSFile * vfs_new (const char * path, VFSConstructor * vtable, void * handle) WARN_RETURN;
const char * vfs_get_filename (VFSFile * file) WARN_RETURN;
void * vfs_get_handle (VFSFile * file) WARN_RETURN;

VFSFile * vfs_fopen (const char * path, const char * mode) WARN_RETURN;
int vfs_fclose (VFSFile * file);

int64_t vfs_fread (void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
 WARN_RETURN;
int64_t vfs_fwrite (const void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
 WARN_RETURN;

int vfs_getc (VFSFile * stream) WARN_RETURN;
int vfs_ungetc (int c, VFSFile * stream) WARN_RETURN;
char * vfs_fgets (char * s, int n, VFSFile * stream) WARN_RETURN;
int vfs_fputs (const char * s, VFSFile * stream) WARN_RETURN;
bool_t vfs_feof (VFSFile * file) WARN_RETURN;
int vfs_fprintf (VFSFile * stream, char const * format, ...) __attribute__
 ((__format__ (__printf__, 2, 3)));

int vfs_fseek (VFSFile * file, int64_t offset, int whence) WARN_RETURN;
int64_t vfs_ftell (VFSFile * file) WARN_RETURN;
int64_t vfs_fsize (VFSFile * file) WARN_RETURN;
int vfs_ftruncate (VFSFile * file, int64_t length) WARN_RETURN;

bool_t vfs_is_streaming (VFSFile * file) WARN_RETURN;

/* free returned string with str_unref() */
char * vfs_get_metadata (VFSFile * file, const char * field) WARN_RETURN;

bool_t vfs_file_test (const char * path, int test) WARN_RETURN;
bool_t vfs_is_writeable (const char * path) WARN_RETURN;
bool_t vfs_is_remote (const char * path) WARN_RETURN;

void vfs_file_read_all (VFSFile * file, void * * buf, int64_t * size);
void vfs_file_get_contents (const char * filename, void * * buf, int64_t * size);

void vfs_set_lookup_func (VFSConstructor * (* func) (const char * scheme));
void vfs_set_verbose (bool_t verbose);

#undef WARN_RETURN

#endif /* LIBAUDCORE_VFS_H */
