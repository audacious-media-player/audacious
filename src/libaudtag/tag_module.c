/*
 * tag_module.c
 * Copyright 2009-2011 Paula Stanciu and John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
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

tag_module_t * find_tag_module (VFSFile * fd, int new_type)
{
    int i;

    for (i = 0; i < ARRAY_LEN (modules); i ++)
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
        for (i = 0; i < ARRAY_LEN (modules); i ++)
        {
            if (modules[i]->type == new_type)
                return modules[i];
        }
    }

    TAGDBG("no module found\n");
    return NULL;
}
