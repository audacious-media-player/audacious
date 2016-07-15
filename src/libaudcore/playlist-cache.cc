/*
 * playlist-cache.h
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

#include "playlist-internal.h"
#include "mainloop.h"
#include "multihash.h"

#include <pthread.h>

static SimpleHash<String, PlaylistAddItem> cache;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static QueuedFunc clear_timer;

EXPORT void aud_playlist_cache_selected (int playlist)
{
    pthread_mutex_lock (& mutex);

    int entries = aud_playlist_entry_count (playlist);

    for (int i = 0; i < entries; i ++)
    {
        if (! aud_playlist_entry_get_selected (playlist, i))
            continue;

        String filename = aud_playlist_entry_get_filename (playlist, i);
        Tuple tuple = aud_playlist_entry_get_tuple (playlist, i, Playlist::NoWait);
        PluginHandle * decoder = aud_playlist_entry_get_decoder (playlist, i, Playlist::NoWait);

        if (tuple.valid () || decoder)
            cache.add (filename, {filename, std::move (tuple), decoder});
    }

    clear_timer.queue (30000, (QueuedFunc::Func) playlist_cache_clear, nullptr);

    pthread_mutex_unlock (& mutex);
}

void playlist_cache_load (Index<PlaylistAddItem> & items)
{
    pthread_mutex_lock (& mutex);

    if (! cache.n_items ())
        goto out;

    for (auto & item : items)
    {
        if (item.tuple.valid () && item.decoder)
            continue;

        auto node = cache.lookup (item.filename);
        if (! node)
            continue;

        if (! item.tuple.valid () && node->tuple.valid ())
            item.tuple = node->tuple.ref ();
        if (! item.decoder && node->decoder)
            item.decoder = node->decoder;
    }

out:
    pthread_mutex_unlock (& mutex);
}

void playlist_cache_clear ()
{
    pthread_mutex_lock (& mutex);

    cache.clear ();
    clear_timer.stop ();

    pthread_mutex_unlock (& mutex);
}
