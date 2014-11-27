/*
 * strpool.c
 * Copyright 2011-2013 John Lindgren
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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "audstrings.h"
#include "internal.h"
#include "multihash.h"
#include "objects.h"
#include "runtime.h"

#ifdef VALGRIND_FRIENDLY

struct StrNode {
    unsigned hash;
    char magic;
    char str[];
};

#define NODE_SIZE_FOR(s) (offsetof (StrNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((StrNode *) ((s) - offsetof (StrNode, str)))

EXPORT char * String::raw_get (const char * str)
{
    if (! str)
        return nullptr;

    StrNode * node = (StrNode *) malloc (NODE_SIZE_FOR (str));
    if (! node)
        throw std::bad_alloc ();

    node->magic = '@';
    node->hash = str_calc_hash (str);

    strcpy (node->str, str);
    return node->str;
}

EXPORT char * String::raw_ref (const char * str)
{
    if (! str)
        return nullptr;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (str_calc_hash (str) == node->hash);

    return raw_get (str);
}

EXPORT void String::raw_unref (char * str)
{
    if (! str)
        return;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (str_calc_hash (str) == node->hash);

    node->magic = 0;
    free (node);
}

EXPORT unsigned String::raw_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return str_calc_hash (str);
}

EXPORT bool String::raw_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return ! strcmp_safe (str1, str2);
}

EXPORT void string_leak_check ()
{
}

#else /* ! VALGRIND_FRIENDLY */

struct StrNode {
    MultiHash::Node base;
    unsigned refs;
    char magic;
    char str[1];  // variable size
};

#define NODE_SIZE_FOR(s) (offsetof (StrNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((StrNode *) ((s) - offsetof (StrNode, str)))

static bool match_cb (const MultiHash::Node * node_, const void * data_)
{
    const StrNode * node = (const StrNode *) node_;
    const char * data = (const char *) data_;

    return data == node->str || ! strcmp (data, node->str);
}

static MultiHash strpool_table (match_cb);

static MultiHash::Node * add_cb (const void * data_, void * state)
{
    const char * data = (const char *) data_;

    StrNode * node = (StrNode *) malloc (NODE_SIZE_FOR (data));
    if (! node)
        throw std::bad_alloc ();

    node->refs = 1;
    node->magic = '@';
    strcpy (node->str, data);

    * ((char * *) state) = node->str;
    return (MultiHash::Node *) node;
}

static bool ref_cb (MultiHash::Node * node_, void * state)
{
    StrNode * node = (StrNode *) node_;

    __sync_fetch_and_add (& node->refs, 1);

    * ((char * *) state) = node->str;
    return false;
}

/* If the pool contains a copy of <str>, increments its reference count.
 * Otherwise, adds a copy of <str> to the pool with a reference count of one.
 * In either case, returns the copy.  Because this copy may be shared by other
 * parts of the code, it should not be modified.  If <str> is null, simply
 * returns null with no side effects. */
EXPORT char * String::raw_get (const char * str)
{
    if (! str)
        return nullptr;

    char * ret = nullptr;
    strpool_table.lookup (str, str_calc_hash (str), add_cb, ref_cb, & ret);
    return ret;
}

/* Increments the reference count of <str>, where <str> is the address of a
 * string already in the pool.  Faster than calling raw_get() a second time.
 * Returns <str> for convenience.  If <str> is null, simply returns null with no
 * side effects. */
EXPORT char * String::raw_ref (const char * str)
{
    if (! str)
        return nullptr;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    __sync_fetch_and_add (& node->refs, 1);

    return (char *) str;
}

static bool remove_cb (MultiHash::Node * node_, void * state)
{
    StrNode * node = (StrNode *) node_;

    if (! __sync_bool_compare_and_swap (& node->refs, 1, 0))
        return false;

    node->magic = 0;
    free (node);
    return true;
}

/* Decrements the reference count of <str>, where <str> is the address of a
 * string in the pool.  If the reference count drops to zero, releases the
 * memory used by <str>.   If <str> is null, simply returns null with no side
 * effects. */
EXPORT void String::raw_unref (char * str)
{
    if (! str)
        return;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    while (1)
    {
        int refs = __sync_fetch_and_add (& node->refs, 0);

        if (refs > 1)
        {
            if (__sync_bool_compare_and_swap (& node->refs, refs, refs - 1))
                break;
        }
        else
        {
            int status = strpool_table.lookup (node->str, node->base.hash, nullptr,
             remove_cb, nullptr);

            assert (status & MultiHash::Found);
            if (status & MultiHash::Removed)
                break;
        }
    }
}

static bool leak_cb (MultiHash::Node * node, void * state)
{
    AUDWARN ("String leaked: %s\n", ((StrNode *) node)->str);
    return false;
}

void string_leak_check ()
{
    strpool_table.iterate (leak_cb, nullptr);
}

/* Returns the cached hash value of a pooled string (or 0 for null). */
EXPORT unsigned String::raw_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return node->base.hash;
}


/* Checks whether two pooled strings are equal.  Since the pool never contains
 * duplicate strings, this is a simple pointer comparison and thus much faster
 * than strcmp().  null is considered equal to null but not equal to any string. */
EXPORT bool String::raw_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return str1 == str2;
}

#endif /* ! VALGRIND_FRIENDLY */
