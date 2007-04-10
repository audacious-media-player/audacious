/*
 * Audacious
 * Copyright (c) 2005-2007 Yoshiki Yazawa
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

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif

#include "main.h"
#include "ui_main.h"
#include "signals.h"
#include "build_stamp.h"

static void
signal_process_segv(void)
{
    g_printerr(_("\nAudacious has caught signal 11 (SIGSEGV).\n\n"
         "We apologize for the inconvenience, but Audacious has crashed.\n"
         "This is a bug in the program, and should never happen under normal circumstances.\n"
	 "Your current configuration has been saved and should not be damaged.\n\n"
	 "You can help improve the quality of Audacious by filing a bug at http://bugs-meta.atheme.org\n"
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
           g_printerr("   %d. %s\n", i + 1, strings[i]);

        g_free(strings);
    }
#else
    g_printerr("Stacktrace was unavailable.\n");
#endif

    g_printerr(_("\nBugs can be reported at http://bugs-meta.atheme.org against the Audacious product.\n"));

    g_critical("Received SIGSEGV -- Audacious has crashed.");

    bmp_config_save();
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
            mainwin_quit_cb();
            break;

        case SIGTERM:
            g_print("Audacious has received SIGTERM and is shutting down.\n");
            mainwin_quit_cb();
            break;
        }
    }

    return NULL; //dummy
}

/* sets up blocking signals for pthreads. 
 * linuxthreads sucks and needs this to make sigwait(2) work 
 * correctly. --nenolod
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

    if(pthread_sigmask(SIG_SETMASK, &blockset, NULL))
        g_print("pthread_sigmask() failed.\n");    
}

void 
signal_handlers_init (void)
{
    signal_initialize_blockers();
    pthread_atfork(NULL, NULL, signal_initialize_blockers);

    g_thread_create(signal_process_signals, NULL, FALSE, NULL);
}
