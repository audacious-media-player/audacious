/*
 * Audacious
 * Copyright (c) 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_VFS_ASYNC_H
#define AUDACIOUS_VFS_ASYNC_H

#include <libaudcore/vfs.h>

typedef gboolean (*VFSConsumer)(gpointer buf, gint64 size, gpointer userdata);

void vfs_async_file_get_contents(const gchar *filename, VFSConsumer cons_f, gpointer userdata);

#endif
