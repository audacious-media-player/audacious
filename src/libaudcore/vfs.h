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

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

enum VFSFileTest {
    VFS_IS_REGULAR    = (1 << 0),
    VFS_IS_SYMLINK    = (1 << 1),
    VFS_IS_DIR        = (1 << 2),
    VFS_IS_EXECUTABLE = (1 << 3),
    VFS_EXISTS        = (1 << 4)
};

enum VFSSeekType {
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2
};

#ifdef WANT_VFS_STDIO_COMPAT

#include <stdio.h>

constexpr int from_vfs_seek_type (VFSSeekType whence)
{
    return (whence == VFS_SEEK_SET) ? SEEK_SET :
           (whence == VFS_SEEK_CUR) ? SEEK_CUR :
           (whence == VFS_SEEK_END) ? SEEK_END : -1;
}

constexpr VFSSeekType to_vfs_seek_type (int whence)
{
    return (whence == SEEK_SET) ? VFS_SEEK_SET :
           (whence == SEEK_CUR) ? VFS_SEEK_CUR :
           (whence == SEEK_END) ? VFS_SEEK_END : (VFSSeekType) -1;
}

#endif // WANT_VFS_STDIO_COMPAT

class VFSImpl
{
public:
    VFSImpl () {}
    virtual ~VFSImpl () {}

    VFSImpl (const VFSImpl &) = delete;
    VFSImpl & operator= (const VFSImpl &) = delete;

    virtual int64_t fread (void * ptr, int64_t size, int64_t nmemb) = 0;
    virtual int fseek (int64_t offset, VFSSeekType whence) = 0;

    virtual int64_t ftell () = 0;
    virtual int64_t fsize () = 0;
    virtual bool feof () = 0;

    virtual int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb) = 0;
    virtual int ftruncate (int64_t length) = 0;
    virtual int fflush () = 0;

    virtual String get_metadata (const char * field) { return String (); }
};

class VFSFile
{
public:
    constexpr VFSFile () {}

    VFSFile (const char * filename, VFSImpl * impl) :
        m_filename (filename),
        m_impl (impl) {}

    VFSFile (const char * filename, const char * mode);

    explicit operator bool () const
        { return (bool) m_impl; }
    const char * filename () const
        { return m_filename; }

    int64_t fread (void * ptr, int64_t size, int64_t nmemb) __attribute__ ((warn_unused_result));
    int fseek (int64_t offset, VFSSeekType whence) __attribute__ ((warn_unused_result));

    int64_t ftell ();
    int64_t fsize ();
    bool feof ();

    int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb) __attribute__ ((warn_unused_result));
    int ftruncate (int64_t length) __attribute__ ((warn_unused_result));
    int fflush () __attribute__ ((warn_unused_result));

    String get_metadata (const char * field);

    /* for internal use only */
    typedef VFSImpl * (* OpenFunc) (const char * filename, const char * mode);
    typedef OpenFunc (* LookupFunc) (const char * scheme);

    static void set_lookup_func (LookupFunc func);

private:
    String m_filename;
    SmartPtr<VFSImpl> m_impl;
};

/* old-style function wrappers */
static inline const char * vfs_get_filename (VFSFile * file)
    { return file->filename (); }

static inline VFSFile * vfs_fopen (const char * path, const char * mode)
{
    auto file = new VFSFile (path, mode);
    if (* file)
        return file;
    delete file;
    return nullptr;
}

static inline int vfs_fclose (VFSFile * file)
{
    int ret = file->fflush ();
    delete file;
    return ret;
}

static inline int64_t vfs_fread (void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
    { return file->fread (ptr, size, nmemb); }
static inline int64_t vfs_fwrite (const void * ptr, int64_t size, int64_t nmemb, VFSFile * file)
    { return file->fwrite (ptr, size, nmemb); }
static inline int vfs_fseek (VFSFile * file, int64_t offset, VFSSeekType whence)
    { return file->fseek (offset, whence); }
static inline int64_t vfs_ftell (VFSFile * file)
    { return file->ftell (); }
static inline int64_t vfs_fsize (VFSFile * file)
    { return file->fsize (); }
static inline bool vfs_feof (VFSFile * file)
    { return file->feof (); }
static inline int vfs_ftruncate (VFSFile * file, int64_t length)
    { return file->ftruncate (length); }
static inline String vfs_get_metadata (VFSFile * file, const char * field)
    { return file->get_metadata (field); }

/* utility functions */
int vfs_getc (VFSFile * stream);
int vfs_ungetc (int c, VFSFile * stream);
char * vfs_fgets (char * s, int n, VFSFile * stream);
int vfs_fputs (const char * s, VFSFile * stream);
int vfs_fprintf (VFSFile * stream, char const * format, ...) __attribute__
 ((__format__ (__printf__, 2, 3)));

bool vfs_is_streaming (VFSFile * file);

bool vfs_file_test (const char * path, VFSFileTest test);
bool vfs_is_writeable (const char * path);
bool vfs_is_remote (const char * path);

Index<char> vfs_file_read_all (VFSFile * file);
Index<char> vfs_file_get_contents (const char * filename);

#endif /* LIBAUDCORE_VFS_H */
