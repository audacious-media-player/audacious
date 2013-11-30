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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "core.h"

#define NUM_TABLES     16   /* must be a power of two */
#define TABLE_SHIFT     4   /* log (base 2) of NUM_TABLES */
#define INITIAL_SIZE  256   /* must be a power of two */

typedef char TinyLock;

struct _HashNode {
    struct _HashNode * next;
    unsigned hash, refs;
    char magic;
    char str[];
};

typedef struct _HashNode HashNode;

#define NODE_SIZE_FOR(s) (offsetof (HashNode, str) + strlen (s) + 1)
#define NODE_OF(s) ((HashNode *) ((s) - offsetof (HashNode, str)))

typedef struct {
    TinyLock lock;
    HashNode * * buckets;
    unsigned size, used;
} HashTable;

static HashTable tables[NUM_TABLES];

static void tiny_lock (TinyLock * lock)
{
    while (__sync_lock_test_and_set (lock, 1))
        sched_yield ();
}

static void tiny_unlock (TinyLock * lock)
{
    __sync_lock_release (lock);
}

static HashNode * find_in_list (HashNode * list, unsigned hash, const char * str)
{
    while (list && (list->hash != hash || strcmp (list->str, str)))
        list = list->next;

    return list;
}

static void add_to_list (HashNode * * list, HashNode * node)
{
    node->next = * list;
    * list = node;
}

static void remove_from_list (HashNode * * list, HashNode * node)
{
    if (* list == node)
        * list = node->next;
    else
    {
        HashNode * prev = * list;
        while (prev->next != node)
            prev = prev->next;

        prev->next = node->next;
    }
}

static void resize_table (HashTable * table, unsigned new_size)
{
    HashNode * * new_buckets = calloc (new_size, sizeof (HashNode *));

    for (int i = 0; i < table->size; i ++)
    {
        HashNode * node = table->buckets[i];

        while (node)
        {
            HashNode * next = node->next;
            unsigned new_bucket = (node->hash >> TABLE_SHIFT) & (new_size - 1);

            add_to_list (& new_buckets[new_bucket], node);
            node = next;
        }
    }

    free (table->buckets);
    table->buckets = new_buckets;
    table->size = new_size;
}

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    unsigned hash = g_str_hash (str);
    HashTable * table = & tables[hash & (NUM_TABLES - 1)];

    tiny_lock (& table->lock);

    if (! table->buckets)
    {
        table->buckets = calloc (INITIAL_SIZE, sizeof (HashNode *));
        table->size = INITIAL_SIZE;
        table->used = 0;
    }

    unsigned bucket = (hash >> TABLE_SHIFT) & (table->size - 1);
    HashNode * node = find_in_list (table->buckets[bucket], hash, str);

    if (node)
        __sync_fetch_and_add (& node->refs, 1);
    else
    {
        node = malloc (NODE_SIZE_FOR (str));
        node->hash = hash;
        node->refs = 1;
        node->magic = '@';
        strcpy (node->str, str);

        add_to_list (& table->buckets[bucket], node);

        table->used ++;

        if (table->used > table->size)
            resize_table (table, table->size << 1);
    }

    tiny_unlock (& table->lock);

    return node->str;
}

EXPORT char * str_ref (const char * str)
{
    if (! str)
        return NULL;

    HashNode * node = NODE_OF (str);
    assert (node->magic == '@');

    __sync_fetch_and_add (& node->refs, 1);

    return (char *) str;
}

EXPORT void str_unref (char * str)
{
    if (! str)
        return;

    HashNode * node = NODE_OF (str);
    assert (node->magic == '@');

restart:;
    int refs = __sync_fetch_and_add (& node->refs, 0);

    if (refs > 1)
    {
        if (! __sync_bool_compare_and_swap (& node->refs, refs, refs - 1))
            goto restart;
    }
    else
    {
        HashTable * table = & tables[node->hash & (NUM_TABLES - 1)];

        tiny_lock (& table->lock);

        if (! __sync_bool_compare_and_swap (& node->refs, 1, 0))
        {
            tiny_unlock (& table->lock);
            goto restart;
        }

        unsigned bucket = (node->hash >> TABLE_SHIFT) & (table->size - 1);
        remove_from_list (& table->buckets[bucket], node);

        node->magic = 0;
        free (node);

        table->used --;

        if (table->used < table->size >> 2 && table->size > INITIAL_SIZE)
            resize_table (table, table->size >> 1);

        tiny_unlock (& table->lock);
    }
}

EXPORT unsigned str_hash (const char * str)
{
    if (! str)
        return 0;

    HashNode * node = NODE_OF (str);
    assert (node->magic == '@');

    return node->hash;
}


EXPORT bool_t str_equal (const char * str1, const char * str2)
{
    assert (! str1 || NODE_OF (str1)->magic == '@');
    assert (! str2 || NODE_OF (str2)->magic == '@');

    return str1 == str2;
}

EXPORT char * str_nget (const char * str, int len)
{
    if (strlen (str) <= len)
        return str_get (str);

    char buf[len + 1];
    memcpy (buf, str, len);
    buf[len] = 0;

    return str_get (buf);
}

EXPORT char * str_printf (const char * format, ...)
{
    va_list args;

    va_start (args, format);
    int len = vsnprintf (NULL, 0, format, args);
    va_end (args);

    char buf[len + 1];

    va_start (args, format);
    vsnprintf (buf, sizeof buf, format, args);
    va_end (args);

    return str_get (buf);
}

EXPORT void strpool_shutdown (void)
{
    for (int t = 0; t < NUM_TABLES; t ++)
    {
        HashTable * table = & tables[t];

        tiny_lock (& table->lock);

        if (table->buckets)
        {
            if (table->used)
            {
                for (int i = 0; i < table->size; i ++)
                    for (HashNode * node = table->buckets[i]; node; node = node->next)
                        fprintf (stderr, "String leaked: %s\n", node->str);
            }
            else
            {
                free (table->buckets);
                table->buckets = NULL;
                table->size = 0;
            }
        }

        tiny_unlock (& table->lock);
    }
}
