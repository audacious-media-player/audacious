/*
 * eventqueue.c
 * Copyright 2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <pthread.h>
#include <string.h>

#include "eventqueue.h"
#include "hook.h"

typedef struct {
    char * name;
    void * data;
    bool free_data;
    int source;
} Event;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GList * events;

static bool event_execute (Event * event)
{
    pthread_mutex_lock (& mutex);

    g_source_remove (event->source);
    events = g_list_remove (events, event);

    pthread_mutex_unlock (& mutex);

    hook_call (event->name, event->data);

    g_free (event->name);
    if (event->free_data)
        g_free (event->data);

    g_slice_free (Event, event);
    return FALSE;
}

void event_queue_full (int time, const char * name, void * data, bool free_data)
{
    Event * event = g_slice_new (Event);
    event->name = g_strdup (name);
    event->data = data;
    event->free_data = free_data;

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
            if (event->free_data)
                g_free (event->data);

            g_slice_free (Event, event);
        }

        node = next;
    }

    pthread_mutex_unlock (& mutex);
}
