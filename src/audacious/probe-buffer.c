/*
 * probe-buffer.c
 * Copyright 2010-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "probe-buffer.h"

typedef struct
{
    VFSFile * file;
    unsigned char buffer[16384];
    int filled, at;
}
ProbeBuffer;

static int probe_buffer_fclose (VFSFile * file)
{
    ProbeBuffer * p = vfs_get_handle (file);

    int ret = vfs_fclose (p->file);
    g_slice_free (ProbeBuffer, p);
    return ret;
}

static void increase_buffer (ProbeBuffer * p, int64_t size)
{
    size = (size + 0xFF) & ~0xFF;

    if (size > sizeof p->buffer)
        size = sizeof p->buffer;

    if (p->filled < size)
        p->filled += vfs_fread (p->buffer + p->filled, 1, size - p->filled,
         p->file);
}

static int64_t probe_buffer_fread (void * buffer, int64_t size, int64_t count,
 VFSFile * file)
{
    ProbeBuffer * p = vfs_get_handle (file);

    increase_buffer (p, p->at + size * count);
    int readed = (size > 0) ? MIN (count, (p->filled - p->at) / size) : 0;
    memcpy (buffer, p->buffer + p->at, size * readed);

    p->at += size * readed;
    return readed;
}

static int64_t probe_buffer_fwrite (const void * data, int64_t size, int64_t count,
 VFSFile * file)
{
    /* not implemented */
    return 0;
}

static int probe_buffer_getc (VFSFile * file)
{
    unsigned char c;
    return (probe_buffer_fread (& c, 1, 1, file) == 1) ? c : EOF;
}

static int probe_buffer_fseek (VFSFile * file, int64_t offset, int whence)
{
    ProbeBuffer * p = vfs_get_handle (file);

    if (whence == SEEK_END)
        return -1;

    if (whence == SEEK_CUR)
        offset += p->at;

    g_return_val_if_fail (offset >= 0, -1);
    increase_buffer (p, offset);

    if (offset > p->filled)
        return -1;

    p->at = offset;
    return 0;
}

static int probe_buffer_ungetc (int c, VFSFile * file)
{
    return (! probe_buffer_fseek (file, -1, SEEK_CUR)) ? c : EOF;
}

static void probe_buffer_rewind (VFSFile * file)
{
    probe_buffer_fseek (file, 0, SEEK_SET);
}

static int64_t probe_buffer_ftell (VFSFile * file)
{
    return ((ProbeBuffer *) vfs_get_handle (file))->at;
}

static bool_t probe_buffer_feof (VFSFile * file)
{
    ProbeBuffer * p = vfs_get_handle (file);
    return (p->at < p->filled) ? FALSE : vfs_feof (p->file);
}

static int probe_buffer_ftruncate (VFSFile * file, int64_t size)
{
    /* not implemented */
    return -1;
}

static int64_t probe_buffer_fsize (VFSFile * file)
{
    return vfs_fsize (((ProbeBuffer *) vfs_get_handle (file))->file);
}

static char * probe_buffer_get_metadata (VFSFile * file, const char * field)
{
    return vfs_get_metadata (((ProbeBuffer *) vfs_get_handle (file))->file, field);
}

static VFSConstructor probe_buffer_table =
{
	.vfs_fopen_impl = NULL,
	.vfs_fclose_impl = probe_buffer_fclose,
	.vfs_fread_impl = probe_buffer_fread,
	.vfs_fwrite_impl = probe_buffer_fwrite,
	.vfs_getc_impl = probe_buffer_getc,
	.vfs_ungetc_impl = probe_buffer_ungetc,
	.vfs_fseek_impl = probe_buffer_fseek,
	.vfs_rewind_impl = probe_buffer_rewind,
	.vfs_ftell_impl = probe_buffer_ftell,
	.vfs_feof_impl = probe_buffer_feof,
	.vfs_ftruncate_impl = probe_buffer_ftruncate,
	.vfs_fsize_impl = probe_buffer_fsize,
	.vfs_get_metadata_impl = probe_buffer_get_metadata,
};

VFSFile * probe_buffer_new (const char * filename)
{
    VFSFile * file = vfs_fopen (filename, "r");

    if (! file)
        return NULL;

    ProbeBuffer * p = g_slice_new (ProbeBuffer);
    p->file = file;
    p->filled = 0;
    p->at = 0;

    return vfs_new (filename, & probe_buffer_table, p);
}
