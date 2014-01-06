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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "audstrings.h"
#include "multihash.h"

#ifdef VALGRIND_FRIENDLY

typedef struct {
    unsigned hash;
    char magic;
    char str[];
} StrNode;

#define NODE_SIZE_FOR(s) (offsetof (StrNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((StrNode *) ((s) - offsetof (StrNode, str)))

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    StrNode * node = g_malloc (NODE_SIZE_FOR (str));
    node->magic = '@';
    node->hash = g_str_hash (str);

    strcpy (node->str, str);
    return node->str;
}

EXPORT char * str_ref (const char * str)
{
    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (g_str_hash (str) == node->hash);

    return str_get (str);
}

EXPORT void str_unref (char * str)
{
    if (! str)
        return;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (g_str_hash (str) == node->hash);

    node->magic = 0;
    g_free (node);
}

EXPORT unsigned str_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return g_str_hash (str);
}

EXPORT bool_t str_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return ! g_strcmp0 (str1, str2);
}

EXPORT void strpool_shutdown (void)
{
}

#else /* ! VALGRIND_FRIENDLY */

typedef struct {
    MultihashNode node;
    unsigned hash, refs;
    char magic;
    char str[];
} StrNode;

#define NODE_SIZE_FOR(s) (offsetof (StrNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((StrNode *) ((s) - offsetof (StrNode, str)))

static unsigned hash_cb (const MultihashNode * node)
{
    return ((const StrNode *) node)->hash;
}

static bool_t match_cb (const MultihashNode * node_, const void * data, unsigned hash)
{
    const StrNode * node = (const StrNode *) node_;
    return data == node->str || (hash == node->hash && ! strcmp (data, node->str));
}

static MultihashTable strpool_table = {
    .hash_func = hash_cb,
    .match_func = match_cb
};

static MultihashNode * add_cb (const void * data, unsigned hash, void * state)
{
    StrNode * node = g_malloc (NODE_SIZE_FOR (data));
    node->hash = hash;
    node->refs = 1;
    node->magic = '@';
    strcpy (node->str, data);

    * ((char * *) state) = node->str;
    return (MultihashNode *) node;
}

static bool_t ref_cb (MultihashNode * node_, void * state)
{
    StrNode * node = (StrNode *) node_;

    __sync_fetch_and_add (& node->refs, 1);

    * ((char * *) state) = node->str;
    return FALSE;
}

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    char * ret = NULL;
    multihash_lookup (& strpool_table, str, g_str_hash (str), add_cb, ref_cb, & ret);
    return ret;
}

EXPORT char * str_ref (const char * str)
{
    if (! str)
        return NULL;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    __sync_fetch_and_add (& node->refs, 1);

    return (char *) str;
}

static bool_t remove_cb (MultihashNode * node_, void * state)
{
    StrNode * node = (StrNode *) node_;

    if (! __sync_bool_compare_and_swap (& node->refs, 1, 0))
        return FALSE;

    node->magic = 0;
    g_free (node);
    return TRUE;
}

EXPORT void str_unref (char * str)
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
            int status = multihash_lookup (& strpool_table, node->str,
             node->hash, NULL, remove_cb, NULL);

            assert (status & MULTIHASH_FOUND);
            if (status & MULTIHASH_REMOVED)
                break;
        }
    }
}

static bool_t leak_cb (MultihashNode * node, void * state)
{
    fprintf (stderr, "String leaked: %s\n", ((StrNode *) node)->str);
    return FALSE;
}

EXPORT void strpool_shutdown (void)
{
    multihash_iterate (& strpool_table, leak_cb, NULL);
}

EXPORT unsigned str_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return node->hash;
}


EXPORT bool_t str_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return str1 == str2;
}

#endif /* ! VALGRIND_FRIENDLY */

EXPORT char * str_nget (const char * str, int len)
{
    if (memchr (str, 0, len))
        return str_get (str);

    SNCOPY (buf, str, len);
    return str_get (buf);
}
