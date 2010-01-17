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

#include "effect.h"
#include "hook.h"
#include "input.h"
#include "output.h"
#include "vis_runner.h"

#define INTERVAL 30 /* milliseconds */
#define DEBUG 0

static GMutex * mutex;
static gboolean playing, paused, active;
static GList * vis_list, * vis_tail, * hooks;
static gint source;

static gboolean send_audio (void * unused)
{
    gint outputted;
    VisNode * vis_node, * next;

    g_mutex_lock (mutex);

    if (! source) /* stopped by another thread? */
    {
        g_mutex_unlock (mutex);
        return FALSE;
    }

    vis_node = NULL;
    outputted = new_effect_output_to_decoder_time
     (current_output_plugin->output_time ());

    while (vis_list != NULL)
    {
        next = vis_list->data;

        if ((vis_node != NULL) ? (next->time > outputted) : (next->time >
         outputted + INTERVAL))
            break;

#if DEBUG
        if (vis_node != NULL)
            printf ("DISCARD %d\n", vis_node->time);
#endif

        g_free (vis_node);
        vis_node = next;
        vis_list = g_list_delete_link (vis_list, vis_list);

        if (vis_list == NULL)
            vis_tail = NULL;
    }

    g_mutex_unlock (mutex);

    if (vis_node != NULL)
    {
        gint channel;
        GList * node;

#if DEBUG
        printf ("SEND %d/%d\n", outputted, vis_node->time);
#endif

        for (channel = 0; channel < vis_node->nch; channel ++)
            memset (vis_node->data[channel] + vis_node->length, 0, 2 * (512 -
             vis_node->length));

        vis_node->length = 512;

        for (node = hooks; node != NULL; node = node->next)
        {
            HookItem * item = node->data;

            item->func (vis_node, item->user_data);
        }
    }
#if DEBUG
    else
        printf ("MISS %d\n", outputted);
#endif

    g_free (vis_node);
    return TRUE;
}

static gboolean send_clear (void * unused)
{
    hook_call ("visualization clear", NULL);
    source = 0;
    return FALSE;
}

static void flush_locked (void)
{
    while (vis_list != NULL)
    {
        g_free (vis_list->data);
        vis_list = g_list_delete_link (vis_list, vis_list);
    }

    vis_tail = NULL;
}

void vis_runner_init (void)
{
    mutex = g_mutex_new ();
    playing = FALSE;
    paused = FALSE;
    active = FALSE;
    hooks = NULL;
    vis_list = NULL;
    vis_tail = NULL;
    source = 0;
}

void vis_runner_start_stop (gboolean new_playing, gboolean new_paused)
{
    g_mutex_lock (mutex);

    playing = new_playing;
    paused = new_paused;
    active = playing && hooks != NULL;

    if (source != 0)
    {
        g_source_remove (source);
        source = 0;
    }

    if (! active)
    {
        flush_locked ();
        source = g_timeout_add (0, send_clear, NULL);
    }
    else if (! paused)
        source = g_timeout_add (INTERVAL, send_audio, NULL);

    g_mutex_unlock (mutex);
}

void vis_runner_pass_audio (gint time, gfloat * data, gint samples, gint
 channels)
{
    VisNode * vis_node;
    gint channel;
#if DEBUG
    gint old_time = time;
#endif

    g_mutex_lock (mutex);

    if (! active)
        goto UNLOCK;

    time -= time % INTERVAL;

    if (vis_tail == NULL)
        vis_node = NULL;
    else
    {
        vis_node = vis_tail->data;

        if (vis_node->time != time || vis_node->nch != MIN (channels, 2))
            vis_node = NULL;
    }

    if (vis_node == NULL)
    {
#if DEBUG
        printf ("GET %d/%d\n", old_time, time);
#endif

        vis_node = g_malloc (sizeof (VisNode));
        vis_node->time = time;
        vis_node->nch = MIN (channels, 2);
        vis_node->length = 0;

        if (vis_tail == NULL)
        {
            vis_tail = g_list_append (NULL, vis_node);
            vis_list = vis_tail;
        }
        else
            vis_tail = g_list_append (vis_tail, vis_node)->next;
    }
#if DEBUG
    else
        printf ("COMBINE %d/%d\n", old_time, time);
#endif

    if (samples > channels * (512 - vis_node->length))
        samples = channels * (512 - vis_node->length);

    for (channel = 0; channel < vis_node->nch; channel ++)
    {
        gfloat * from = data + channel;
        gfloat * end = from + samples;
        gint16 * to = vis_node->data[channel] + vis_node->length;

        while (from < end)
        {
            register gfloat temp = * from;
            * to ++ = CLAMP (temp, -1, 1) * 32767;
            from += channels;
        }
    }

    vis_node->length += samples / channels;

UNLOCK:
    g_mutex_unlock (mutex);
}

void vis_runner_time_offset (gint offset)
{
    GList * node;

#if DEBUG
    printf ("OFFSET %d\n", offset);
#endif

    g_mutex_lock (mutex);

    for (node = vis_list; node != NULL; node = node->next)
        ((VisNode *) (node->data))->time += offset;

    g_mutex_unlock (mutex);
}

void vis_runner_flush (void)
{
#if DEBUG
    printf ("FLUSH\n");
#endif

    g_mutex_lock (mutex);
    flush_locked ();
    g_mutex_unlock (mutex);
}

void vis_runner_add_hook (HookFunction func, gpointer user_data)
{
    HookItem * item = g_malloc (sizeof (HookItem));

    item->func = func;
    item->user_data = user_data;
    hooks = g_list_prepend (hooks, item);
    vis_runner_start_stop (playing, paused);
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
    vis_runner_start_stop (playing, paused);
}
