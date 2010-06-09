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

#include <glib.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "audtag.h"
#include "util.h"
#include "tag_module.h"
#include "wma/module.h"
#include "id3/id3v1.h"
#include "id3/id3v22.h"
#include "id3/id3v24.h"
#include "ape/ape.h"
/* #include "aac/aac.h" */

void init_tag_modules(void)
{
    mowgli_node_add((void *)&id3v24, &id3v24.node, &tag_modules);
    mowgli_node_add((void *)&id3v22, &id3v22.node, &tag_modules);
    mowgli_node_add((void *)&ape, &ape.node, &tag_modules);
    mowgli_node_add((void *)&id3v1, &id3v1.node, &tag_modules);
}

tag_module_t * find_tag_module (VFSFile * fd, gint new_type)
{
    mowgli_node_t *mod, *tmod;
    MOWGLI_LIST_FOREACH_SAFE(mod, tmod, tag_modules.head)
    {
        if (vfs_fseek(fd, 0, SEEK_SET))
        {
            AUDDBG("not a seekable file\n");
            return NULL;
        }
        if (((tag_module_t *) (mod->data))->can_handle_file(fd))
            return (tag_module_t *) (mod->data);
    }

    /* No existing tag; see if we can create a new one. */
    if (new_type != TAG_TYPE_NONE)
    {
        MOWGLI_LIST_FOREACH_SAFE (mod, tmod, tag_modules.head)
        {
            if (((tag_module_t *) (mod->data))->type == new_type)
                return mod->data;
        }
    }

    AUDDBG("no module found\n");
    return NULL;
}
