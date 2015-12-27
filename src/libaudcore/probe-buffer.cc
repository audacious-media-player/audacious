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

#include "probe-buffer.h"
#include "runtime.h"

#include <string.h>

static constexpr int MAXBUF = 256 * 1024;

ProbeBuffer::ProbeBuffer (const char * filename, VFSImpl * file) :
    m_filename (filename),
    m_file (file)
{
    AUDINFO ("<%p> buffering enabled for %s\n", this, (const char *) m_filename);
}

ProbeBuffer::~ProbeBuffer ()
{
    delete[] m_buffer;
}

void ProbeBuffer::increase_buffer (int64_t size)
{
    size = aud::min ((size + 0xFF) & ~0xFF, (int64_t) MAXBUF);

    if (m_filled < size)
    {
        if (! m_buffer)
            m_buffer = new char[MAXBUF];

        m_filled += m_file->fread (m_buffer + m_filled, 1, size - m_filled);
    }
}

void ProbeBuffer::release_buffer ()
{
    AUDINFO ("<%p> buffering disabled for %s\n", this, (const char *) m_filename);
    delete[] m_buffer;
    m_buffer = nullptr;
    m_filled = 0;
    m_at = -1;
}

int64_t ProbeBuffer::fread (void * buffer, int64_t size, int64_t count)
{
    int64_t total = 0;
    int64_t remain = size * count;

    /* read from buffer if possible */
    if (remain && m_at >= 0 && m_at < MAXBUF)
    {
        increase_buffer (m_at + remain);

        int copy = aud::min (remain, (int64_t) (m_filled - m_at));
        memcpy (buffer, m_buffer + m_at, copy);

        m_at += copy;
        buffer = (char *) buffer + copy;
        total += copy;
        remain -= copy;
    }

    /* then read from real file if allowed */
    if (remain && ! m_limited)
    {
        /* release buffer if leaving bufferable area */
        if (m_at == MAXBUF)
            release_buffer ();

        if (m_at < 0)
            total += m_file->fread (buffer, 1, remain);
    }

    return (size > 0) ? total / size : 0;
}

int64_t ProbeBuffer::fwrite (const void * data, int64_t size, int64_t count)
{
    return 0; /* not allowed */
}

int ProbeBuffer::fseek (int64_t offset, VFSSeekType whence)
{
    /* seek within the buffer if possible */
    if (m_at >= 0 && whence != VFS_SEEK_END)
    {
        /* calculate requested position */
        if (whence == VFS_SEEK_CUR)
        {
            offset += m_at;
            whence = VFS_SEEK_SET;
        }

        if (offset < 0)
            return -1; /* invalid argument */

        if (offset <= MAXBUF)
        {
            increase_buffer (offset);

            if (offset > m_filled)
                return -1; /* seek past end of file */

            m_at = offset;
            return 0;
        }
    }

    /* seek within real file if allowed */
    if (m_limited || m_file->fseek (offset, whence) < 0)
        return -1;

    /* release buffer only if real seek succeeded
     * (prevents change of file position if seek failed) */
    if (m_at >= 0)
        release_buffer ();

    /* activate buffering again when seeking to beginning of file */
    if (whence == VFS_SEEK_SET && offset == 0)
    {
        AUDINFO ("<%p> buffering enabled for %s\n", this, (const char *) m_filename);
        m_at = 0;
    }

    return 0;
}

int64_t ProbeBuffer::ftell ()
{
    if (m_at >= 0)
        return m_at;

    return m_file->ftell ();
}

bool ProbeBuffer::feof ()
{
    if (m_at >= 0 && m_at < m_filled)
        return false;

    return m_file->feof ();
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
    return m_file->fsize ();
}

String ProbeBuffer::get_metadata (const char * field)
{
    return m_file->get_metadata (field);
}
