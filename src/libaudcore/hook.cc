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

#include "index.h"
#include "internal.h"
#include "multihash.h"
#include "objects.h"
#include "runtime.h"
#include "threads.h"

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

static aud::mutex mutex;
static SimpleHash<String, HookList> hooks;

EXPORT void hook_associate (const char * name, HookFunction func, void * user)
{
    auto mh = mutex.take ();

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        list = hooks.add (key, HookList ());

    list->items.append (func, user);
}

EXPORT void hook_dissociate (const char * name, HookFunction func, void * user)
{
    auto mh = mutex.take ();

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        return;

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
}

EXPORT void hook_call (const char * name, void * data)
{
    auto mh = mutex.take ();

    String key (name);
    HookList * list = hooks.lookup (key);
    if (! list)
        return;

    list->use_count ++;

    /* note: the list may grow (but not shrink) during the hook call */
    for (int i = 0; i < list->items.len (); i ++)
    {
        /* copy locally to prevent race condition */
        HookItem item = list->items[i];

        if (item.func)
        {
            mh.unlock ();
            item.func (data, item.user);
            mh.lock ();
        }
    }

    list->use_count --;

    if (! list->use_count)
    {
        list->compact ();
        if (! list->items.len ())
            hooks.remove (key);
    }
}

void hook_cleanup ()
{
    auto mh = mutex.take ();

    hooks.iterate ([] (const String & name, HookList & list) {
        AUDWARN ("Hook not disconnected: %s (%d)\n", (const char *) name, list.items.len ());
    });

    hooks.clear ();
}
