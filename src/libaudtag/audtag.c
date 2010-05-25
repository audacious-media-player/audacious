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

gboolean tag_tuple_read (Tuple * tuple, VFSFile * handle)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (module == NULL)
        return FALSE;

    return module->read_tag (tuple, handle);
}

gboolean tag_image_read (VFSFile * handle, void * * data, gint * size)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (module == NULL || module->read_image == NULL)
        return FALSE;

    return module->read_image (handle, data, size);
}

gboolean tag_tuple_write (Tuple * tuple, VFSFile * handle, gint new_type)
{
    tag_module_t * module = find_tag_module (handle, new_type);

    if (module == NULL)
        return FALSE;

    return module->write_tag (tuple, handle);
}

/* deprecated */
gboolean tag_tuple_write_to_file (Tuple * tuple, VFSFile * handle)
{
    return tag_tuple_write (tuple, handle, TAG_TYPE_NONE);
}
