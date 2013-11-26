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

#include "core.h"
#include "hook.h"

typedef struct {
    HookFunction func;
    void * user;
    int lock_count;
    bool_t remove_flag;
} HookItem;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GHashTable * hooks; /* of (GQueue of (HookItem *) *)  */

EXPORT void hook_associate (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        hooks = g_hash_table_new_full (g_str_hash, g_str_equal,
         (GDestroyNotify) str_unref, (GDestroyNotify) g_queue_free);

    GQueue * list = g_hash_table_lookup (hooks, name);

    if (! list)
        g_hash_table_insert (hooks, str_get (name), list = g_queue_new ());

    HookItem * item = g_slice_new (HookItem);
    item->func = func;
    item->user = user;
    item->lock_count = 0;
    item->remove_flag = FALSE;

    g_queue_push_tail (list, item);

    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_dissociate_full (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        goto DONE;

    GQueue * list = g_hash_table_lookup (hooks, name);

    if (! list)
        goto DONE;

    for (GList * node = list->head; node;)
    {
        HookItem * item = node->data;
        GList * next = node->next;

        if (item->func == func && (! user || item->user == user))
        {
            if (item->lock_count)
                item->remove_flag = TRUE;
            else
            {
                g_queue_delete_link (list, node);
                g_slice_free (HookItem, item);
            }
        }

        node = next;
    }

    if (! list->head)
        g_hash_table_remove (hooks, name);

DONE:
    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_call (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    if (! hooks)
        goto DONE;

    GQueue * list = g_hash_table_lookup (hooks, name);

    if (! list)
        goto DONE;

    for (GList * node = list->head; node;)
    {
        HookItem * item = node->data;

        if (! item->remove_flag)
        {
            item->lock_count ++;
            pthread_mutex_unlock (& mutex);

            item->func (data, item->user);

            pthread_mutex_lock (& mutex);
            item->lock_count --;
        }

        GList * next = node->next;

        if (item->remove_flag && ! item->lock_count)
        {
            g_queue_delete_link (list, node);
            g_slice_free (HookItem, item);
        }

        node = next;
    }

    if (! list->head)
        g_hash_table_remove (hooks, name);

DONE:
    pthread_mutex_unlock (& mutex);
}
