/*
 * multihash.c
 * Copyright 2013 John Lindgren
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

#include "multihash.h"

#include <glib.h>

#define INITIAL_SIZE 256  /* must be a power of two */

static void resize_channel (MultihashTable * table, MultihashChannel * channel, unsigned size)
{
    MultihashNode * * buckets = g_new0 (MultihashNode *, size);

    for (int b1 = 0; b1 < channel->size; b1 ++)
    {
        MultihashNode * node = channel->buckets[b1];

        while (node)
        {
            MultihashNode * next = node->next;

            unsigned hash = table->hash_func (node);
            unsigned b2 = (hash >> MULTIHASH_SHIFT) & (size - 1);
            MultihashNode * * node_ptr = & buckets[b2];

            node->next = * node_ptr;
            * node_ptr = node;

            node = next;
        }
    }

    g_free (channel->buckets);
    channel->buckets = buckets;
    channel->size = size;
}

EXPORT int multihash_lookup (MultihashTable * table, const void * data,
 unsigned hash, MultihashAddFunc add, MultihashActionFunc action, void * state)
{
    unsigned c = hash & (MULTIHASH_CHANNELS - 1);
    MultihashChannel * channel = & table->channels[c];

    int status = 0;
    tiny_lock (& channel->lock);

    if (! channel->buckets)
    {
        if (! add)
            goto DONE;

        channel->buckets = g_new0 (MultihashNode *, INITIAL_SIZE);
        channel->size = INITIAL_SIZE;
        channel->used = 0;
    }

    unsigned b = (hash >> MULTIHASH_SHIFT) & (channel->size - 1);
    MultihashNode * * node_ptr = & channel->buckets[b];
    MultihashNode * node = * node_ptr;

    while (node && ! table->match_func (node, data, hash))
    {
        node_ptr = & node->next;
        node = * node_ptr;
    }

    if (node)
    {
        status |= MULTIHASH_FOUND;

        MultihashNode * next = node->next;

        if (action && action (node, state))
        {
            status |= MULTIHASH_REMOVED;

            * node_ptr = next;

            channel->used --;
            if (channel->used < channel->size >> 2 && channel->size > INITIAL_SIZE)
                resize_channel (table, channel, channel->size >> 1);
        }
    }
    else if (add && (node = add (data, hash, state)))
    {
        status |= MULTIHASH_ADDED;

        * node_ptr = node;
        node->next = NULL;

        channel->used ++;
        if (channel->used > channel->size)
            resize_channel (table, channel, channel->size << 1);
    }

DONE:
    tiny_unlock (& channel->lock);
    return status;
}

EXPORT void multihash_iterate (MultihashTable * table, MultihashActionFunc action, void * state)
{
    for (int c = 0; c < MULTIHASH_CHANNELS; c ++)
        tiny_lock (& table->channels[c].lock);

    for (int c = 0; c < MULTIHASH_CHANNELS; c ++)
    {
        MultihashChannel * channel = & table->channels[c];

        for (int b = 0; b < channel->size; b ++)
        {
            MultihashNode * * node_ptr = & channel->buckets[b];
            MultihashNode * node = * node_ptr;

            while (node)
            {
                MultihashNode * next = node->next;

                if (action (node, state))
                {
                    * node_ptr = next;
                    channel->used --;
                }
                else
                    node_ptr = & node->next;

                node = next;
            }
        }

        if (channel->used < channel->size >> 2 && channel->size > INITIAL_SIZE)
            resize_channel (table, channel, channel->size >> 1);
    }

    for (int c = 0; c < MULTIHASH_CHANNELS; c ++)
        tiny_unlock (& table->channels[c].lock);
}
