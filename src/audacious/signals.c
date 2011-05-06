/*
 * signals.c
 * Copyright 2009 John Lindgren
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

#include <signal.h>
#include <glib.h>
#include <libaudcore/eventqueue.h>

#include "config.h"

#ifdef HAVE_SIGWAIT
static sigset_t signal_set;

static void * signal_thread (void * data)
{
    gint signal;

    while (! sigwait (& signal_set, & signal))
        event_queue ("quit", NULL);

    return NULL;
}
#endif

/* Must be called before any threads are created. */
void signals_init (void)
{
#ifdef HAVE_SIGWAIT
    sigemptyset (& signal_set);
    sigaddset (& signal_set, SIGHUP);
    sigaddset (& signal_set, SIGINT);
    sigaddset (& signal_set, SIGQUIT);
    sigaddset (& signal_set, SIGTERM);

    sigprocmask (SIG_BLOCK, & signal_set, NULL);
    g_thread_create (signal_thread, NULL, FALSE, NULL);
#endif
}
