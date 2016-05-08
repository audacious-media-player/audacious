/*
 * cue-cache.h
 * Copyright 2016 John Lindgren
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

#ifndef LIBAUDCORE_CUE_CACHE_H
#define LIBAUDCORE_CUE_CACHE_H

#include "index.h"
#include "tuple.h"

struct CueCacheNode;

class CueCacheRef
{
public:
    CueCacheRef (const char * filename);
    ~CueCacheRef ();

    const Index<PlaylistAddItem> & load ();

private:
    String m_filename;
    CueCacheNode * m_node;
};

#endif // LIBAUDCORE_CUE_CACHE_H
