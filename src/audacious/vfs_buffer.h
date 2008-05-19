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

#endif /* AUDACIOUS_VFS_BUFFER_H */
