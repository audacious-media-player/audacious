/*
 * hook.c
 * Copyright 2011 John Lindgren
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

#include <glib.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "core.h"
#include "hook.h"

typedef struct {
    HookFunction func;
    void * user;
} HookItem;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GHashTable * hooks;

/* str_unref() may be a macro */
static void str_unref_cb (void * str)
{
    str_unref (str);
}

EXPORT void hook_associate (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        hooks = g_hash_table_new_full (g_str_hash, g_str_equal, str_unref_cb, NULL);

    HookItem * item = g_slice_new (HookItem);
    item->func = func;
    item->user = user;

    GList * items = g_hash_table_lookup (hooks, name);
    items = g_list_prepend (items, item);
    g_hash_table_insert (hooks, str_get (name), items);

    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_dissociate_full (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        goto DONE;

    GList * items = g_hash_table_lookup (hooks, name);

    GList * node = items;
    while (node)
    {
        HookItem * item = node->data;
        GList * next = node->next;

        if (item->func == func && (! user || item->user == user))
        {
            items = g_list_delete_link (items, node);
            g_slice_free (HookItem, item);
        }

        node = next;
    }

    if (items)
        g_hash_table_insert (hooks, str_get (name), items);
    else
        g_hash_table_remove (hooks, name);

DONE:
    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_call (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        goto DONE;

    GList * node = g_hash_table_lookup (hooks, name);

    for (; node; node = node->next)
    {
        HookItem * item = node->data;
        item->func (data, item->user);
    }

DONE:
    pthread_mutex_unlock (& mutex);
}
