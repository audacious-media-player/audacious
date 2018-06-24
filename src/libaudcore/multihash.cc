/*
 * multihash.c
 * Copyright 2013-2014 John Lindgren
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

EXPORT void HashBase::add (Node * node, unsigned hash)
{
    if (! buckets)
    {
        buckets = new Node *[InitialSize]();
        size = InitialSize;
    }

    unsigned b = hash & (size - 1);
    node->next = buckets[b];
    node->hash = hash;
    buckets[b] = node;

    used ++;
    if (used > size)
        resize (size << 1);
}

EXPORT HashBase::Node * HashBase::lookup (MatchFunc match, const void * data,
 unsigned hash, NodeLoc * loc) const
{
    if (! buckets)
        return nullptr;

    unsigned b = hash & (size - 1);
    Node * * node_ptr = & buckets[b];
    Node * node = * node_ptr;

    while (1)
    {
        if (! node)
            return nullptr;

        if (node->hash == hash && match (node, data))
            break;

        node_ptr = & node->next;
        node = * node_ptr;
    }

    if (loc)
    {
        loc->ptr = node_ptr;
        loc->next = node->next;
    }

    return node;
}

EXPORT void HashBase::remove (const NodeLoc & loc)
{
    * loc.ptr = loc.next;

    used --;
    if (used < size >> 2 && size > InitialSize)
        resize (size >> 1);
}

EXPORT void HashBase::iterate (FoundFunc func, void * state)
{
    for (unsigned b = 0; b < size; b ++)
    {
        Node * * ptr = & buckets[b];
        Node * node = * ptr;

        while (node)
        {
            Node * next = node->next;

            if (func (node, state))
            {
                * ptr = next;
                used --;
            }
            else
                ptr = & node->next;

            node = next;
        }
    }

    if (used < size >> 2 && size > InitialSize)
        resize (size >> 1);
}

void HashBase::resize (unsigned new_size)
{
    Node * * new_buckets = new Node *[new_size]();

    for (unsigned b1 = 0; b1 < size; b1 ++)
    {
        Node * node = buckets[b1];

        while (node)
        {
            Node * next = node->next;

            unsigned b2 = node->hash & (new_size - 1);
            node->next = new_buckets[b2];
            new_buckets[b2] = node;

            node = next;
        }
    }

    delete[] buckets;
    buckets = new_buckets;
    size = new_size;
}

EXPORT int MultiHash::lookup (const void * data, unsigned hash, AddFunc add,
 FoundFunc found, void * state)
{
    const unsigned c = (hash >> Shift) & (Channels - 1);
    HashBase & channel = channels[c];

    int status = 0;
    tiny_lock (& locks[c]);

    HashBase::NodeLoc loc;
    Node * node = channel.lookup (match, data, hash, & loc);

    if (node)
    {
        status |= Found;
        if (found && found (node, state))
        {
            status |= Removed;
            channel.remove (loc);
        }
    }
    else if (add && (node = add (data, state)))
    {
        status |= Added;
        channel.add (node, hash);
    }

    tiny_unlock (& locks[c]);
    return status;
}

EXPORT void MultiHash::iterate (FoundFunc func, void * state)
{
    iterate (func, state, nullptr, nullptr);
}

EXPORT void MultiHash::iterate (FoundFunc func, void * state, FinalFunc final, void * fstate)
{
    for (TinyLock & lock : locks)
        tiny_lock (& lock);

    for (HashBase & channel : channels)
        channel.iterate (func, state);

    if (final)
        final (fstate);

    for (TinyLock & lock : locks)
        tiny_unlock (& lock);
}
