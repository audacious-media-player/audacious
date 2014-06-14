/*
 * eventqueue.c
 * Copyright 2011-2014 John Lindgren, Michał Lipski
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

#include <glib.h>
#include <pthread.h>
#include <string.h>

#include "mainloop.h"
#include "objects.h"

struct Event {
    String name;
    void * data;
    void (* destroy) (void *);
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GList * events;
static QueuedFunc queued_events;

static void events_execute (void * unused)
{
    while (1)
    {
        pthread_mutex_lock (& mutex);

        Event * event = (Event *) g_list_nth_data (events, 0);
        if (! event)
        {
            pthread_mutex_unlock (& mutex);
            return;
        }

        events = g_list_remove (events, event);

        pthread_mutex_unlock (& mutex);

        hook_call (event->name, event->data);

        if (event->destroy)
            event->destroy (event->data);

        delete event;
    }
}

EXPORT void event_queue_full (const char * name, void * data, void (* destroy) (void *))
{
    Event * event = new Event ();

    event->name = String (name);
    event->data = data;
    event->destroy = destroy;

    pthread_mutex_lock (& mutex);

    if (! events)
        queued_events.queue (events_execute, NULL);

    events = g_list_prepend (events, event);

    pthread_mutex_unlock (& mutex);
}

EXPORT void event_queue_cancel (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    GList * node = events;
    while (node)
    {
        Event * event = (Event *) node->data;
        GList * next = node->next;

        if (! strcmp (event->name, name) && (! data || event->data == data))
        {
            events = g_list_delete_link (events, node);

            if (event->destroy)
                event->destroy (event->data);

            delete event;
        }

        node = next;
    }

    pthread_mutex_unlock (& mutex);
}

EXPORT void event_queue_cancel_all (void)
{
    pthread_mutex_lock (& mutex);

    GList * node = events;
    while (node)
    {
        Event * event = (Event *) node->data;
        GList * next = node->next;

        events = g_list_delete_link (events, node);

        if (event->destroy)
            event->destroy (event->data);

        delete event;

        node = next;
    }

    pthread_mutex_unlock (& mutex);
}
