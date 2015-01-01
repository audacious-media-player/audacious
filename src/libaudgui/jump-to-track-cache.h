/*
 * jump-to-track-cache.h
 * Copyright 2008 Jussi Judin
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

#ifndef LIBAUDGUI_JUMPTOTRACK_CACHE_H
#define LIBAUDGUI_JUMPTOTRACK_CACHE_H

#include <libaudcore/index.h>
#include <libaudcore/multihash.h>
#include <libaudcore/objects.h>

// Struct to keep information about matches from searches.
struct KeywordMatch {
    int entry;
    String title, artist, album, path;
};

typedef Index<KeywordMatch> KeywordMatches;

class JumpToTrackCache : private SimpleHash<String, KeywordMatches>
{
public:
    const KeywordMatches * search (const char * keyword);
    using SimpleHash::clear;

private:
    void init ();
    const KeywordMatches * search_within (const KeywordMatches * subset, const char * keyword);
};

#endif
