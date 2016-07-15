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

#include "audstrings.h"
#include "i18n.h"
#include "internal.h"
#include "plugin.h"
#include "plugins-internal.h"
#include "probe-buffer.h"
#include "runtime.h"
#include "vfs_local.h"

/* embedded plugins */
static LocalTransport local_transport;
static StdinTransport stdin_transport;

static TransportPlugin * lookup_transport (const char * filename,
 String & error, bool * custom_input = nullptr)
{
    StringBuf scheme = uri_get_scheme (filename);
    if (! scheme)
    {
        AUDERR ("Invalid URI: %s\n", filename);
        error = String (_("Invalid URI"));
        return nullptr;
    }

    if (! strcmp (scheme, "file"))
        return & local_transport;
    if (! strcmp (scheme, "stdin"))
        return & stdin_transport;

    for (PluginHandle * plugin : aud_plugin_list (PluginType::Transport))
    {
        if (! aud_plugin_get_enabled (plugin))
            continue;

        if (transport_plugin_has_scheme (plugin, scheme))
        {
            auto tp = (TransportPlugin *) aud_plugin_get_header (plugin);
            if (tp)
                return tp;
        }
    }

    if (custom_input)
    {
        for (PluginHandle * plugin : aud_plugin_list (PluginType::Input))
        {
            if (! aud_plugin_get_enabled (plugin))
                continue;

            if (input_plugin_has_key (plugin, InputKey::Scheme, scheme))
            {
                * custom_input = true;
                return nullptr;
            }
        }
    }

    AUDERR ("Unknown URI scheme: %s://\n", (const char *) scheme);
    error = String (_("Unknown URI scheme"));
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
    auto tp = lookup_transport (filename, m_error);
    if (! tp)
        return;

    VFSImpl * impl = tp->fopen (strip_subtune (filename), mode, m_error);
    if (! impl)
        return;

    /* enable buffering for read-only handles */
    if (mode[0] == 'r' && ! strchr (mode, '+'))
        impl = new ProbeBuffer (filename, impl);

    AUDINFO ("<%p> open (mode %s) %s\n", impl, mode, filename);
    m_filename = String (filename);
    m_impl.capture (impl);
}

EXPORT VFSFile VFSFile::tmpfile ()
{
    VFSFile file;
    file.m_impl.capture (vfs_tmpfile (file.m_error));
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
    int64_t readed = m_impl->fread (ptr, size, nmemb);

    AUDDBG ("<%p> read %" PRId64 " elements of size %" PRId64 " = %" PRId64 "\n",
     m_impl.get (), nmemb, size, readed);

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
     whence == VFS_SEEK_CUR ? "current" : whence == VFS_SEEK_SET ? "beginning" :
     whence == VFS_SEEK_END ? "end" : "invalid");

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

EXPORT void VFSFile::set_limit_to_buffer (bool limit)
{
    auto buffer = dynamic_cast<ProbeBuffer *> (m_impl.get ());
    if (buffer)
        buffer->set_limit_to_buffer (limit);
    else
        AUDERR ("<%p> buffering not supported!\n", m_impl.get ());
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

EXPORT bool VFSFile::copy_from (VFSFile & source, int64_t size)
{
    constexpr int bufsize = 65536;

    Index<char> buf;
    buf.resize (bufsize);

    while (size < 0 || size > 0)
    {
        int64_t to_read = (size > 0 && size < bufsize) ? size : bufsize;
        int64_t readsize = source.fread (buf.begin (), 1, to_read);

        if (size > 0)
            size -= readsize;

        if (fwrite (buf.begin (), 1, readsize) != readsize)
            return false;

        if (readsize < to_read)
            break;
    }

    /* if a fixed size was requested, return true only if all the data was read.
     * otherwise, return true only if the end of the source file was reached. */
    return size == 0 || (size < 0 && source.feof ());
}

EXPORT bool VFSFile::replace_with (VFSFile & source)
{
    if (source.fseek (0, VFS_SEEK_SET) < 0)
        return false;

    if (fseek (0, VFS_SEEK_SET) < 0)
        return false;

    if (ftruncate (0) < 0)
        return false;

    return copy_from (source, -1);
}

EXPORT bool VFSFile::test_file (const char * filename, VFSFileTest test)
{
    String error;  /* discarded */
    return test_file (filename, test, error) == test;
}

EXPORT VFSFileTest VFSFile::test_file (const char * filename, VFSFileTest test, String & error)
{
    bool custom_input = false;
    auto tp = lookup_transport (filename, error, & custom_input);

    /* for URI schemes handled by input plugins, return 0, indicating that we
     * have no way of testing file attributes */
    if (custom_input)
        return VFSFileTest (0);

    /* for unsupported URI schemes, return VFS_NO_ACCESS */
    if (! tp)
        return VFSFileTest (test & VFS_NO_ACCESS);

    return tp->test_file (strip_subtune (filename), test, error);
}

EXPORT Index<String> VFSFile::read_folder (const char * filename, String & error)
{
    auto tp = lookup_transport (filename, error);
    return tp ? tp->read_folder (filename, error) : Index<String> ();
}
