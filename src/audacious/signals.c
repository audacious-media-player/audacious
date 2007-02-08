/*
 * Audacious
 * Copyright (c) 2005-2007 Audacious development team
 *
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2005  BMP development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "main.h"
#include "ui_main.h"
#include "signals.h"

GCond *exit_cond;
GMutex *exit_mutex;

typedef void (*SignalHandler) (gint);

static SignalHandler
signal_install_handler_full (gint           signal_number,
                             SignalHandler  handler,
                             gint          *signals_to_block,
                             gsize          n_signals)
{
    struct sigaction action, old_action;
    gsize i;

    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;

    sigemptyset (&action.sa_mask);

    for (i = 0; i < n_signals; i++)
        sigaddset (&action.sa_mask, signals_to_block[i]);

    if (sigaction (signal_number, &action, &old_action) == -1)
    {
        g_message ("Failed to install handler for signal %d", signal_number);
        return NULL;
    }

    return old_action.sa_handler;
}

/* 
 * A version of signal() that works more reliably across different
 * platforms. It: 
 * a. restarts interrupted system calls
 * b. does not reset the handler
 * c. blocks the same signal within the handler
 *
 * (adapted from Unix Network Programming Vol. 1)
 */
static SignalHandler
signal_install_handler (gint          signal_number,
                        SignalHandler handler)
{
    return signal_install_handler_full (signal_number, handler, NULL, 0);
}

static void
signal_empty_handler (gint signal_number)
{
    /* empty */
}

static void
sigsegv_handler (gint signal_number)
{
#ifndef SIGSEGV_ABORT
    g_printerr(_("\nReceived SIGSEGV\n\n"
                 "This could be a bug in Audacious. If you don't know why this happened, "
                 "file a bug at http://bugs-meta.atheme.org/\n\n"));
    g_critical("Received SIGSEGV");

    /* TO DO: log stack trace and possibly dump config file. */
    exit (EXIT_FAILURE);
#else
    abort ();
#endif
}

static void
sigterm_handler (gint signal_number)
{
    cfg.terminate = TRUE;
    g_cond_signal(exit_cond);
}

static void *
signal_process_events (void *data)
{
    while (1) {
        if (cfg.terminate == TRUE)
        {
            g_print("Audacious has received SIGTERM and is shutting down.\n");
            mainwin_quit_cb();
        }
        g_mutex_lock(exit_mutex);
        g_cond_wait(exit_cond, exit_mutex);
        g_mutex_unlock(exit_mutex);
    }

    return NULL;
}

void 
signal_handlers_init (void)
{
    char *magic;
    magic = getenv("AUD_ENSURE_BACKTRACE");

    exit_cond = g_cond_new();
    exit_mutex = g_mutex_new();

    signal_install_handler(SIGPIPE, signal_empty_handler);
    signal_install_handler(SIGINT, sigterm_handler);
    signal_install_handler(SIGTERM, sigterm_handler);

    /* in particular environment (maybe with glibc 2.5), core file
       through signal handler doesn't contain useful back trace. --yaz */
    if (magic == NULL)
        signal_install_handler(SIGSEGV, sigsegv_handler);

    g_thread_create(signal_process_events, NULL, FALSE, NULL);

}
