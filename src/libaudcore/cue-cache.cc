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

#include <pthread.h>

enum NodeState {NotLoaded, Loading, Loaded};

struct CueCacheNode {
    Index<PlaylistAddItem> items;
    NodeState state = NotLoaded;
    int refcount = 0;
};

static SimpleHash<String, CueCacheNode> cache;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

CueCacheRef::CueCacheRef (const char * filename) :
    m_filename (filename)
{
    pthread_mutex_lock (& mutex);

    m_node = cache.lookup (m_filename);
    if (! m_node)
        m_node = cache.add (m_filename, CueCacheNode ());

    m_node->refcount ++;

    pthread_mutex_unlock (& mutex);
}

CueCacheRef::~CueCacheRef ()
{
    pthread_mutex_lock (& mutex);

    m_node->refcount --;
    if (! m_node->refcount)
        cache.remove (m_filename);

    pthread_mutex_unlock (& mutex);
}

const Index<PlaylistAddItem> & CueCacheRef::load ()
{
    String title; // not used
    pthread_mutex_lock (& mutex);

    switch (m_node->state)
    {
    case NotLoaded:
        // load the cuesheet in this thread
        m_node->state = Loading;
        pthread_mutex_unlock (& mutex);
        playlist_load (m_filename, title, m_node->items);
        pthread_mutex_lock (& mutex);

        m_node->state = Loaded;
        pthread_cond_broadcast (& cond);
        break;

    case Loading:
        // wait for cuesheet to load in another thread
        while (m_node->state != Loaded)
            pthread_cond_wait (& cond, & mutex);

        break;

    case Loaded:
        // cuesheet already loaded
        break;
    }

    pthread_mutex_unlock (& mutex);
    return m_node->items;
}
