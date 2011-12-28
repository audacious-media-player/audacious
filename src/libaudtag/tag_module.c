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
#include <stdio.h>

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "audtag.h"
#include "util.h"
#include "tag_module.h"
#include "id3/id3v1.h"
#include "id3/id3v22.h"
#include "id3/id3v24.h"
#include "ape/ape.h"

static tag_module_t * const modules[] = {& id3v24, & id3v22, & ape, & id3v1};
#define N_MODULES (sizeof modules / sizeof modules[0])

tag_module_t * find_tag_module (VFSFile * fd, int new_type)
{
    int i;

    for (i = 0; i < N_MODULES; i ++)
    {
        if (vfs_fseek(fd, 0, SEEK_SET))
        {
            TAGDBG("not a seekable file\n");
            return NULL;
        }

        if (modules[i]->can_handle_file (fd))
        {
            TAGDBG ("Module %s accepted file.\n", modules[i]->name);
            return modules[i];
        }
    }

    /* No existing tag; see if we can create a new one. */
    if (new_type != TAG_TYPE_NONE)
    {
        for (i = 0; i < N_MODULES; i ++)
        {
            if (modules[i]->type == new_type)
                return modules[i];
        }
    }

    TAGDBG("no module found\n");
    return NULL;
}
