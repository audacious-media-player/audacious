/*
 * cue-cache.cc
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

#include "cue-cache.h"
#include "multihash.h"
#include "playlist-internal.h"
#include "threads.h"

enum NodeState
{
    NotLoaded,
    Loading,
    Loaded
};

struct CueCacheNode
{
    Index<PlaylistAddItem> items;
    NodeState state = NotLoaded;
    int refcount = 0;
};

static SimpleHash<String, CueCacheNode> cache;
static aud::mutex mutex;
static aud::condvar cond;

CueCacheRef::CueCacheRef(const char * filename) : m_filename(filename)
{
    auto mh = mutex.take();

    m_node = cache.lookup(m_filename);
    if (!m_node)
        m_node = cache.add(m_filename, CueCacheNode());

    m_node->refcount++;
}

CueCacheRef::~CueCacheRef()
{
    auto mh = mutex.take();

    m_node->refcount--;
    if (!m_node->refcount)
        cache.remove(m_filename);
}

const Index<PlaylistAddItem> & CueCacheRef::load()
{
    auto mh = mutex.take();
    String title; // not used

    switch (m_node->state)
    {
    case NotLoaded:
        // load the cuesheet in this thread
        m_node->state = Loading;
        mh.unlock();
        playlist_load(m_filename, title, m_node->items);
        mh.lock();

        m_node->state = Loaded;
        cond.notify_all();
        break;

    case Loading:
        // wait for cuesheet to load in another thread
        while (m_node->state != Loaded)
            cond.wait(mh);

        break;

    case Loaded:
        // cuesheet already loaded
        break;
    }

    return m_node->items;
}
