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

EXPORT bool read_tag (VFSFile & file, Tuple * tuple, Index<char> * image)
{
    TagModule * module = find_tag_module (file, TagType::None);

    if (! module)
    {
        AUDINFO ("read_tag() not supported for %s\n", file.filename ());
        return false;
    }

    return module->read_tag (file, tuple, image);
}

EXPORT bool tuple_read (Tuple & tuple, VFSFile & file)
{
    return read_tag (file, & tuple, nullptr);
}

EXPORT Index<char> image_read (VFSFile & file)
{
    Index<char> image;
    read_tag (file, nullptr, & image);
    return image;
}

EXPORT bool tuple_write (const Tuple & tuple, VFSFile & file, TagType new_type)
{
    TagModule * module = find_tag_module (file, new_type);

    if (! module)
    {
        AUDINFO ("write_tag() not supported for %s\n", file.filename ());
        return false;
    }

    return module->write_tag (file, tuple);
}

}
