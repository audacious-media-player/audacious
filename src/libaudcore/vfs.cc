/*
 * vfs.c
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

#include "vfs.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"
#include "runtime.h"
#include "vfs_local.h"

/* Audacious core provides us with a function that looks up a VFS transport for
 * a given URI scheme.  This function will load plugins as needed. */

static VFSFile::LookupFunc lookup_func;

EXPORT void VFSFile::set_lookup_func (LookupFunc func)
{
    lookup_func = func;
}

/**
 * Opens a stream from a VFS transport using one of the registered
 * #VFSConstructor handlers.
 *
 * @param path The path or URI to open.
 * @param mode The preferred access privileges (not guaranteed).
 * @return On success, a #VFSFile object representing the stream.
 */
EXPORT VFSFile * VFSFile::fopen (const char * path, const char * mode)
{
    g_return_val_if_fail (path && mode, nullptr);

    OpenFunc fopen_impl = nullptr;

    if (! strncmp (path, "file://", 7))
        fopen_impl = vfs_local_fopen;
    else
    {
        const char * s = strstr (path, "://");

        if (! s)
        {
            AUDERR ("Invalid URI: %s\n", path);
            return nullptr;
        }

        StringBuf scheme = str_copy (path, s - path);

        if (! (fopen_impl = lookup_func (scheme)))
        {
            AUDERR ("Unknown URI scheme: %s://", (const char *) scheme);
            return nullptr;
        }
    }

    const char * sub;
    uri_parse (path, nullptr, nullptr, & sub, nullptr);

    StringBuf buf = str_copy (path, sub - path);

    VFSFile * file = fopen_impl (buf, mode);

    if (file)
        AUDINFO ("<%p> open (mode %s) %s\n", file, mode, path);

    return file;
}

/**
 * Reads from a VFS stream.
 *
 * @param ptr A pointer to the destination buffer.
 * @param size The size of each element to read.
 * @param nmemb The number of elements to read.
 * @param file #VFSFile object that represents the VFS stream.
 * @return The number of elements succesfully read.
 */
EXPORT int64_t VFSFile::fread (void * ptr, int64_t size, int64_t nmemb)
{
    int64_t readed = fread_impl (ptr, size, nmemb);

    AUDDBG ("<%p> read %" PRId64 " elements of size %" PRId64 " = %" PRId64 "\n",
     this, nmemb, size, readed);

    return readed;
}

/**
 * Writes to a VFS stream.
 *
 * @param ptr A const pointer to the source buffer.
 * @param size The size of each element to write.
 * @param nmemb The number of elements to write.
 * @param file #VFSFile object that represents the VFS stream.
 * @return The number of elements succesfully written.
 */
EXPORT int64_t VFSFile::fwrite (const void * ptr, int64_t size, int64_t nmemb)
{
    int64_t written = fwrite_impl (ptr, size, nmemb);

    AUDDBG ("<%p> write %" PRId64 " elements of size %" PRId64 " = %" PRId64 "\n",
     this, nmemb, size, written);

    return written;
}

/**
 * Reads a character from a VFS stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, a character. Otherwise, -1.
 */
EXPORT int
vfs_getc(VFSFile *file)
{
    unsigned char c;

    if (vfs_fread (& c, 1, 1, file) != 1)
        return -1;

    return c;
}

/**
 * Pushes a character back to the VFS stream.
 *
 * @param c The character to push back.
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, 0. Otherwise, EOF.
 */
EXPORT int
vfs_ungetc(int c, VFSFile *file)
{
    if (vfs_fseek (file, -1, VFS_SEEK_CUR) < 0)
        return EOF;

    return c;
}

/**
 * Performs a seek in given VFS stream. Standard C-style values
 * of whence can be used to indicate desired action.
 *
 * - SEEK_CUR seeks relative to current stream position.
 * - SEEK_SET seeks to given absolute position (relative to stream beginning).
 * - SEEK_END sets stream position to current file end.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param offset The offset to seek to.
 * @param whence Type of the seek: SEEK_CUR, SEEK_SET or SEEK_END.
 * @return On success, 0. Otherwise, -1.
 */
EXPORT int VFSFile::fseek (int64_t offset, VFSSeekType whence)
{
    AUDDBG ("<%p> seek to %" PRId64 " from %s\n", this, offset,
         whence == SEEK_CUR ? "current" : whence == SEEK_SET ? "beginning" :
         whence == SEEK_END ? "end" : "invalid");

    if (! fseek_impl (offset, whence))
        return 0;

    AUDDBG ("<%p> seek failed!\n", this);

    return -1;
}

