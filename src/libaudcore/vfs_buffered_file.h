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
/**
 * @file vfs_buffered_file.h
 * Functions and structures for buffered file access over VFS.
 */

#ifndef AUDACIOUS_VFS_BUFFERED_FILE_H
#define AUDACIOUS_VFS_BUFFERED_FILE_H

#include <glib.h>
#include "vfs.h"
#include "vfs_buffer.h"

G_BEGIN_DECLS

/** Private data for the VFS memorybuffer class. */
typedef struct {
	VFSFile    *fd;     /**< VFS handle for the active FD. */
	VFSFile    *buffer; /**< First 32kb read from the FD. */
	gchar      *mem;    /**< The memory for the buffer. */
	gboolean    which;  /**< Whether to use the live FD or the buffer. */
} VFSBufferedFile;

VFSFile *vfs_buffered_file_new_from_uri(const gchar *uri);
VFSFile *vfs_buffered_file_release_live_fd(VFSFile *fd);

G_END_DECLS

#endif /* AUDACIOUS_VFS_BUFFERED_FILE_H */
