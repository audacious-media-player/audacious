/*
 * vis_runner.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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

#include <glib.h>

#include "hook.h"
#include "input.h"
#include "playback.h"
#include "plugin.h"
#include "vis_runner.h"

#define INTERVAL 30 /* milliseconds */

static GMutex * mutex;
static gboolean active;
static GList * vis_list, * hooks;
static gint source;

static gboolean send_audio (gpointer user_data)
{
    gint outputted = playback_get_time ();
    VisNode * vis_node, * next;

    vis_node = NULL;

    g_mutex_lock (mutex);

    while (vis_list != NULL)
    {
        next = vis_list->data;

        if (next->time > outputted)
            break;

        g_free (vis_node);
        vis_node = next;
        vis_list = g_list_delete_link (vis_list, vis_list);
    }

    g_mutex_unlock (mutex);

    if (vis_node != NULL)
    {
        GList * node;

        for (node = hooks; node != NULL; node = node->next)
        {
            HookItem * item = node->data;

            item->func (vis_node, item->user_data);
        }
    }

    g_free (vis_node);
    return 1;
}

static void start_stop (gpointer hook_data, gpointer user_data)
{
    if (hooks != NULL && playback_get_playing () && ! playback_get_paused ())
    {
        active = TRUE;

        if (! source)
            source = g_timeout_add (INTERVAL, send_audio, NULL);
    }
    else
    {
        if (source)
        {
            g_source_remove (source);
            source = 0;
        }

        active = FALSE;
        vis_runner_flush ();
    }
}

void vis_runner_init (void)
{
    mutex = g_mutex_new ();
    active = FALSE;
    hooks = NULL;
    vis_list = NULL;
    source = 0;

    hook_associate ("playback begin", start_stop, NULL);
    hook_associate ("playback pause", start_stop, NULL);
    hook_associate ("playback unpause", start_stop, NULL);
    hook_associate ("playback stop", start_stop, NULL);
}

void vis_runner_pass_audio (gint time, gfloat * data, gint samples, gint
 channels)
{
    VisNode * vis_node;
    gint channel;

    if (! active)
        return;

    vis_node = g_malloc (sizeof (VisNode));
    vis_node->time = time;
    vis_node->nch = MIN (channels, 2);
    vis_node->length = MIN (samples / channels, 512);

    for (channel = 0; channel < vis_node->nch; channel ++)
    {
        gfloat * from = data + channel;
        gfloat * end = from + channels * vis_node->length;
        gint16 * to = vis_node->data[channel];

        while (from < end)
        {
            register gfloat temp = * from;
            * to ++ = CLAMP (temp, -1, 1) * 32767;
            from += channels;
        }
    }

    g_mutex_lock (mutex);

    if (active)
        vis_list = g_list_append (vis_list, vis_node);
    else
        g_free (vis_node);

    g_mutex_unlock (mutex);
}

void vis_runner_flush (void)
{
    g_mutex_lock (mutex);

    while (vis_list != NULL)
    {
        g_free (vis_list->data);
        vis_list = g_list_delete_link (vis_list, vis_list);
    }

    g_mutex_unlock (mutex);
}

void vis_runner_add_hook (HookFunction func, gpointer user_data)
{
    HookItem * item = g_malloc (sizeof (HookItem));

    item->func = func;
    item->user_data = user_data;
    hooks = g_list_prepend (hooks, item);
    start_stop (NULL, NULL);
}

void vis_runner_remove_hook (HookFunction func)
{
    GList * node;
    HookItem * item;

    for (node = hooks; node != NULL; node = node->next)
    {
        item = node->data;

        if (item->func == func)
            goto FOUND;
    }

    return;

  FOUND:
    hooks = g_list_delete_link (hooks, node);
    g_free (item);
    start_stop (NULL, NULL);
}
