/*
 * multihash.h
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

#ifndef LIBAUDCORE_MULTIHASH_H
#define LIBAUDCORE_MULTIHASH_H

#include <libaudcore/core.h>
#include <libaudcore/tinylock.h>

/* Multihash is a generic, thread-safe hash table.  It scales well to multiple
 * processors by the use of multiple channels, each with a separate lock.  The
 * hash value of a given node decides what channel it is stored in.  Hence,
 * different processors will tend to hit different channels, keeping lock
 * contention to a minimum.  The data structures are public in order to allow
 * static initialization (setting all bytes to zero gives the initial state).
 * The all-purpose lookup function enables a variety of atomic operations, such
 * as allocating and adding a node only if not already present. */

#define MULTIHASH_CHANNELS 16  /* must be a power of two */
#define MULTIHASH_SHIFT     4  /* log (base 2) of MULTIHASH_CHANNELS */

#define MULTIHASH_FOUND    (1 << 0)
#define MULTIHASH_ADDED    (1 << 1)
#define MULTIHASH_REMOVED  (1 << 2)

/* Skeleton structure containing internal member(s) of a multihash node.  Actual
 * node structures should be defined with MultihashNode as the first member. */
typedef struct _MultihashNode {
    struct _MultihashNode * next;
} MultihashNode;

/* Single channel of a multihash table.  For internal use only. */
typedef struct {
    TinyLock lock;
    MultihashNode * * buckets;
    unsigned size, used;
} MultihashChannel;

/* Callback.  Calculates (or retrieves) the hash value of <node>. */
typedef unsigned (* MultihashFunc) (const MultihashNode * node);

/* Callback.  Returns TRUE if <node> matches <data>, otherwise FALSE. */
typedef bool_t (* MultihashMatchFunc) (const MultihashNode * node,
 const void * data, unsigned hash);

/* Multihash table.  <hash_func> and <match_func> should be initialized to
 * functions appropriate for the type of data to be stored in the table. */
typedef struct {
    MultihashFunc hash_func;
    MultihashMatchFunc match_func;
    MultihashChannel channels[MULTIHASH_CHANNELS];
} MultihashTable;

/* Callback.  May create a new node representing <data> to be added to the
 * table.  Returns the new node or NULL. */
typedef MultihashNode * (* MultihashAddFunc) (const void * data, unsigned hash, void * state);

/* Callback.  Performs a user-defined action when a matching node is found.
 * Doubles as a node removal function.  Returns TRUE if <node> was freed and is
 * to be removed, otherwise FALSE. */
typedef bool_t (* MultihashActionFunc) (MultihashNode * node, void * state);

/* All-purpose lookup function.  The caller passes in the data to be looked up
 * along with its hash value.  The two callbacks are optional.  <add> (if not
 * NULL) is called if no matching node is found, and may return a new node to
 * add to the table.  <action> (if not NULL) is called if a matching node is
 * found, and may return TRUE to remove the node from the table.  <state> is
 * forwarded to either callback.  Returns the status of the lookup as a bitmask
 * of MULTIHASH_FOUND, MULTIHASH_ADDED, and MULTIHASH_REMOVED. */
int multihash_lookup (MultihashTable * table, const void * data, unsigned hash,
 MultihashAddFunc add, MultihashActionFunc action, void * state);

/* All-purpose iteration function.  All channels of the table are locked
 * simultaneously during the iteration to freeze the table in a consistent
 * state.  <action> is called on each node in order, and may return TRUE to
 * remove the node from the table.  <state> is forwarded to <action>. */
void multihash_iterate (MultihashTable * table, MultihashActionFunc action, void * state);

#endif /* LIBAUDCORE_MULTIHASH_H */
