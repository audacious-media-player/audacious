/*
 * Copyright 2010 Tony Vroon
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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

#ifndef ID3V1_H

#define ID3V1_H

#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

/* TAG plugin API */
gboolean id3v1_can_handle_file(VFSFile *f);
Tuple *id3v1_populate_tuple_from_file(Tuple *tuple,VFSFile *f);
gboolean id3v1_write_tuple_to_file(Tuple* tuple, VFSFile *f);

static tag_module_t id3v1 = {
    .name = "ID3v1",
    .can_handle_file = id3v1_can_handle_file,
    .populate_tuple_from_file = id3v1_populate_tuple_from_file,
    .write_tuple_to_file = id3v1_write_tuple_to_file,
};
#endif
