/*
 * vis_runner.c
 * Copyright 2009-2011 John Lindgren
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

#include "output.h"
#include "vis_runner.h"
#include "visualization.h"

#define INTERVAL 30 /* milliseconds */

typedef struct {
    gint time;
    gfloat * data;
    gint channels;
} VisNode;

G_LOCK_DEFINE_STATIC (mutex);
static gboolean enabled = FALSE;
static gboolean playing = FALSE, paused = FALSE, active = FALSE;
static VisNode * current_node = NULL;
static gint current_frames;
static GQueue vis_list = G_QUEUE_INIT;
static gint send_source = 0, clear_source = 0;

static void vis_node_free (VisNode * node)
{
    g_free (node->data);
    g_free (node);
}

static gboolean send_audio (void * unused)
{
    G_LOCK (mutex);

    if (! send_source)
    {
        G_UNLOCK (mutex);
        return FALSE;
    }

    gint outputted = get_raw_output_time ();

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

        if (vis_node)
            vis_node_free (vis_node);

        vis_node = g_queue_pop_head (& vis_list);
    }

    G_UNLOCK (mutex);

    if (! vis_node)
        return TRUE;

    vis_send_audio (vis_node->data, vis_node->channels);

    vis_node_free (vis_node);
    return TRUE;
}

static gboolean send_clear (void * unused)
{
    G_LOCK (mutex);
    clear_source = 0;
    G_UNLOCK (mutex);

    vis_send_clear ();

    return FALSE;
}

static gboolean locked = FALSE;

void vis_runner_lock (void)
{
    G_LOCK (mutex);
    locked = TRUE;
}

void vis_runner_unlock (void)
{
    locked = FALSE;
    G_UNLOCK (mutex);
}

gboolean vis_runner_locked (void)
{
    return locked;
}

void vis_runner_flush (void)
{
    if (current_node)
    {
        vis_node_free (current_node);
        current_node = NULL;
    }

    g_queue_foreach (& vis_list, (GFunc) vis_node_free, NULL);
    g_queue_clear (& vis_list);

    if (! clear_source)
        clear_source = g_timeout_add (0, send_clear, NULL);
}

void vis_runner_start_stop (gboolean new_playing, gboolean new_paused)
{
    playing = new_playing;
    paused = new_paused;
    active = playing && enabled;

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
        vis_runner_flush ();
    else if (! paused)
        send_source = g_timeout_add (INTERVAL, send_audio, NULL);
}

void vis_runner_pass_audio (gint time, gfloat * data, gint samples, gint
 channels, gint rate)
{
    if (! active)
        return;

    /* We can build a single node from multiple calls; we can also build
     * multiple nodes from the same call.  If current_node is present, it was
     * partly built in the last call and needs to be finished. */

    if (current_node && current_node->channels != channels)
    {
        vis_node_free (current_node);
        current_node = NULL;
    }

    gint at = 0;

    while (1)
    {
        if (! current_node)
        {
            gint node_time = time;
            VisNode * last;

            /* There is no partly-built node, so start a new one.  Normally
             * there will be nodes in the queue already; if so, we want to copy
             * audio data from the signal starting at 30 milliseconds after the
             * beginning of the most recent node.  If there are no nodes in the
             * queue, we are at the beginning of the song or had an underrun,
             * and we want to copy the earliest audio data we have. */

            if ((last = g_queue_peek_tail (& vis_list)))
                node_time = last->time + INTERVAL;

            at = channels * (gint) ((gint64) (node_time - time) * rate / 1000);

            if (at < 0)
                at = 0;
            if (at >= samples)
                break;

            current_node = g_malloc (sizeof (VisNode));
            current_node->time = node_time;
            current_node->data = g_malloc (sizeof (gfloat) * channels * 512);
            current_node->channels = channels;
            current_frames = 0;
        }

        /* Copy as much data as we can, limited by how much we have and how much
         * space is left in the node.  If we cannot fill the node, we return and
         * wait for more data to be passed in the next call.  If we do fill the
         * node, we loop and start building a new one. */

        gint copy = MIN (samples - at, channels * (512 - current_frames));
        memcpy (current_node->data + channels * current_frames, data + at, sizeof (gfloat) * copy);
        current_frames += copy / channels;

        if (current_frames < 512)
            break;

        g_queue_push_tail (& vis_list, current_node);
        current_node = NULL;
    }
}

static void time_offset_cb (VisNode * vis_node, void * offset)
{
    vis_node->time += GPOINTER_TO_INT (offset);
}

void vis_runner_time_offset (gint offset)
{
    if (current_node)
        current_node->time += offset;

    g_queue_foreach (& vis_list, (GFunc) time_offset_cb, GINT_TO_POINTER (offset));
}

void vis_runner_enable (gboolean enable)
{
    G_LOCK (mutex);
    enabled = enable;
    vis_runner_start_stop (playing, paused);
    G_UNLOCK (mutex);
}
