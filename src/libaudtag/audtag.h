/*
 * audtag.h
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

#ifndef AUDTAG_H
#define AUDTAG_H

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

namespace audtag {

enum class TagType
{
    None,
    APE,
    ID3v2
};

bool read_tag (VFSFile & file, Tuple & tuple, Index<char> * image);

/* new_type specifies the type of tag (see the TagType enum) that should be
 * written if the file does not have any existing tag. */
bool write_tuple (VFSFile & file, const Tuple & tuple, TagType new_type);

}

#endif /* AUDTAG_H */
