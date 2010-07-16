/*
 * vis_runner.c
 * Copyright 2009-2010 John Lindgren
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
#include <libaudcore/hook.h>

#include "misc.h"
#include "output.h"
#include "vis_runner.h"

#define INTERVAL 30 /* milliseconds */

typedef struct {
    VisHookFunc func;
    void * user;
} VisHookItem;

G_LOCK_DEFINE_STATIC (mutex);
static gboolean playing = FALSE, paused = FALSE, active = FALSE;
static GList * hooks = NULL;
static VisNode * current_node = NULL;
static GQueue vis_list = G_QUEUE_INIT;
static gint send_source = 0, clear_source = 0;

static gboolean send_audio (void * unused)
{
    G_LOCK (mutex);

    if (! send_source)
    {
        G_UNLOCK (mutex);
        return FALSE;
    }

    /* We need raw time, not changed for effects and gapless playback. */
    gint outputted = current_output_plugin->output_time ();

    VisNode * vis_node = NULL;
    VisNode * next;

    while ((next = g_queue_peek_head (& vis_list)))
    {
        /* If we are considering a node, stop searching and use it if it is the
         * most recent (that is, the next one is in the future).  Otherwise,
         * consider the next node if it is not in the future by more than the
         * length of an interval. */
        if (next->time > outputted + (vis_node ? 0 : INTERVAL))
            break;

        g_free (vis_node);
        vis_node = g_queue_pop_head (& vis_list);
    }

    G_UNLOCK (mutex);

    if (! vis_node)
        return TRUE;

    for (GList * node = hooks; node; node = node->next)
    {
        VisHookItem * item = node->data;
        item->func (vis_node, item->user);
    }

    g_free (vis_node);
    return TRUE;
}

static gboolean send_clear (void * unused)
{
    G_LOCK (mutex);
    clear_source = 0;
    G_UNLOCK (mutex);

    hook_call ("visualization clear", NULL);
    return FALSE;
}

static void flush_locked (void)
{
    g_free (current_node);
    current_node = NULL;
    g_queue_foreach (& vis_list, (GFunc) g_free, NULL);
    g_queue_clear (& vis_list);

    clear_source = g_timeout_add (0, send_clear, NULL);
}

void vis_runner_start_stop (gboolean new_playing, gboolean new_paused)
{
    G_LOCK (mutex);

    playing = new_playing;
    paused = new_paused;
    active = playing && hooks;

    if (send_source)
    {
        g_source_remove (send_source);
        send_source = 0;
    }

    if (clear_source)
    {
        g_source_remove (clear_source);
        clear_source = 0;
    }

    if (! active)
        flush_locked ();
    else if (! paused)
        send_source = g_timeout_add (INTERVAL, send_audio, NULL);

    G_UNLOCK (mutex);
}

void vis_runner_pass_audio (gint time, gfloat * data, gint samples, gint
 channels, gint rate)
{
    G_LOCK (mutex);

    if (! active)
        goto UNLOCK;

    if (current_node && current_node->nch != MIN (channels, 2))
    {
        g_free (current_node);
        current_node = NULL;
    }

    gint at = 0;

    while (1)
    {
        if (! current_node)
        {
            gint node_time = time;
            VisNode * last;

            if ((last = g_queue_peek_tail (& vis_list)))
                node_time = last->time + INTERVAL;

            at = channels * (gint) ((gint64) (node_time - time) * rate / 1000);

            if (at < 0)
                at = 0;
            if (at >= samples)
                break;

            current_node = g_malloc (sizeof (VisNode));
            current_node->time = node_time;
            current_node->nch = MIN (channels, 2);
            current_node->length = 0;
        }

        gint copy = MIN (samples - at, channels * (512 - current_node->length));

        for (gint channel = 0; channel < current_node->nch; channel ++)
        {
            gfloat * from = data + at + channel;
            gfloat * end = from + copy;
            gint16 * to = current_node->data[channel] + current_node->length;

            while (from < end)
            {
                register gfloat temp = * from;
                * to ++ = CLAMP (temp, -1, 1) * 32767;
                from += channels;
            }
        }

        current_node->length += copy / channels;

        if (current_node->length < 512)
            break;

        g_queue_push_tail (& vis_list, current_node);
        current_node = NULL;
    }

UNLOCK:
    G_UNLOCK (mutex);
}

static void time_offset_cb (VisNode * vis_node, void * offset)
{
    vis_node->time += GPOINTER_TO_INT (offset);
}

void vis_runner_time_offset (gint offset)
{
    G_LOCK (mutex);

    if (current_node)
        current_node->time += offset;

    g_queue_foreach (& vis_list, (GFunc) time_offset_cb, GINT_TO_POINTER (offset));

    G_UNLOCK (mutex);
}

void vis_runner_flush (void)
{
    G_LOCK (mutex);
    flush_locked ();
    G_UNLOCK (mutex);
}

void vis_runner_add_hook (VisHookFunc func, void * user)
{
    G_LOCK (mutex);

    VisHookItem * item = g_malloc (sizeof (VisHookItem));
    item->func = func;
    item->user = user;
    hooks = g_list_prepend (hooks, item);

    G_UNLOCK (mutex);
    vis_runner_start_stop (playing, paused);
}

void vis_runner_remove_hook (VisHookFunc func)
{
    G_LOCK (mutex);

    for (GList * node = hooks; node; node = node->next)
    {
        if (((VisHookItem *) node->data)->func == func)
        {
            g_free (node->data);
            hooks = g_list_delete_link (hooks, node);
            break;
        }
    }

    G_UNLOCK (mutex);
    vis_runner_start_stop (playing, paused);
}
