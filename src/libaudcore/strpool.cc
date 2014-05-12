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

struct StrNode {
    unsigned hash;
    char magic;
    char str[];
};

#define NODE_SIZE_FOR(s) (offsetof (StrNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((StrNode *) ((s) - offsetof (StrNode, str)))

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    StrNode * node = (StrNode *) g_malloc (NODE_SIZE_FOR (str));
    node->magic = '@';
    node->hash = str_calc_hash (str);

    strcpy (node->str, str);
    return node->str;
}

EXPORT char * str_ref (const char * str)
{
    if (! str)
        return NULL;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (str_calc_hash (str) == node->hash);

    return str_get (str);
}

EXPORT void str_unref (char * str)
{
    if (! str)
        return;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');
    assert (str_calc_hash (str) == node->hash);

    node->magic = 0;
    g_free (node);
}

EXPORT unsigned str_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return str_calc_hash (str);
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

    StrNode * node = (StrNode *) g_malloc (NODE_SIZE_FOR (data));
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

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    char * ret = NULL;
    strpool_table.lookup (str, str_calc_hash (str), add_cb, ref_cb, & ret);
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

static bool remove_cb (MultiHash::Node * node_, void * state)
{
    StrNode * node = (StrNode *) node_;

    if (! __sync_bool_compare_and_swap (& node->refs, 1, 0))
        return false;

    node->magic = 0;
    g_free (node);
    return true;
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
            int status = strpool_table.lookup (node->str, node->base.hash, NULL,
             remove_cb, NULL);

            assert (status & MultiHash::Found);
            if (status & MultiHash::Removed)
                break;
        }
    }
}

static bool leak_cb (MultiHash::Node * node, void * state)
{
    fprintf (stderr, "String leaked: %s\n", ((StrNode *) node)->str);
    return false;
}

EXPORT void strpool_shutdown (void)
{
    strpool_table.iterate (leak_cb, NULL);
}

EXPORT unsigned str_hash (const char * str)
{
    if (! str)
        return 0;

    StrNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return node->base.hash;
}


EXPORT bool_t str_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return str1 == str2;
}

#endif /* ! VALGRIND_FRIENDLY */
