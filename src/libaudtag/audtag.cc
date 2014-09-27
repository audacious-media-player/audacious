/*
 * audtag.c
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

#include <libaudcore/runtime.h>

#include "audtag.h"
#include "tag_module.h"

/* The tuple's file-related attributes are already set */

namespace audtag {

EXPORT bool tuple_read (Tuple & tuple, VFSFile & handle)
{
    TagModule * module = find_tag_module (handle, TagType::None);

    if (! module)
    {
        AUDINFO ("read_tag() not supported for %s\n", handle.filename ());
        return false;
    }

    return module->read_tag (tuple, handle);
}

EXPORT Index<char> image_read (VFSFile & handle)
{
    TagModule * module = find_tag_module (handle, TagType::None);

    if (! module)
    {
        AUDINFO ("read_image() not supported for %s\n", handle.filename ());
        return Index<char> ();
    }

    return module->read_image (handle);
}

EXPORT bool tuple_write (const Tuple & tuple, VFSFile & handle, TagType new_type)
{
    TagModule * module = find_tag_module (handle, new_type);

    if (! module)
    {
        AUDINFO ("write_tag() not supported for %s\n", handle.filename ());
        return false;
    }

    return module->write_tag (tuple, handle);
}

}
