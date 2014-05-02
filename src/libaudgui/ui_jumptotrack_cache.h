/*
 * ui_jumptotrack_cache.h
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

#ifndef LIBAUDGUI_UI_JUMPTOTRACK_CACHE_H
#define LIBAUDGUI_UI_JUMPTOTRACK_CACHE_H

#include <glib.h>

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

// Struct to keep information about matches from searches.
struct KeywordMatch {
    int entry;
    String title, artist, album, path;
};

typedef Index<KeywordMatch> KeywordMatches;

typedef GHashTable JumpToTrackCache;

JumpToTrackCache* ui_jump_to_track_cache_new (void);
const KeywordMatches * ui_jump_to_track_cache_search (JumpToTrackCache * cache, const char * keyword);
void ui_jump_to_track_cache_free (JumpToTrackCache * cache);

#endif
