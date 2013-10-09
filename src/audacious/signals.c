/*
 * signals.c
 * Copyright 2009 John Lindgren
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

#ifdef HAVE_SIGWAIT

#include <pthread.h>
#include <signal.h>

#include "drct.h"
#include "main.h"

static sigset_t signal_set;

static void * signal_thread (void * data)
{
    int signal;

    while (! sigwait (& signal_set, & signal))
        drct_quit ();

    return NULL;
}

/* Must be called before any threads are created. */
void signals_init_one (void)
{
    sigemptyset (& signal_set);
    sigaddset (& signal_set, SIGHUP);
    sigaddset (& signal_set, SIGINT);
    sigaddset (& signal_set, SIGQUIT);
    sigaddset (& signal_set, SIGTERM);

    sigprocmask (SIG_BLOCK, & signal_set, NULL);
}

void signals_init_two (void)
{
    pthread_t thread;
    pthread_create (& thread, NULL, signal_thread, NULL);
}

#endif /* HAVE_SIGWAIT */
