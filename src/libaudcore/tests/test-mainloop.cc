/*
 * test-mainloop.cc - Main loop test for libaudcore
 * Copyright 2014 John Lindgren
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

#include "mainloop.h"
#include "runtime.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static bool use_qt = false;

MainloopType aud_get_mainloop_type ()
{
    return use_qt ? MainloopType::Qt : MainloopType::GLib;
}

static QueuedFunc counters[70];
static QueuedFunc fast, slow;

static int count;
static pthread_t main_thread;

static void count_up (void * data)
{
    assert (pthread_self () == main_thread);
    assert (count == (int) (size_t) data);

    if (! (count % 10))
        printf ("UP: ");

    count ++;

    printf ("%d%c", count, (count % 10) ? ' ' : '\n');
}

static void count_down (void * data)
{
    assert (pthread_self () == main_thread);
    assert (data == & count);

    count -= 10;

    printf ("DOWN: %d\n", count);

    if (! count)
    {
        fast.stop ();
        slow.stop ();
        mainloop_quit ();
    }
}

static void check_count (void * data)
{
    assert (pthread_self () == main_thread);
    assert (count == (int) (size_t) data);

    printf ("CHECK: %d\n", count);
}

static void * worker (void * data)
{
    for (int i = 50; i < 70; i ++)
        counters[i].queue (count_up, (void *) (size_t) (i - 10));

    slow.start (350, check_count, (void *) (size_t) 30);

    return nullptr;
}

int main (int argc, const char * * argv)
{
    if (argc >= 2 && ! strcmp (argv[1], "--qt"))
        use_qt = true;

    main_thread = pthread_self ();

    for (int j = 0; j < 2; j ++)
    {
        for (int i = 0; i < 50; i ++)
            counters[i].queue (count_up, (void *) (size_t) (i - 30));

        for (int i = 10; i < 30; i ++)
            counters[i].stop ();

        for (int i = 0; i < 20; i ++)
            counters[i].queue (count_up, (void *) (size_t) (20 + i));

        fast.start (100, count_down, & count);

        pthread_t thread;
        pthread_create (& thread, nullptr, worker, nullptr);

        mainloop_run ();

        pthread_join (thread, nullptr);
    }

    return 0;
}
