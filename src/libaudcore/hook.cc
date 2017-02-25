/*
 * hook.c
 * Copyright 2011-2014 John Lindgren
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

#include "hook.h"

#include <pthread.h>

#include "index.h"
#include "internal.h"
#include "multihash.h"
#include "objects.h"
#include "runtime.h"

struct HookItem {
    HookFunction func;
    void * user;
};

struct HookList
{
    Index<HookItem> items;
    int use_count;

    void compact ()
    {
        auto is_empty = [] (const HookItem & item)
            { return ! item.func; };

        items.remove_if (is_empty);
    }
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static SimpleHash<String, HookList> hooks;

EXPORT void hook_associate (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        list = hooks.add (key, HookList ());

    list->items.append (func, user);

    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_dissociate (const char * name, HookFunction func, void * user)
{
    pthread_mutex_lock (& mutex);

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        goto DONE;

    for (HookItem & item : list->items)
    {
        if (item.func == func && (! user || item.user == user))
            item.func = nullptr;
    }

    if (! list->use_count)
    {
        list->compact ();
        if (! list->items.len ())
            hooks.remove (key);
    }

DONE:
    pthread_mutex_unlock (& mutex);
}

EXPORT void hook_call (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        goto DONE;

    list->use_count ++;

    /* note: the list may grow (but not shrink) during the hook call */
    for (int i = 0; i < list->items.len (); i ++)
    {
        /* copy locally to prevent race condition */
        HookItem item = list->items[i];

        if (item.func)
        {
            pthread_mutex_unlock (& mutex);
            item.func (data, item.user);
            pthread_mutex_lock (& mutex);
        }
    }

    list->use_count --;

    if (! list->use_count)
    {
        list->compact ();
        if (! list->items.len ())
            hooks.remove (key);
    }

DONE:
    pthread_mutex_unlock (& mutex);
}

void hook_cleanup ()
{
    pthread_mutex_lock (& mutex);

    hooks.iterate ([] (const String & name, HookList & list) {
        AUDWARN ("Hook not disconnected: %s (%d)\n", (const char *) name, list.items.len ());
    });

    hooks.clear ();

    pthread_mutex_unlock (& mutex);
}
