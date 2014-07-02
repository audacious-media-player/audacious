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

#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include <libaudcore/index.h>

#include "audtag.h"
#include "util.h"
#include "tag_module.h"
#include "builtin.h"

namespace audtag {

static Index<TagModule *> modules;
static bool init_builtin_modules = false;

static void setup_builtin_modules ()
{
    if (init_builtin_modules)
        return;

    static APETagModule m_builtin_ape;
    static ID3v1TagModule m_builtin_id3v1;
    static ID3v22TagModule m_builtin_id3v22;
    static ID3v24TagModule m_builtin_id3v24;

    modules.append (& m_builtin_id3v24);
    modules.append (& m_builtin_id3v22);
    modules.append (& m_builtin_id3v1);
    modules.append (& m_builtin_ape);

    init_builtin_modules = true;
}

TagModule * find_tag_module (VFSFile * fd, int new_type)
{
    if (! init_builtin_modules)
        setup_builtin_modules();

    for (TagModule * module : modules)
    {
        if (vfs_fseek(fd, 0, SEEK_SET))
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
    if (new_type != audtag::TAG_TYPE_NONE)
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
bool TagModule::can_handle_file (VFSFile * handle)
{
    AUDDBG("Module %s does not support %s (no probing function implemented).\n", this->m_name,
           vfs_get_filename(handle));
    return false;
}

bool TagModule::read_image (VFSFile * handle, void * * data, int64_t * size)
{
    AUDDBG("Module %s does not support images.\n", this->m_name);
    return false;
}

bool TagModule::read_tag (Tuple & tuple, VFSFile * handle)
{
    AUDDBG ("%s: read_tag() not implemented.\n", this->m_name);
    return false;
}

bool TagModule::write_tag (Tuple const & tuple, VFSFile * handle)
{
    AUDDBG ("%s: write_tag() not implemented.\n", this->m_name);
    return false;
}

}
