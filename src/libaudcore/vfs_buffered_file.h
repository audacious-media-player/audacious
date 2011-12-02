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

#ifndef LIBAUDCORE_VFS_BUFFERED_FILE_H
#define LIBAUDCORE_VFS_BUFFERED_FILE_H

#include <libaudcore/vfs.h>

/** Private data for the VFS memorybuffer class. */
typedef struct {
	VFSFile    *fd;     /**< VFS handle for the active FD. */
	VFSFile    *buffer; /**< First 32kb read from the FD. */
	char      *mem;    /**< The memory for the buffer. */
	boolean    which;  /**< Whether to use the live FD or the buffer. */
} VFSBufferedFile;

VFSFile *vfs_buffered_file_new_from_uri(const char *uri);
VFSFile *vfs_buffered_file_release_live_fd(VFSFile *fd);

#endif /* AUDACIOUS_VFS_BUFFERED_FILE_H */
