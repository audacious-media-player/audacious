/*
 * audacious: Cross-platform multimedia player.
 * vfs_buffered_file.h: VFS Buffered I/O support.                    
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

#ifndef AUDACIOUS_VFS_BUFFERED_FILE_H
#define AUDACIOUS_VFS_BUFFERED_FILE_H

#include <glib.h>
#include "vfs.h"
#include "vfs_buffer.h"

G_BEGIN_DECLS

/**
 * VFSBufferedFile:
 * @fd: The VFS handle for the active FD.
 * @buffer: The first 32kb read from the FD.
 * @mem: The memory for the buffer.
 * @which: Whether to use the live FD or the buffer.
 *
 * Private data for the VFS memorybuffer class.
 **/

typedef struct {
	VFSFile    *fd;
	VFSFile    *buffer;
	gchar      *mem;
	gboolean    which;
} VFSBufferedFile;

/**
 * vfs_buffered_file_new_from_uri:
 * @uri: The location to read from.
 *
 * Creates a VFSBufferedFile. VFSBufferedFile is read-only.
 *
 * Return value: A VFSFile handle for the VFSBufferedFile.
 **/
VFSFile *vfs_buffered_file_new_from_uri(const gchar *uri);

VFSFile *vfs_buffered_file_release_live_fd(VFSFile *fd);


G_END_DECLS

#endif
