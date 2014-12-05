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

#include <glib/gstdio.h>

#include "audstrings.h"
#include "i18n.h"
#include "plugin.h"
#include "plugins-internal.h"
#include "runtime.h"
#include "vfs_local.h"

static TransportPlugin * lookup_transport (const char * scheme)
{
    for (PluginHandle * plugin : aud_plugin_list (PluginType::Transport))
    {
        if (! aud_plugin_get_enabled (plugin))
            continue;

        if (transport_plugin_has_scheme (plugin, scheme))
            return (TransportPlugin *) aud_plugin_get_header (plugin);
    }

    return nullptr;
}

/**
 * Opens a stream from a VFS transport using one of the registered
 * #VFSConstructor handlers.
 *
 * @param path The path or URI to open.
 * @param mode The preferred access privileges (not guaranteed).
 * @return On success, a #VFSFile object representing the stream.
 */
EXPORT VFSFile::VFSFile (const char * filename, const char * mode)
{
    StringBuf scheme = uri_get_scheme (filename);
    if (! scheme)
    {
        AUDERR ("Invalid URI: %s\n", filename);
        m_error = String (_("Invalid URI"));
        return;
    }

    const char * sub;
    uri_parse (filename, nullptr, nullptr, & sub, nullptr);
    StringBuf nosub = str_copy (filename, sub - filename);

    if (! strcmp (scheme, "file"))
        m_impl.capture (vfs_local_fopen (nosub, mode, m_error));
    else
    {
        TransportPlugin * tp = lookup_transport (scheme);
        if (! tp)
        {
            AUDERR ("Unknown URI scheme: %s://", (const char *) scheme);
            m_error = String (_("Unknown URI scheme"));
            return;
        }

        m_impl.capture (tp->fopen (nosub, mode, m_error));
    }

    if (! m_impl)
        return;

    AUDINFO ("<%p> open (mode %s) %s\n", m_impl.get (), mode, filename);
    m_filename = String (filename);
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
    int64_t readed = m_impl->fread (ptr, size, nmemb);

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
    int64_t written = m_impl->fwrite (ptr, size, nmemb);

    AUDDBG ("<%p> write %" PRId64 " elements of size %" PRId64 " = %" PRId64 "\n",
     m_impl.get (), nmemb, size, written);

    return written;
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
    AUDDBG ("<%p> seek to %" PRId64 " from %s\n", m_impl.get (), offset,
         whence == SEEK_CUR ? "current" : whence == SEEK_SET ? "beginning" :
         whence == SEEK_END ? "end" : "invalid");

    if (! m_impl->fseek (offset, whence))
        return 0;

    AUDDBG ("<%p> seek failed!\n", m_impl.get ());

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
    int64_t told = m_impl->ftell ();

    AUDDBG ("<%p> tell = %" PRId64 "\n", m_impl.get (), told);

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
    bool eof = m_impl->feof ();

    AUDDBG ("<%p> eof = %s\n", m_impl.get (), eof ? "yes" : "no");

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
    AUDDBG ("<%p> truncate to %" PRId64 "\n", m_impl.get (), length);

    if (! m_impl->ftruncate (length))
        return 0;

    AUDDBG ("<%p> truncate failed!\n", m_impl.get ());

    return -1;
}

EXPORT int VFSFile::fflush ()
{
    AUDDBG ("<%p> flush\n", m_impl.get ());

    if (! m_impl->fflush ())
        return 0;

    AUDDBG ("<%p> flush failed!\n", m_impl.get ());

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
    int64_t size = m_impl->fsize ();

    AUDDBG ("<%p> size = %" PRId64 "\n", m_impl.get (), size);

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
    return m_impl->get_metadata (field);
}

EXPORT Index<char> VFSFile::read_all ()
{
    constexpr int maxbuf = 16777216;
    constexpr int pagesize = 4096;

    Index<char> buf;
    int64_t size = fsize ();
    int64_t pos = ftell ();

    if (size >= 0 && pos >= 0 && pos <= size)
    {
        buf.insert (0, aud::min (size - pos, (int64_t) maxbuf));
        size = fread (buf.begin (), 1, buf.len ());
    }
    else
    {
        size = 0;

        buf.insert (0, pagesize);

        int64_t readsize;
        while ((readsize = fread (& buf[size], 1, buf.len () - size)))
        {
            size += readsize;

            if (size == buf.len ())
            {
                if (buf.len () > maxbuf - pagesize)
                    break;

                buf.insert (-1, pagesize);
            }
        }
    }

    buf.remove (size, -1);

    return buf;
}

/**
 * Wrapper for g_file_test().
 *
 * @param path A path to test.
 * @param test A GFileTest to run.
 * @return The result of g_file_test().
 */
EXPORT bool VFSFile::test_file (const char * path, VFSFileTest test)
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
