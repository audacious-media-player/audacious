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

#include <string.h>

#include "internal.h"
#include "list.h"
#include "mainloop.h"
#include "objects.h"
#include "threads.h"

struct Event : public ListNode
{
    String name;
    void * data;
    void (*destroy)(void *);

    Event(const char * name, void * data, EventDestroyFunc destroy)
        : name(name), data(data), destroy(destroy)
    {
    }

    ~Event()
    {
        if (destroy)
            destroy(data);
    }
};

static aud::mutex mutex;
static bool paused;
static List<Event> events;
static QueuedFunc queued_events;

static void events_execute(void *)
{
    auto mh = mutex.take();

    Event * event;
    while (!paused && (event = events.head()))
    {
        events.remove(event);

        mh.unlock();

        hook_call(event->name, event->data);
        delete event;

        mh.lock();
    }
}

EXPORT void event_queue(const char * name, void * data,
                        EventDestroyFunc destroy)
{
    auto mh = mutex.take();

    if (!paused && !events.head())
        queued_events.queue(events_execute, nullptr);

    events.append(new Event(name, data, destroy));
}

EXPORT void event_queue_cancel(const char * name, void * data)
{
    auto mh = mutex.take();

    Event * event = events.head();
    while (event)
    {
        Event * next = events.next(event);

        if (!strcmp(event->name, name) && (!data || event->data == data))
        {
            events.remove(event);
            delete event;
        }

        event = next;
    }
}

// this is only for use by the playlist, to ensure that queued playlist
// updates are processed before generic events
void event_queue_pause()
{
    auto mh = mutex.take();
    if (!paused)
        queued_events.stop();

    paused = true;
}

void event_queue_unpause()
{
    auto mh = mutex.take();
    if (paused && events.head())
        queued_events.queue(events_execute, nullptr);

    paused = false;
}

void event_queue_cancel_all()
{
    auto mh = mutex.take();
    events.clear();
}
