/*
 * vfs.h
 * Copyright 2006-2013 Ariadne Conill, Daniel Barkalow, Ralf Ertzinger,
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

#include <libaudcore/export.h>
#include <libaudcore/index.h>
#include <libaudcore/objects.h>

enum VFSFileTest
{
    VFS_IS_REGULAR = (1 << 0),
    VFS_IS_SYMLINK = (1 << 1),
    VFS_IS_DIR = (1 << 2),
    VFS_IS_EXECUTABLE = (1 << 3),
    VFS_EXISTS = (1 << 4),
    VFS_NO_ACCESS = (1 << 5)
};

enum VFSReadOptions
{
    VFS_APPEND_NULL = (1 << 0),
    VFS_IGNORE_MISSING = (1 << 1)
};

enum VFSSeekType
{
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2,
    /* This value exists only to translate out-of-range values from external
     * libraries/APIs (e.g. POSIX). Avoid using it. For validity checks, use an
     * else/default case rather than testing equality with VFS_SEEK_INVALID. */
    VFS_SEEK_INVALID = -1
};

#ifdef WANT_VFS_STDIO_COMPAT

#include <stdio.h>

constexpr int from_vfs_seek_type(VFSSeekType whence)
{
    return (whence == VFS_SEEK_SET)   ? SEEK_SET
           : (whence == VFS_SEEK_CUR) ? SEEK_CUR
           : (whence == VFS_SEEK_END) ? SEEK_END
                                      : -1;
}

constexpr VFSSeekType to_vfs_seek_type(int whence)
{
    return (whence == SEEK_SET)   ? VFS_SEEK_SET
           : (whence == SEEK_CUR) ? VFS_SEEK_CUR
           : (whence == SEEK_END) ? VFS_SEEK_END
                                  : VFS_SEEK_INVALID;
}

#endif // WANT_VFS_STDIO_COMPAT

// #undef POSIX functions/macros to avoid name conflicts
#undef fread
#undef fseek
#undef ftell
#undef fsize
#undef feof
#undef fwrite
#undef ftruncate
#undef fflush

class LIBAUDCORE_PUBLIC VFSImpl
{
public:
    VFSImpl() {}
    virtual ~VFSImpl() {}

    VFSImpl(const VFSImpl &) = delete;
    VFSImpl & operator=(const VFSImpl &) = delete;

    virtual int64_t fread(void * ptr, int64_t size, int64_t nmemb) = 0;
    virtual int fseek(int64_t offset, VFSSeekType whence) = 0;

    virtual int64_t ftell() = 0;
    virtual int64_t fsize() = 0;
    virtual bool feof() = 0;

    virtual int64_t fwrite(const void * ptr, int64_t size, int64_t nmemb) = 0;
    virtual int ftruncate(int64_t length) = 0;
    virtual int fflush() = 0;

    virtual String get_metadata(const char * field) { return String(); }
};

class VFSFile
{
public:
    VFSFile() {}

    VFSFile(const char * filename, VFSImpl * impl)
        : m_filename(filename), m_impl(impl)
    {
    }

    VFSFile(const char * filename, const char * mode);

    /* creates a temporary file (deleted when closed) */
    static VFSFile tmpfile();

    explicit operator bool() const { return (bool)m_impl; }
    const char * filename() const { return m_filename; }
    const char * error() const { return m_error; }

    /* basic operations */

    int64_t fread(void * ptr, int64_t size, int64_t nmemb)
        __attribute__((warn_unused_result));
    int fseek(int64_t offset, VFSSeekType whence)
        __attribute__((warn_unused_result));

    int64_t ftell();
    int64_t fsize();
    bool feof();

    int64_t fwrite(const void * ptr, int64_t size, int64_t nmemb)
        __attribute__((warn_unused_result));
    int ftruncate(int64_t length) __attribute__((warn_unused_result));
    int fflush() __attribute__((warn_unused_result));

    /* used to read e.g. ICY metadata */
    String get_metadata(const char * field);

    /* the VFS layer buffers up to 256 KB of data at the beginning of files
     * opened in read-only mode; this function disallows reading outside the
     * buffered region (useful for probing the file type) */
    void set_limit_to_buffer(bool limit);

    /* utility functions */

    /* reads the entire file into memory (limited to 256 MiB) */
    Index<char> read_all();

    /* reads data from another open file and appends it to this one */
    bool copy_from(VFSFile & source, int64_t size = -1);

    /* overwrites the entire file with the contents of another */
    bool replace_with(VFSFile & source);

    /* tests certain attributes of a file without opening it.
     * the 2-argument version returns true if all requested tests passed.
     * the 3-argument version returns a bitmask indicating which tests passed.
     */
    static bool test_file(const char * filename, VFSFileTest test);
    static VFSFileTest test_file(const char * filename, VFSFileTest test,
                                 String & error);

    /* returns a sorted list of folder entries (as full URIs) */
    static Index<String> read_folder(const char * filename, String & error);

    /* convenience functions to read/write entire files */
    static Index<char> read_file(const char * filename, VFSReadOptions options);
    static bool write_file(const char * filename, const void * data,
                           int64_t len);

    /* returns a list of supported URI schemes */
    static Index<const char *> supported_uri_schemes();

private:
    String m_filename, m_error;
    SmartPtr<VFSImpl> m_impl;
};

#endif /* LIBAUDCORE_VFS_H */
