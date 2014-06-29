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

#include <vector>

#include <glib.h>
#include <stdio.h>

#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "audtag.h"
#include "util.h"
#include "tag_module.h"

namespace audtag {
    static std::vector<TagModule> modules;

    TagModule * find_tag_module (VFSFile * fd, int new_type)
    {
        for (auto &mod : modules)
        {
            if (vfs_fseek(fd, 0, SEEK_SET))
            {
                AUDDBG("not a seekable file\n");
                return nullptr;
            }

            if (mod.can_handle_file (fd))
            {
                AUDDBG ("Module %s accepted file.\n", mod.m_name.c_str());
                return &mod;
            }
        }

        /* No existing tag; see if we can create a new one. */
        if (new_type != TAG_TYPE_NONE)
        {
            for (auto &mod : modules)
            {
                if (mod.m_type == new_type)
                    return &mod;
            }
        }

        AUDDBG("no module found\n");
        return nullptr;
    }

    /******************************************************************************************************************/

    TagModule::TagModule (const char *name, int type) :
        m_name(name), m_type(type)
    {
        modules.push_back(*this);
    }

    TagModule::~TagModule ()
    {
        // modules.erase(*this);
    }

    bool TagModule::can_handle_file (VFSFile * handle)
    {
        AUDDBG("Module %s does not support %s (no probing function implemented).\n", this->m_name.c_str(),
               vfs_get_filename(handle));
        return false;
    }

    bool TagModule::read_image (VFSFile * handle, void * * data, int64_t * size)
    {
        AUDDBG("Module %s does not support images.\n", this->m_name.c_str());
        return false;
    }

    bool TagModule::read_tag (Tuple & tuple, VFSFile * handle)
    {
        AUDDBG ("%s: read_tag() not implemented.\n", this->m_name.c_str());
        return false;
    }

    bool TagModule::write_tag (Tuple const & tuple, VFSFile * handle)
    {
        AUDDBG ("%s: write_tag() not implemented.\n", this->m_name.c_str());
        return false;
>>>>>>> audtag: C++ object system conversion
    }
};

