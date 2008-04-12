/*
 * Audacious
 * Copyright (c) 2005-2007 Yoshiki Yazawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

//#define _XOPEN_SOURCE
#include <unistd.h>	/* for signal_check_for_broken_impl() */

#include <glib.h>
#include <glib/gi18n.h>
#include <config.h>
#include <stdlib.h>
#include <pthread.h>	/* for pthread_sigmask() */
#include <signal.h>

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif

#include "main.h"
#include "signals.h"
#include "build_stamp.h"
#include "eggsmclient.h"

gint linuxthread_signal_number = 0;

static void
signal_process_segv(void)
{
    g_printerr(_("\nAudacious has caught signal 11 (SIGSEGV).\n\n"
         "We apologize for the inconvenience, but Audacious has crashed.\n"
         "This is a bug in the program, and should never happen under normal circumstances.\n"
	 "Your current configuration has been saved and should not be damaged.\n\n"
	 "You can help improve the quality of Audacious by filing a bug at http://bugzilla.atheme.org/\n"
         "Please include the entire text of this message and a description of what you were doing when\n"
         "this crash occured in order to quickly expedite the handling of your bug report:\n\n"));

    g_printerr("Program version: Audacious %s (buildid: %s)\n\n", VERSION, svn_stamp);

#ifdef HAVE_EXECINFO_H
    {
        void *stack[20];
        size_t size;
        char **strings;
        size_t i;

        size = backtrace(stack, 20);
        strings = backtrace_symbols(stack, size);

        g_printerr("Stacktrace (%zd frames):\n", size);

        for (i = 0; i < size; i++)
            g_printerr("   %ld. %s\n", (long)i + 1, strings[i]);

        g_free(strings);
    }
#else
    g_printerr(_("Stacktrace was unavailable. You might want to reproduce this "
    "problem while running Audacious under GDB to get a proper backtrace.\n"));
#endif

    g_printerr(_("\nBugs can be reported at http://bugzilla.atheme.org/ against "
    "the Audacious or Audacious Plugins product.\n"));

    g_critical("Received SIGSEGV -- Audacious has crashed.");

    aud_config_save();
    abort();
}

static void *
signal_process_signals (void *data)
{
    sigset_t waitset;
    int sig;

    sigemptyset(&waitset);
    sigaddset(&waitset, SIGPIPE);
    sigaddset(&waitset, SIGSEGV);  
    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGTERM);

    while(1) {
        sigwait(&waitset, &sig);

        switch(sig){
        case SIGPIPE:
            /*
             * do something.
             */
            break;

        case SIGSEGV:
            signal_process_segv();
            break;

        case SIGINT:
            g_print("Audacious has received SIGINT and is shutting down.\n");
            aud_quit();
            break;

        case SIGTERM:
            g_print("Audacious has received SIGTERM and is shutting down.\n");
            aud_quit();
            break;
        }
    }

    return NULL; //dummy
}

/********************************************************************************/
/* for linuxthread */
/********************************************************************************/

typedef void (*SignalHandler) (gint);

static void *
signal_process_signals_linuxthread (void *data)
{
    while(1) {
        g_usleep(1000000);

        switch(linuxthread_signal_number){
        case SIGPIPE:
            /*
             * do something.
             */
            linuxthread_signal_number = 0;
            break;

        case SIGSEGV:
            signal_process_segv();
            break;

        case SIGINT:
            g_print("Audacious has received SIGINT and is shutting down.\n");
            aud_quit();
            break;

        case SIGTERM:
            g_print("Audacious has received SIGTERM and is shutting down.\n");
            aud_quit();
            break;
        }
    }

    return NULL; //dummy
}

static void
linuxthread_handler (gint signal_number)
{
    /* note: cannot manipulate mutex from signal handler */
    linuxthread_signal_number = signal_number;
}

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


/* sets up blocking signals for pthreads. 
 * linuxthreads sucks and needs this to make sigwait(2) work 
 * correctly. --nenolod
 *
 * correction -- this trick does not work on linuxthreads.
 * going to keep it in it's own function though --nenolod
 */
static void
signal_initialize_blockers(void)
{
    sigset_t blockset;

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGPIPE);
    sigaddset(&blockset, SIGSEGV);  
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGTERM);

    if(pthread_sigmask(SIG_BLOCK, &blockset, NULL))
        g_print("pthread_sigmask() failed.\n");    
}

static gboolean
signal_check_for_broken_impl(void)
{
#ifdef _CS_GNU_LIBPTHREAD_VERSION
    {
        gchar str[1024];
        confstr(_CS_GNU_LIBPTHREAD_VERSION, str, sizeof(str));

        if (g_ascii_strncasecmp("linuxthreads", str, 12) == 0)
            return TRUE;
    }
#endif

    return FALSE;
}

static void
signal_session_quit_cb(EggSMClient *client, gpointer user_data)
{
    g_print("Session quit requested. Saving state and shutting down.\n");    
    aud_quit();
}

static void
signal_session_save_cb(EggSMClient *client, const char *state_dir, gpointer user_data)
{
    g_print("Session save requested. Saving state.\n");    
    aud_config_save();
}

void 
signal_handlers_init(void)
{
    EggSMClient *client;

    client = egg_sm_client_get ();
    if (client != NULL) 
    {
        egg_sm_client_set_mode (EGG_SM_CLIENT_MODE_NORMAL);
        g_signal_connect (client, "quit",
                          G_CALLBACK (signal_session_quit_cb), NULL);
        g_signal_connect (client, "save-state",
                          G_CALLBACK (signal_session_save_cb), NULL);
    
    }

    if (signal_check_for_broken_impl() != TRUE)
    {
        signal_initialize_blockers();
        g_thread_create(signal_process_signals, NULL, FALSE, NULL);
    }
    else
    {
        g_printerr(_("Your signaling implementation is broken.\n"
		     "Expect unusable crash reports.\n"));

        /* install special handler which catches signals and forwards to the signal handling thread */
        signal_install_handler(SIGPIPE, linuxthread_handler);
        signal_install_handler(SIGSEGV, linuxthread_handler);
        signal_install_handler(SIGINT, linuxthread_handler);
        signal_install_handler(SIGTERM, linuxthread_handler);

        /* create handler thread */
        g_thread_create(signal_process_signals_linuxthread, NULL, FALSE, NULL);

    }
}