/**
 * Returns the current position in the VFS stream's buffer.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the current position. Otherwise, -1.
 */
EXPORT int64_t VFSFile::ftell ()
{
    int64_t told = ftell_impl ();

    AUDDBG ("<%p> tell = %" PRId64 "\n", this, told);

    return told;
}

/**
 * Returns whether or not the VFS stream has reached EOF.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, whether or not the VFS stream is at EOF. Otherwise, false.
 */
EXPORT bool VFSFile::feof ()
{
    bool eof = feof_impl ();

    AUDDBG ("<%p> eof = %s\n", this, eof ? "yes" : "no");

    return eof;
}

/**
 * Truncates a VFS stream to a certain size.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param length The length to truncate at.
 * @return On success, 0. Otherwise, -1.
 */
EXPORT int VFSFile::ftruncate (int64_t length)
{
    AUDDBG ("<%p> truncate to %" PRId64 "\n", this, length);

    if (! ftruncate_impl (length))
        return 0;

    AUDDBG ("<%p> truncate failed!\n", this);

    return -1;
}

EXPORT int VFSFile::fflush ()
{
    AUDDBG ("<%p> flush\n", this);

    if (! fflush_impl ())
        return 0;

    AUDDBG ("<%p> flush failed!\n", this);

    return -1;
}

/**
 * Returns size of the file.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @return On success, the size of the file in bytes. Otherwise, -1.
 */
EXPORT int64_t VFSFile::fsize ()
{
    int64_t size = fsize_impl ();

    AUDDBG ("<%p> size = %" PRId64 "\n", this, size);

    return size;
}

/**
 * Returns metadata about the stream.
 *
 * @param file #VFSFile object that represents the VFS stream.
 * @param field The string constant field name to get.
 * @return On success, a copy of the value of the field. Otherwise, nullptr.
 */
EXPORT String VFSFile::get_metadata (const char * field)
{
    return get_metadata_impl (field);
}

/**
 * Wrapper for g_file_test().
 *
 * @param path A path to test.
 * @param test A GFileTest to run.
 * @return The result of g_file_test().
 */
EXPORT bool
vfs_file_test(const char * path, VFSFileTest test)
{
    if (strncmp (path, "file://", 7))
        return false; /* only local files are handled */

    const char * sub;
    uri_parse (path, nullptr, nullptr, & sub, nullptr);

    StringBuf no_sub = str_copy (path, sub - path);

    StringBuf path2 = uri_to_filename (no_sub);
    if (! path2)
        return false;

#ifdef S_ISLNK
    if (test & VFS_IS_SYMLINK)
    {
        GStatBuf st;
        if (g_lstat (path2, & st) < 0)
            return false;

        if (S_ISLNK (st.st_mode))
            test = (VFSFileTest) (test & ~VFS_IS_SYMLINK);
    }
#endif

    if (test & (VFS_IS_REGULAR | VFS_IS_DIR | VFS_IS_EXECUTABLE | VFS_EXISTS))
    {
        GStatBuf st;
        if (g_stat (path2, & st) < 0)
            return false;

        if (S_ISREG (st.st_mode))
            test = (VFSFileTest) (test & ~VFS_IS_REGULAR);
        if (S_ISDIR (st.st_mode))
            test = (VFSFileTest) (test & ~VFS_IS_DIR);
        if (st.st_mode & S_IXUSR)
            test = (VFSFileTest) (test & ~VFS_IS_EXECUTABLE);

        test = (VFSFileTest) (test & ~VFS_EXISTS);
    }

    return ! test;
}

/**
 * Tests if a file is writeable.
 *
 * @param path A path to test.
 * @return true if the file is writeable, otherwise false.
 */
EXPORT bool
vfs_is_writeable(const char * path)
{
    GStatBuf info;
    StringBuf realfn = uri_to_filename (path);

    if (! realfn || g_stat (realfn, & info) < 0)
        return false;

    return (info.st_mode & S_IWUSR);
}

/**
 * Tests if a path is remote uri.
 *
 * @param path A path to test.
 * @return true if the file is remote, otherwise false.
 */
EXPORT bool vfs_is_remote (const char * path)
{
    return strncmp (path, "file://", 7) ? true : false;
}

/**
 * Tests if a file is associated to streaming.
 *
 * @param file A #VFSFile object to test.
 * @return true if the file is streaming, otherwise false.
 */
EXPORT bool vfs_is_streaming (VFSFile * file)
{
    return (vfs_fsize (file) < 0);
}
