/*
 * vfs_local.c
 * Copyright 2009-2014 John Lindgren
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

#define WANT_VFS_STDIO_COMPAT
#include "vfs_local.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"
#include "runtime.h"

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif

#define perror(s) AUDERR ("%s: %s\n", (const char *) (s), strerror (errno))

enum LocalOp {
    OP_NONE,
    OP_READ,
    OP_WRITE
};

class LocalFile : public VFSFile
{
public:
    LocalFile (const char * uri, const char * path, FILE * stream) :
        VFSFile (uri),
        m_path (path),
        m_stream (stream),
        m_cached_size (-1),
        m_last_op (OP_NONE) {}

    ~LocalFile ();

protected:
    int64_t fread_impl (void * ptr, int64_t size, int64_t nmemb);
    int fseek_impl (int64_t offset, VFSSeekType whence);

    int64_t ftell_impl ();
    int64_t fsize_impl ();
    bool feof_impl ();

    int64_t fwrite_impl (const void * ptr, int64_t size, int64_t nmemb);
    int ftruncate_impl (int64_t length);
    int fflush_impl ();

private:
    String m_path;
    FILE * m_stream;
    int64_t m_cached_size;
    LocalOp m_last_op;
};

VFSFile * vfs_local_fopen (const char * uri, const char * mode)
{
    StringBuf path = uri_to_filename (uri);
    g_return_val_if_fail (path, nullptr);

    const char * suffix = "";

#ifdef _WIN32
    if (! strchr (mode, 'b'))  /* binary mode (Windows) */
        suffix = "b";
#else
    if (! strchr (mode, 'e'))  /* close on exec (POSIX) */
        suffix = "e";
#endif

    StringBuf mode2 = str_concat ({mode, suffix});

    FILE * stream = g_fopen (path, mode2);

    if (! stream)
    {
        perror (path);
        return nullptr;
    }

    return new LocalFile (uri, path, stream);
}

LocalFile::~LocalFile ()
{
    if (fclose (m_stream) < 0)
        perror (m_path);
}

int64_t LocalFile::fread_impl (void * ptr, int64_t size, int64_t nitems)
{
    if (m_last_op == OP_WRITE)
    {
        if (::fflush (m_stream) < 0)
        {
            perror (m_path);
            return 0;
        }
    }

    m_last_op = OP_READ;

    clearerr (m_stream);

    int64_t result = ::fread (ptr, size, nitems, m_stream);
    if (result < nitems && ferror (m_stream))
        perror (m_path);

    return result;
}

int64_t LocalFile::fwrite_impl (const void * ptr, int64_t size, int64_t nitems)
{
    if (m_last_op == OP_READ)
    {
        if (::fflush (m_stream) < 0)
        {
            perror (m_path);
            return 0;
        }
    }

    m_last_op = OP_WRITE;
    m_cached_size = -1;

    clearerr (m_stream);

    int64_t result = ::fwrite (ptr, size, nitems, m_stream);
    if (result < nitems && ferror (m_stream))
        perror (m_path);

    return result;
}

int LocalFile::fseek_impl (int64_t offset, VFSSeekType whence)
{
    int result = fseeko (m_stream, offset, from_vfs_seek_type (whence));
    if (result < 0)
        perror (m_path);

    if (result == 0)
        m_last_op = OP_NONE;

    return result;
}

int64_t LocalFile::ftell_impl ()
{
    return ftello (m_stream);
}

bool LocalFile::feof_impl ()
{
    return ::feof (m_stream);
}

int LocalFile::ftruncate_impl (int64_t length)
{
    if (m_last_op != OP_NONE)
    {
        if (::fflush (m_stream) < 0)
        {
            perror (m_path);
            return -1;
        }
    }

    int result = ::ftruncate (fileno (m_stream), length);
    if (result < 0)
        perror (m_path);

    if (result == 0)
    {
        m_last_op = OP_NONE;
        m_cached_size = length;
    }

    return result;
}

int LocalFile::fflush_impl ()
{
    if (m_last_op != OP_WRITE)
        return 0;

    int result = ::fflush (m_stream);
    if (result < 0)
        perror (m_path);

    if (result == 0)
        m_last_op = OP_NONE;

    return result;
}

int64_t LocalFile::fsize_impl ()
{
    if (m_cached_size < 0)
    {
        int64_t saved_pos = ftello (m_stream);
        if (saved_pos < 0)
            goto ERR;

        if (fseek_impl (0, VFS_SEEK_END) < 0)
            goto ERR;

        int64_t length = ftello (m_stream);
        if (length < 0)
            goto ERR;

        if (fseek_impl (saved_pos, VFS_SEEK_SET) < 0)
            goto ERR;

        m_cached_size = length;
    }

    return m_cached_size;

ERR:
    perror (m_path);
    return -1;
}