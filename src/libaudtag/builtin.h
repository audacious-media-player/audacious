/*
 * builtin.h
 * Copyright (c) 2014 William Pitcock
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

#include "tag_module.h"

#ifndef __LIBAUDTAG_BUILTIN_H__
#define __LIBAUDTAG_BUILTIN_H__

namespace audtag {

struct ID3v1TagModule : TagModule
{
    constexpr ID3v1TagModule () : TagModule ("ID3v1", TagType::None) {}

    bool can_handle_file (VFSFile & file);
    bool read_tag (VFSFile & file, Tuple & tuple, Index<char> * image);
};

struct ID3v22TagModule : TagModule
{
    constexpr ID3v22TagModule () : TagModule ("ID3v2.2", TagType::None) {}

    bool can_handle_file (VFSFile & file);
    bool read_tag (VFSFile & file, Tuple & tuple, Index<char> * image);
};

struct ID3v24TagModule : TagModule
{
    constexpr ID3v24TagModule () : TagModule ("ID3v2.3/v2.4", TagType::ID3v2) {}

    bool can_handle_file (VFSFile & file);
    bool read_tag (VFSFile & file, Tuple & tuple, Index<char> * image);
    bool write_tag (VFSFile & file, const Tuple & tuple);
};

struct APETagModule : TagModule
{
    constexpr APETagModule () : TagModule ("APE", TagType::APE) {}

    bool can_handle_file (VFSFile & file);
    bool read_tag (VFSFile & file, Tuple & tuple, Index<char> * image);
    bool write_tag (VFSFile & file, const Tuple & tuple);
};

}

#endif
