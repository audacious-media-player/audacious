/*
 * history.c
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
#include <stdio.h>
#include <string.h>

#include <libaudcore/hook.h>

#include "main.h"
#include "misc.h"

#define MAX_ENTRIES 30

static GQueue history = G_QUEUE_INIT;
static bool_t loaded, modified;

static void history_save (void)
{
    if (! modified)
        return;

    GList * node = history.head;
    for (int i = 0; i < MAX_ENTRIES; i ++)
    {
        if (! node)
            break;

        char name[32];
        snprintf (name, sizeof name, "entry%d", i);
        set_str ("history", name, node->data);

        node = node->next;
    }

    modified = FALSE;
}

static void history_load (void)
{
    if (loaded)
        return;

    for (int i = 0; ; i ++)
    {
        char name[32];
        snprintf (name, sizeof name, "entry%d", i);
        char * path = get_str ("history", name);

        if (! path[0])
        {
            str_unref (path);
            break;
        }

        g_queue_push_tail (& history, path);
    }

    loaded = TRUE;
    hook_associate ("config save", (HookFunction) history_save, NULL);
}

void history_cleanup (void)
{
    if (! loaded)
        return;

    hook_dissociate ("config save", (HookFunction) history_save);

    g_queue_foreach (& history, (GFunc) str_unref, NULL);
    g_queue_clear (& history);

    loaded = FALSE;
    modified = FALSE;
}

const char * history_get (int entry)
{
    history_load ();
    return g_queue_peek_nth (& history, entry);
}

void history_add (const char * path)
{
    history_load ();

    GList * next;
    for (GList * node = history.head; node; node = next)
    {
        next = node->next;
        if (! strcmp (node->data, path))
        {
            str_unref (node->data);
            g_queue_delete_link (& history, node);
        }
    }

    g_queue_push_head (& history, str_get (path));
    modified = TRUE;
}
