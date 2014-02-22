/*
 * vis_runner.c
 * Copyright 2009-2012 John Lindgren
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

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "output.h"
#include "vis_runner.h"
#include "visualization.h"

#define INTERVAL 30 /* milliseconds */
#define FRAMES_PER_NODE 512

struct _VisNode {
    struct _VisNode * next;
    int channels;
    int time;
    float data[];
};

typedef struct _VisNode VisNode;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool_t enabled = FALSE;
static bool_t playing = FALSE, paused = FALSE, active = FALSE;
static VisNode * current_node = NULL;
static int current_frames;
static VisNode * vis_list = NULL;
static VisNode * vis_list_tail = NULL;
static VisNode * vis_pool = NULL;
static int send_source = 0, clear_source = 0;

static VisNode * alloc_node_locked (int channels)
{
    VisNode * node;

    if (vis_pool)
    {
        node = vis_pool;
        assert (node->channels == channels);
        vis_pool = node->next;
    }
    else
    {
        node = g_malloc (offsetof (VisNode, data) + sizeof (float) * channels * FRAMES_PER_NODE);
        node->channels = channels;
    }

    node->next = NULL;
    return node;
}

static void free_node_locked (VisNode * node)
{
    node->next = vis_pool;
    vis_pool = node;
}

static void push_node_locked (VisNode * node)
{
    if (vis_list)
        vis_list_tail->next = node;
    else
        vis_list = node;

    vis_list_tail = node;
}

static VisNode * pop_node_locked (void)
{
    VisNode * node = vis_list;
    vis_list = node->next;
    node->next = NULL;

    if (vis_list_tail == node)
        vis_list_tail = NULL;

    return node;
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

    while (vis_list)
    {
        /* If we are considering a node, stop searching and use it if it is the
         * most recent (that is, the next one is in the future).  Otherwise,
         * consider the next node if it is not in the future by more than the
         * length of an interval. */
        if (vis_list->time > outputted + (vis_node ? 0 : INTERVAL))
            break;

        if (vis_node)
            free_node_locked (vis_node);

        vis_node = pop_node_locked ();
    }

    pthread_mutex_unlock (& mutex);

    if (! vis_node)
        return TRUE;

    vis_send_audio (vis_node->data, vis_node->channels);

    pthread_mutex_lock (& mutex);
    free_node_locked (vis_node);
    pthread_mutex_unlock (& mutex);

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

static void flush_locked (void)
{
    g_free (current_node);
    current_node = NULL;

    while (vis_list)
    {
        VisNode * node = vis_list;
        vis_list = node->next;
        g_free (node);
    }

    vis_list_tail = NULL;

    while (vis_pool)
    {
        VisNode * node = vis_pool;
        vis_pool = node->next;
        g_free (node);
    }

    if (! clear_source)
        clear_source = g_timeout_add (0, send_clear, NULL);
}

void vis_runner_flush (void)
{
    pthread_mutex_lock (& mutex);
    flush_locked ();
    pthread_mutex_unlock (& mutex);
}

static void start_stop_locked (bool_t new_playing, bool_t new_paused)
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
        flush_locked ();
    else if (! paused)
        send_source = g_timeout_add (INTERVAL, send_audio, NULL);
}

void vis_runner_start_stop (bool_t new_playing, bool_t new_paused)
{
    pthread_mutex_lock (& mutex);
    start_stop_locked (new_playing, new_paused);
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

    int at = 0;

    while (1)
    {
        if (current_node)
            assert (current_node->channels == channels);
        else
        {
            int node_time = time;

            /* There is no partly-built node, so start a new one.  Normally
             * there will be nodes in the queue already; if so, we want to copy
             * audio data from the signal starting at 30 milliseconds after the
             * beginning of the most recent node.  If there are no nodes in the
             * queue, we are at the beginning of the song or had an underrun,
             * and we want to copy the earliest audio data we have. */

            if (vis_list_tail)
                node_time = vis_list_tail->time + INTERVAL;

            at = channels * (int) ((int64_t) (node_time - time) * rate / 1000);

            if (at < 0)
                at = 0;
            if (at >= samples)
                break;

            current_node = alloc_node_locked (channels);
            current_node->time = node_time;
            current_frames = 0;
        }

        /* Copy as much data as we can, limited by how much we have and how much
         * space is left in the node.  If we cannot fill the node, we return and
         * wait for more data to be passed in the next call.  If we do fill the
         * node, we loop and start building a new one. */

        int copy = MIN (samples - at, channels * (FRAMES_PER_NODE - current_frames));
        memcpy (current_node->data + channels * current_frames, data + at, sizeof (float) * copy);
        current_frames += copy / channels;

        if (current_frames < FRAMES_PER_NODE)
            break;

        push_node_locked (current_node);
        current_node = NULL;
    }

UNLOCK:
    pthread_mutex_unlock (& mutex);
}

void vis_runner_enable (bool_t enable)
{
    pthread_mutex_lock (& mutex);
    enabled = enable;
    start_stop_locked (playing, paused);
    pthread_mutex_unlock (& mutex);
}
