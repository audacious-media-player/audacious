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

#include <libaudcore/index.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "audtag.h"
#include "util.h"
#include "tag_module.h"
#include "builtin.h"

namespace audtag {

static APETagModule ape;
static ID3v1TagModule id3v1;
static ID3v22TagModule id3v22;
static ID3v24TagModule id3v24;

static TagModule * const modules[] = {& id3v24, & id3v22, & ape, & id3v1};

TagModule * find_tag_module (VFSFile & fd, TagType new_type)
{
    for (TagModule * module : modules)
    {
        if (fd.fseek (0, VFS_SEEK_SET))
        {
            AUDDBG("not a seekable file\n");
            return nullptr;
        }

        if (module->can_handle_file (fd))
        {
            AUDDBG ("Module %s accepted file.\n", module->m_name);
            return module;
        }
    }

    /* No existing tag; see if we can create a new one. */
    if (new_type != TagType::None)
    {
        for (TagModule * module : modules)
        {
            if (module->m_type == new_type)
                return module;
        }
    }

    AUDDBG("no module found\n");
    return nullptr;
}

/**************************************************************************************************************
 * tag module object management                                                                               *
 **************************************************************************************************************/
bool TagModule::can_handle_file (VFSFile & file)
{
    AUDDBG("Module %s does not support %s (no probing function implemented).\n", m_name,
           file.filename ());
    return false;
}

bool TagModule::read_tag (VFSFile & file, Tuple & tuple, Index<char> * image)
{
    AUDDBG ("%s: read_tag() not implemented.\n", m_name);
    return false;
}

bool TagModule::write_tag (VFSFile & file, Tuple const & tuple)
{
    AUDDBG ("%s: write_tag() not implemented.\n", m_name);
    return false;
}

}
