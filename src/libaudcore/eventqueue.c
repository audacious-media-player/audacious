/*
 * eventqueue.c
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

#include "core.h"
#include "hook.h"

typedef struct {
    char * name;
    void * data;
    void (* destroy) (void *);
    int source;
} Event;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GList * events;

static bool_t event_execute (Event * event)
{
    pthread_mutex_lock (& mutex);

    g_source_remove (event->source);
    events = g_list_remove (events, event);

    pthread_mutex_unlock (& mutex);

    hook_call (event->name, event->data);

    g_free (event->name);
    if (event->destroy)
        event->destroy (event->data);

    g_slice_free (Event, event);
    return FALSE;
}

void event_queue_full (int time, const char * name, void * data, void (* destroy) (void *))
{
    Event * event = g_slice_new (Event);
    event->name = g_strdup (name);
    event->data = data;
    event->destroy = destroy;

    pthread_mutex_lock (& mutex);

    event->source = g_timeout_add (time, (GSourceFunc) event_execute, event);
    events = g_list_prepend (events, event);

    pthread_mutex_unlock (& mutex);
}

void event_queue_cancel (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    GList * node = events;
    while (node)
    {
        Event * event = node->data;
        GList * next = node->next;

        if (! strcmp (event->name, name) && (! data || event->data == data))
        {
            g_source_remove (event->source);
            events = g_list_delete_link (events, node);

            g_free (event->name);
            if (event->destroy)
                event->destroy (event->data);

            g_slice_free (Event, event);
        }

        node = next;
    }

    pthread_mutex_unlock (& mutex);
}
