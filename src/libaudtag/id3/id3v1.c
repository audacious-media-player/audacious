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

#include <glib.h>
#include <glib/gstdio.h>

#include "id3v1.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

gboolean id3v1_can_handle_file(VFSFile * f)
{
    gchar *tag = g_new0(gchar, 4);
    vfs_fseek(f, -128, SEEK_END);
    tag = read_char_data(f,3);
    if (!strncmp(tag, "TAG", 3))
        return TRUE;
    return FALSE;
}

Tuple *id3v1_populate_tuple_from_file(Tuple * tuple, VFSFile * f)
{
    return tuple;
}

gboolean id3v1_write_tuple_to_file(Tuple * tuple, VFSFile * f)
{
    return FALSE;
}
