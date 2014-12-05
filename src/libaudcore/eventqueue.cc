/*
 * eventqueue.cc
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

#include <pthread.h>
#include <string.h>

#include "internal.h"
#include "list.h"
#include "mainloop.h"
#include "objects.h"

struct Event : public ListNode
{
    String name;
    void * data;
    void (* destroy) (void *);

    Event (const char * name, void * data, EventDestroyFunc destroy) :
        name (name),
        data (data),
        destroy (destroy) {}

    ~Event ()
    {
        if (destroy)
            destroy (data);
    }
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static List<Event> events;
static QueuedFunc queued_events;

static void events_execute (void *)
{
    pthread_mutex_lock (& mutex);

    Event * event;
    while ((event = events.head ()))
    {
        events.remove (event);

        pthread_mutex_unlock (& mutex);

        hook_call (event->name, event->data);
        delete event;

        pthread_mutex_lock (& mutex);
    }

    pthread_mutex_unlock (& mutex);
}

EXPORT void event_queue (const char * name, void * data, EventDestroyFunc destroy)
{
    pthread_mutex_lock (& mutex);

    if (! events.head ())
        queued_events.queue (events_execute, nullptr);

    events.append (new Event (name, data, destroy));

    pthread_mutex_unlock (& mutex);
}

EXPORT void event_queue_cancel (const char * name, void * data)
{
    pthread_mutex_lock (& mutex);

    Event * event = events.head ();
    while (event)
    {
        Event * next = events.next (event);

        if (! strcmp (event->name, name) && (! data || event->data == data))
        {
            events.remove (event);
            delete event;
        }

        event = next;
    }

    pthread_mutex_unlock (& mutex);
}

void event_queue_cancel_all ()
{
    pthread_mutex_lock (& mutex);
    events.clear ();
    pthread_mutex_unlock (& mutex);
}
