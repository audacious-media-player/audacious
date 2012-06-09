/*
 * vis_runner.c
 * Copyright 2009-2011 John Lindgren
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
#include <stdint.h>
#include <string.h>

#include "output.h"
#include "vis_runner.h"
#include "visualization.h"

#define INTERVAL 30 /* milliseconds */

typedef struct {
    int time;
    float * data;
    int channels;
} VisNode;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool_t enabled = FALSE;
static bool_t playing = FALSE, paused = FALSE, active = FALSE;
static VisNode * current_node = NULL;
static int current_frames;
static GQueue vis_list = G_QUEUE_INIT;
static int send_source = 0, clear_source = 0;

static void vis_node_free (VisNode * node)
{
    g_free (node->data);
    g_free (node);
}

static bool_t send_audio (void * unused)
{
    /* call before locking mutex to avoid deadlock */
    int outputted = output_get_raw_time ();

    pthread_mutex_lock (& mutex);

    if (! send_source)
    {
        pthread_mutex_unlock (& mutex);
        return FALSE;
    }

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

    pthread_mutex_unlock (& mutex);

    if (! vis_node)
        return TRUE;

    vis_send_audio (vis_node->data, vis_node->channels);

    vis_node_free (vis_node);
    return TRUE;
}

static bool_t send_clear (void * unused)
{
    pthread_mutex_lock (& mutex);
    clear_source = 0;
    pthread_mutex_unlock (& mutex);

    vis_send_clear ();

    return FALSE;
}

static void flush (void)
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

void vis_runner_flush (void)
{
    pthread_mutex_lock (& mutex);
    flush ();
    pthread_mutex_unlock (& mutex);
}

static void start_stop (bool_t new_playing, bool_t new_paused)
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
        flush ();
    else if (! paused)
        send_source = g_timeout_add (INTERVAL, send_audio, NULL);
}

void vis_runner_start_stop (bool_t new_playing, bool_t new_paused)
{
    pthread_mutex_lock (& mutex);
    start_stop (new_playing, new_paused);
    pthread_mutex_unlock (& mutex);
}

void vis_runner_pass_audio (int time, float * data, int samples, int
 channels, int rate)
{
    pthread_mutex_lock (& mutex);

    if (! active)
        goto UNLOCK;

    /* We can build a single node from multiple calls; we can also build
     * multiple nodes from the same call.  If current_node is present, it was
     * partly built in the last call and needs to be finished. */

    if (current_node && current_node->channels != channels)
    {
        vis_node_free (current_node);
        current_node = NULL;
    }

    int at = 0;

    while (1)
    {
        if (! current_node)
        {
            int node_time = time;
            VisNode * last;

            /* There is no partly-built node, so start a new one.  Normally
             * there will be nodes in the queue already; if so, we want to copy
             * audio data from the signal starting at 30 milliseconds after the
             * beginning of the most recent node.  If there are no nodes in the
             * queue, we are at the beginning of the song or had an underrun,
             * and we want to copy the earliest audio data we have. */

            if ((last = g_queue_peek_tail (& vis_list)))
                node_time = last->time + INTERVAL;

            at = channels * (int) ((int64_t) (node_time - time) * rate / 1000);

            if (at < 0)
                at = 0;
            if (at >= samples)
                break;

            current_node = g_malloc (sizeof (VisNode));
            current_node->time = node_time;
            current_node->data = g_malloc (sizeof (float) * channels * 512);
            current_node->channels = channels;
            current_frames = 0;
        }

        /* Copy as much data as we can, limited by how much we have and how much
         * space is left in the node.  If we cannot fill the node, we return and
         * wait for more data to be passed in the next call.  If we do fill the
         * node, we loop and start building a new one. */

        int copy = MIN (samples - at, channels * (512 - current_frames));
        memcpy (current_node->data + channels * current_frames, data + at, sizeof (float) * copy);
        current_frames += copy / channels;

        if (current_frames < 512)
            break;

        g_queue_push_tail (& vis_list, current_node);
        current_node = NULL;
    }

UNLOCK:
    pthread_mutex_unlock (& mutex);
}

void vis_runner_enable (bool_t enable)
{
    pthread_mutex_lock (& mutex);
    enabled = enable;
    start_stop (playing, paused);
    pthread_mutex_unlock (& mutex);
}
