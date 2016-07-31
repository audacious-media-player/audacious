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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

/* needs to be after system headers for #undef's to take effect */
#define WANT_VFS_STDIO_COMPAT
#include "vfs_local.h"

#include "audstrings.h"
#include "i18n.h"
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

class LocalFile : public VFSImpl
{
public:
    LocalFile (const char * path, FILE * stream) :
        m_path (path),
        m_stream (stream),
        m_cached_pos (0),
        m_cached_size (-1),
        m_last_op (OP_NONE) {}

    ~LocalFile ();

protected:
    int64_t fread (void * ptr, int64_t size, int64_t nmemb);
    int fseek (int64_t offset, VFSSeekType whence);

    int64_t ftell ();
    int64_t fsize ();
    bool feof ();

    int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb);
    int ftruncate (int64_t length);
    int fflush ();

private:
    String m_path;
    FILE * m_stream;
    int64_t m_cached_pos;
    int64_t m_cached_size;
    LocalOp m_last_op;
};

VFSImpl * LocalTransport::fopen (const char * uri, const char * mode, String & error)
{
    StringBuf path = uri_to_filename (uri);

    if (! path)
    {
        error = String (_("Invalid file name"));
        return nullptr;
    }

    const char * suffix = "";

#ifdef _WIN32
    if (! strchr (mode, 'b'))  /* binary mode (Windows) */
        suffix = "b";
#else
    if (! strchr (mode, 'e'))  /* close on exec (POSIX) */
        suffix = "e";
#endif

    StringBuf mode2 = str_concat ({mode, suffix});

    FILE * stream = ::g_fopen (path, mode2);

    if (! stream)
    {
        int errsave = errno;

        /* try converting to UTF-8, this can solve issues such as:
         * 1) legacy filename used on UTF-8 system
         * 2) UTF-8 filesystem mounted on legacy system */
        if (errsave == ENOENT)
        {
            StringBuf path2 = uri_to_filename (uri, false);
            if (path2 && strcmp (path, path2))
                stream = ::g_fopen (path2, mode2);
        }

        if (! stream)
        {
            perror (path);
            error = String (strerror (errsave));
            return nullptr;
        }
    }

    return new LocalFile (path, stream);
}

VFSImpl * StdinTransport::fopen (const char * uri, const char * mode, String & error)
{
    if (mode[0] != 'r' || strchr (mode, '+'))
    {
        error = String (_("Invalid access mode"));
        return nullptr;
    }

    return new LocalFile ("(stdin)", stdin);
}

VFSImpl * vfs_tmpfile (String & error)
{
    FILE * stream = tmpfile ();

    if (! stream)
    {
        int errsave = errno;
        perror ("(tmpfile)");
        error = String (strerror (errsave));
        return nullptr;
    }

    return new LocalFile ("(tmpfile)", stream);
}

LocalFile::~LocalFile ()
{
    // do not close stdin
    if (m_stream != stdin && fclose (m_stream) < 0)
        perror (m_path);
}

int64_t LocalFile::fread (void * ptr, int64_t size, int64_t nitems)
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

    if (m_cached_pos >= 0)
        m_cached_pos += size * result;

    return result;
}

int64_t LocalFile::fwrite (const void * ptr, int64_t size, int64_t nitems)
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

    clearerr (m_stream);

    int64_t result = ::fwrite (ptr, size, nitems, m_stream);
    if (result < nitems && ferror (m_stream))
        perror (m_path);

    if (m_cached_pos >= 0)
        m_cached_pos += size * result;

    if (m_cached_size >= 0 && m_cached_pos >= 0)
        m_cached_size = aud::max (m_cached_size, m_cached_pos);
    else
        m_cached_size = -1;

    return result;
}

int LocalFile::fseek (int64_t offset, VFSSeekType whence)
{
    int result = fseeko (m_stream, offset, from_vfs_seek_type (whence));
    if (result < 0)
        perror (m_path);

    if (result == 0)
    {
        m_last_op = OP_NONE;

        if (whence == VFS_SEEK_SET)
            m_cached_pos = offset;
        else if (whence == VFS_SEEK_CUR && m_cached_pos >= 0)
            m_cached_pos += offset;
        else
            m_cached_pos = -1;
    }

    return result;
}

int64_t LocalFile::ftell ()
{
    if (m_cached_pos < 0)
        m_cached_pos = ftello (m_stream);

    return m_cached_pos;
}

bool LocalFile::feof ()
{
    return ::feof (m_stream);
}

int LocalFile::ftruncate (int64_t length)
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

int LocalFile::fflush ()
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

int64_t LocalFile::fsize ()
{
    // size of stdin is unknown
    if (m_stream == stdin)
        return -1;

    if (m_cached_size < 0)
    {
        int64_t saved_pos = ftell ();
        if (saved_pos < 0)
            goto ERR;

        if (fseek (0, VFS_SEEK_END) < 0)
            goto ERR;

        m_last_op = OP_NONE;
        m_cached_pos = -1;

        int64_t length = ftello (m_stream);
        if (length < 0)
            goto ERR;

        if (fseek (saved_pos, VFS_SEEK_SET) < 0)
            goto ERR;

        m_cached_pos = saved_pos;
        m_cached_size = length;
    }

    return m_cached_size;

ERR:
    perror (m_path);
    return -1;
}

VFSFileTest LocalTransport::test_file (const char * uri, VFSFileTest test, String & error)
{
    StringBuf path = uri_to_filename (uri);
    if (! path)
    {
        error = String (_("Invalid file name"));
        return VFSFileTest (test & VFS_NO_ACCESS);
    }

    int passed = 0;
    bool need_stat = true;
    GStatBuf st;

#ifdef S_ISLNK
    if (test & VFS_IS_SYMLINK)
    {
        if (g_lstat (path, & st) < 0)
        {
            error = String (strerror (errno));
            passed |= VFS_NO_ACCESS;
            goto out;
        }

        if (S_ISLNK (st.st_mode))
            passed |= VFS_IS_SYMLINK;
        else
            need_stat = false;
    }
#endif

    if (test & (VFS_IS_REGULAR | VFS_IS_DIR | VFS_IS_EXECUTABLE | VFS_EXISTS | VFS_NO_ACCESS))
    {
        if (need_stat && g_stat (path, & st) < 0)
        {
            error = String (strerror (errno));
            passed |= VFS_NO_ACCESS;
            goto out;
        }

        if (S_ISREG (st.st_mode))
            passed |= VFS_IS_REGULAR;
        if (S_ISDIR (st.st_mode))
            passed |= VFS_IS_DIR;
        if (st.st_mode & S_IXUSR)
            passed |= VFS_IS_EXECUTABLE;

        passed |= VFS_EXISTS;
    }

out:
    return VFSFileTest (test & passed);
}

Index<String> LocalTransport::read_folder (const char * uri, String & error)
{
    Index<String> entries;

    StringBuf path = uri_to_filename (uri);
    if (! path)
    {
        error = String (_("Invalid file name"));
        return entries;
    }

    GError * gerr = nullptr;
    GDir * folder = g_dir_open (path, 0, & gerr);
    if (! folder)
    {
        error = String (gerr->message);
        g_error_free (gerr);
        return entries;
    }

    const char * name;
    while ((name = g_dir_read_name (folder)))
        entries.append (String (filename_to_uri (filename_build ({path, name}))));

    g_dir_close (folder);

    return entries;
}
