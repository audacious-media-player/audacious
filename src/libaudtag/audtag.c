/*
 * Copyright 2009 Paula Stanciu
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

#include "libaudcore/tuple.h"
#include "audtag.h"
#include "tag_module.h"
#include "util.h"

void tag_init(void)
{
    init_tag_modules();
}

/* The tuple's file-related attributes are already set */

gboolean tag_tuple_read (Tuple * tuple, VFSFile * fd)
{
    tag_module_t *mod = find_tag_module(fd);

    if (mod == NULL)
        return FALSE;

    AUDDBG("Tag module %s has accepted %s\n", mod->name, fd->uri);
    return mod->populate_tuple_from_file (tuple, fd) != NULL;
}

gboolean tag_tuple_write_to_file(Tuple * tuple, VFSFile * fd)
{
    tag_module_t *mod = find_tag_module(fd);

    if (mod == NULL)
        return FALSE;

    return mod->write_tuple_to_file(tuple, fd);
}
