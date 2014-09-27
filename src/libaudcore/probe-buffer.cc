/*
 * probe-buffer.c
 * Copyright 2010-2013 John Lindgren
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

#include "internal.h"

#include <string.h>

#include "vfs.h"

#define BUFSIZE (256 * 1024)

class ProbeBuffer : public VFSImpl
{
public:
    ProbeBuffer (VFSFile && file) :
        m_file (std::move (file)),
        m_filled (0),
        m_at (0) {}

    int64_t fread (void * ptr, int64_t size, int64_t nmemb);
    int fseek (int64_t offset, VFSSeekType whence);

    int64_t ftell ();
    int64_t fsize ();
    bool feof ();

    int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb);
    int ftruncate (int64_t length);
    int fflush ();

    String get_metadata (const char * field);

private:
    void increase_buffer (int64_t size);

    VFSFile m_file;
    int m_filled, m_at;
    unsigned char m_buffer[BUFSIZE];
};

void ProbeBuffer::increase_buffer (int64_t size)
{
    size = (size + 0xFF) & ~0xFF;

    if (size > (int64_t) sizeof m_buffer)
        size = (int64_t) sizeof m_buffer;

    if (m_filled < size)
        m_filled += m_file.fread (m_buffer + m_filled, 1, size - m_filled);
}

int64_t ProbeBuffer::fread (void * buffer, int64_t size, int64_t count)
{
    increase_buffer (m_at + size * count);
    int readed = (size > 0) ? aud::min (count, (m_filled - m_at) / size) : 0;
    memcpy (buffer, m_buffer + m_at, size * readed);

    m_at += size * readed;
    return readed;
}

int64_t ProbeBuffer::fwrite (const void * data, int64_t size, int64_t count)
{
    return 0; /* not allowed */
}

int ProbeBuffer::fseek (int64_t offset, VFSSeekType whence)
{
    if (whence == VFS_SEEK_END)
        return -1; /* not allowed */

    if (whence == VFS_SEEK_CUR)
        offset += m_at;

    if (offset < 0 || offset > (int64_t) sizeof m_buffer)
        return -1;

    increase_buffer (offset);

    if (offset > m_filled)
        return -1;

    m_at = offset;
    return 0;
}

int64_t ProbeBuffer::ftell ()
{
    return m_at;
}

bool ProbeBuffer::feof ()
{
    if (m_at < m_filled)
        return false;
    if (m_at == sizeof m_buffer)
        return true;

    return m_file.feof ();
}

int ProbeBuffer::ftruncate (int64_t size)
{
    return -1; /* not allowed */
}

int ProbeBuffer::fflush ()
{
    return 0; /* no-op */
}

int64_t ProbeBuffer::fsize ()
{
    int64_t size = m_file.fsize ();
    return aud::min (size, (int64_t) sizeof m_buffer);
}

String ProbeBuffer::get_metadata (const char * field)
{
    return m_file.get_metadata (field);
}

VFSFile probe_buffer_new (const char * filename)
{
    VFSFile file (filename, "r");
    if (! file)
        return file;  // preserve error message

    return VFSFile (filename, new ProbeBuffer (std::move (file)));
}
