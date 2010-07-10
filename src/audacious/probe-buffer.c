/*
 * probe-buffer.c
 * Copyright 2010 John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "probe-buffer.h"

typedef struct
{
    const gchar * filename, * decoder;
    VFSFile * file;
    guchar buffer[65536];
    gint filled, at;
    const gchar * read_warned, * seek_warned;
}
ProbeBuffer;

static gint probe_buffer_fclose (VFSFile * file)
{
    gint ret = vfs_fclose (((ProbeBuffer *) file->handle)->file);
    g_free (file->handle);
    return ret;
}

static void increase_buffer (ProbeBuffer * p, gint64 size)
{
    if (size > sizeof p->buffer)
    {
        if (p->read_warned != p->decoder)
        {
            fprintf (stderr, "%s tried to read past end of buffer while "
             "probing %s.\n", p->decoder, p->filename);
            p->read_warned = p->decoder;
        }

        size = sizeof p->buffer;
    }

    p->filled += vfs_fread (p->buffer + p->at, 1, size - p->filled, p->file);
}

static gint64 probe_buffer_fread (void * buffer, gint64 size, gint64 count,
 VFSFile * file)
{
    ProbeBuffer * p = file->handle;

    increase_buffer (p, p->at + size * count);
    gint readed = (size > 0) ? MIN (count, (p->filled - p->at) / size) : 0;
    memcpy (buffer, p->buffer + p->at, size * readed);

    p->at += size * readed;
    return readed;
}

static gint64 probe_buffer_fwrite (const void * data, gint64 size, gint64 count,
 VFSFile * file)
{
    /* not implemented */
    return 0;
}

static gint probe_buffer_getc (VFSFile * file)
{
    guchar c;
    return (probe_buffer_fread (& c, 1, 1, file) == 1) ? c : EOF;
}

static gint probe_buffer_fseek (VFSFile * file, gint64 offset, gint whence)
{
    ProbeBuffer * p = file->handle;

    if (whence == SEEK_END)
    {
        if (p->seek_warned != p->decoder)
        {
            fprintf (stderr, "%s tried to seek to end of file while probing "
             "%s.\n", p->decoder, p->filename);
            p->seek_warned = p->decoder;
        }

        return -1;
    }

    if (whence == SEEK_CUR)
        offset += p->at;

    g_return_val_if_fail (offset >= 0, -1);
    increase_buffer (p, offset);

    if (offset > p->filled)
        return -1;

    p->at = offset;
    return 0;
}

static gint probe_buffer_ungetc (gint c, VFSFile * file)
{
    return (! probe_buffer_fseek (file, -1, SEEK_CUR)) ? c : EOF;
}

static void probe_buffer_rewind (VFSFile * file)
{
    probe_buffer_fseek (file, 0, SEEK_SET);
}

static gint64 probe_buffer_ftell (VFSFile * file)
{
    return ((ProbeBuffer *) file->handle)->at;
}

static gboolean probe_buffer_feof (VFSFile * file)
{
    ProbeBuffer * p = file->handle;
    return (p->at < p->filled) ? FALSE : vfs_feof (p->file);
}

static gint probe_buffer_ftruncate (VFSFile * file, gint64 size)
{
    /* not implemented */
    return -1;
}

static gint64 probe_buffer_fsize (VFSFile * file)
{
    return vfs_fsize (((ProbeBuffer *) file->handle)->file);
}

static gchar * probe_buffer_get_metadata (VFSFile * file, const gchar * field)
{
    return vfs_get_metadata (((ProbeBuffer *) file->handle)->file, field);
}

static VFSConstructor probe_buffer_table =
{
	.uri_id = NULL,
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

VFSFile * probe_buffer_new (const gchar * filename)
{
    VFSFile * file = vfs_fopen (filename, "r");

    if (! file)
        return NULL;

    ProbeBuffer * p = g_malloc (sizeof (ProbeBuffer));
    p->decoder = NULL;
    p->filename = filename;
    p->file = file;
    p->filled = 0;
    p->at = 0;
    p->read_warned = NULL;
    p->seek_warned = NULL;

    VFSFile * file2 = g_malloc (sizeof (VFSFile));
    file2->base = & probe_buffer_table;
    file2->handle = p;
    file2->uri = g_strdup (filename);
    file2->ref = 1;

    return file2;
}

void probe_buffer_set_decoder (VFSFile * file, const gchar * decoder)
{
    ProbeBuffer * p = file->handle;
    p->decoder = decoder;
}
