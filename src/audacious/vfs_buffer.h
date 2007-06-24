/*
 * audacious: Cross-platform multimedia player.
 * vfs_buffer.h: VFS operations on blocks of memory.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AUDACIOUS_VFS_BUFFER_H
#define AUDACIOUS_VFS_BUFFER_H

#include <glib.h>
#include "vfs.h"

G_BEGIN_DECLS

/**
 * VFSBuffer:
 * @data: The data inside the VFSBuffer.
 * @iter: The current position of the VFS buffer iterator.
 * @begin: The beginning of the memory segment that the VFS buffer uses.
 * @end: The end of the memory segment that the VFS buffer uses.
 * @size: The size of the memory segment.
 *
 * Private data for the VFS memorybuffer class.
 **/

typedef struct {
	guchar *data;
	guchar *iter;
	guchar *end;
	gsize   size;
} VFSBuffer;

/**
 * vfs_buffer_new:
 * @data: Pointer to data to use.
 * @size: Size of data to use.
 *
 * Creates a VFS buffer for reading/writing to a memory segment.
 *
 * Return value: A VFSFile handle for the memory segment's stream 
 *               representation.
 **/
VFSFile *vfs_buffer_new(gpointer data, gsize size);

/**
 * vfs_buffer_new_from_string:
 * @str: String to use.
 *
 * Creates a VFS buffer for reading/writing to a string.
 *
 * Return value: A VFSFile handle for the memory segment's stream 
 *               representation.
 **/
VFSFile *vfs_buffer_new_from_string(gchar *str);

G_END_DECLS

#endif
