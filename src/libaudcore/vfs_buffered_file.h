/*  Audacious
 *  Copyright (c) 2006-2007 William Pitcock
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
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

#endif /* AUDACIOUS_VFS_BUFFERED_FILE_H */
