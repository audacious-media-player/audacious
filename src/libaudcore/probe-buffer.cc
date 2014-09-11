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
#include <glib.h>

#include "vfs.h"

#define BUFSIZE (256 * 1024)

struct ProbeBuffer {
    VFSFile * file;
    int filled, at;
    unsigned char buffer[BUFSIZE];
};

static int probe_buffer_fclose (VFSFile * file)
{
    ProbeBuffer * p = (ProbeBuffer *) vfs_get_handle (file);

    int ret = vfs_fclose (p->file);
    g_free (p);
    return ret;
}

static void increase_buffer (ProbeBuffer * p, int64_t size)
{
    size = (size + 0xFF) & ~0xFF;

    if (size > (int64_t) sizeof p->buffer)
        size = (int64_t) sizeof p->buffer;

    if (p->filled < size)
        p->filled += vfs_fread (p->buffer + p->filled, 1, size - p->filled,
         p->file);
}

static int64_t probe_buffer_fread (void * buffer, int64_t size, int64_t count,
 VFSFile * file)
{
    ProbeBuffer * p = (ProbeBuffer *) vfs_get_handle (file);

    increase_buffer (p, p->at + size * count);
    int readed = (size > 0) ? aud::min (count, (p->filled - p->at) / size) : 0;
    memcpy (buffer, p->buffer + p->at, size * readed);

    p->at += size * readed;
    return readed;
}

static int64_t probe_buffer_fwrite (const void * data, int64_t size, int64_t count,
 VFSFile * file)
{
    return 0; /* not allowed */
}

static int probe_buffer_fseek (VFSFile * file, int64_t offset, VFSSeekType whence)
{
    ProbeBuffer * p = (ProbeBuffer *) vfs_get_handle (file);

    if (whence == VFS_SEEK_END)
        return -1; /* not allowed */

    if (whence == VFS_SEEK_CUR)
        offset += p->at;

    if (offset < 0 || offset > (int64_t) sizeof p->buffer)
        return -1;

    increase_buffer (p, offset);

    if (offset > p->filled)
        return -1;

    p->at = offset;
    return 0;
}

static int64_t probe_buffer_ftell (VFSFile * file)
{
    return ((ProbeBuffer *) vfs_get_handle (file))->at;
}

static bool probe_buffer_feof (VFSFile * file)
{
    ProbeBuffer * p = (ProbeBuffer *) vfs_get_handle (file);

    if (p->at < p->filled)
        return false;
    if (p->at == sizeof p->buffer)
        return true;

    return vfs_feof (p->file);
}

static int probe_buffer_ftruncate (VFSFile * file, int64_t size)
{
    return -1; /* not allowed */
}

static int64_t probe_buffer_fsize (VFSFile * file)
{
    ProbeBuffer * p = (ProbeBuffer *) vfs_get_handle (file);

    int64_t size = vfs_fsize (p->file);
    return aud::min (size, (int64_t) sizeof p->buffer);
}

static String probe_buffer_get_metadata (VFSFile * file, const char * field)
{
    return vfs_get_metadata (((ProbeBuffer *) vfs_get_handle (file))->file, field);
}

static const VFSConstructor probe_buffer_table =
{
    nullptr,
    probe_buffer_fclose,
    probe_buffer_fread,
    probe_buffer_fwrite,
    probe_buffer_fseek,
    probe_buffer_ftell,
    probe_buffer_feof,
    probe_buffer_ftruncate,
    probe_buffer_fsize,
    probe_buffer_get_metadata
};

VFSFile * probe_buffer_new (const char * filename)
{
    VFSFile * file = vfs_fopen (filename, "r");

    if (! file)
        return nullptr;

    ProbeBuffer * p = g_new (ProbeBuffer, 1);
    p->file = file;
    p->filled = 0;
    p->at = 0;

    return vfs_new (filename, & probe_buffer_table, p);
}
