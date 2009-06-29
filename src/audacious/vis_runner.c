// vis_runner.c
// Copyright 2009 John Lindgren

// This file is part of Audacious.

// Audacious is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, version 3 of the License.

// Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with
// Audacious. If not, see <http://www.gnu.org/licenses/>.

// The Audacious team does not consider modular code linking to Audacious or
// using our public API to be a derived work.

#include <stdint.h>

#include <glib.h>

#include "hook.h"
#include "input.h"
#include "playback.h"
#include "plugin.h"

#define INTERVAL 30 // milliseconds

static GMutex * mutex;
static char active = 0;
static int source = 0;
static GList * hooks = 0, * vis_list = 0;

static gboolean send_audio (void * user_data)
{
    int outputted = playback_get_time ();
    VisNode * vis_node, * next;

    vis_node = 0;

    g_mutex_lock (mutex);

    while (vis_list)
    {
        next = vis_list->data;

        if (next->time > outputted)
            break;

        free (vis_node);
        vis_node = next;
        vis_list = g_list_delete_link (vis_list, vis_list);
    }

    g_mutex_unlock (mutex);

    if (vis_node)
    {
        GList * node;

        for (node = hooks; node; node = node->next)
        {
            HookItem * item = node->data;

            item->func (vis_node, item->user_data);
        }
    }

    free (vis_node);
    return 1;
}

static void start_stop (void * hook_data, void * user_data)
{
    if (hooks && playback_get_playing () && ! playback_get_paused ())
    {
        active = 1;

        if (! source)
            source = g_timeout_add (INTERVAL, send_audio, 0);
    }
    else
    {
        if (source)
        {
            g_source_remove (source);
            source = 0;
        }

        active = 0;
    }
}

void vis_runner_init (void)
{
    mutex = g_mutex_new ();
    hook_associate ("playback begin", start_stop, 0);
    hook_associate ("playback pause", start_stop, 0);
    hook_associate ("playback unpause", start_stop, 0);
    hook_associate ("playback stop", start_stop, 0);
}

void vis_runner_pass_audio (int time, float * data, int samples, int channels)
{
    VisNode * vis_node;
    int channel;

    if (! active)
        return;

    vis_node = malloc (sizeof (VisNode));
    vis_node->time = time;
    vis_node->nch = MIN (channels, 2);
    vis_node->length = MIN (samples / channels, 512);

    for (channel = 0; channel < vis_node->nch; channel ++)
    {
        float * from = data + channel;
        float * end = from + channels * vis_node->length;
        int16_t * to = vis_node->data[channel];

        while (from < end)
        {
            register float temp = * from;
            * to ++ = CLAMP (temp, -1, 1) * 32767;
            from += channels;
        }
    }

    g_mutex_lock (mutex);

    if (active)
        vis_list = g_list_append (vis_list, vis_node);
    else
        free (vis_node);

    g_mutex_unlock (mutex);
}

void vis_runner_flush (void)
{
    g_mutex_lock (mutex);

    while (vis_list)
    {
        free (vis_list->data);
        vis_list = g_list_delete_link (vis_list, vis_list);
    }

    g_mutex_unlock (mutex);
}

void vis_runner_add_hook (HookFunction func, void * user_data)
{
    HookItem * item = malloc (sizeof (HookItem));

    item->func = func;
    item->user_data = user_data;
    hooks = g_list_prepend (hooks, item);
    start_stop (0, 0);
}

void vis_runner_remove_hook (HookFunction func)
{
    GList * node;
    HookItem * item;

    for (node = hooks; node; node = node->next)
    {
        item = node->data;

        if (item->func == func)
            goto FOUND;
    }

    return;

  FOUND:
    hooks = g_list_delete_link (hooks, node);
    free (item);
    start_stop (0, 0);
}
