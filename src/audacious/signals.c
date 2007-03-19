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
            g_printerr(_("\nReceived SIGSEGV\n\n"
                         "This could be a bug in Audacious. If you don't know why this happened, "
                         "file a bug at http://bugs-meta.atheme.org/\n\n"));
            g_critical("Received SIGSEGV");
            bmp_config_save();
            abort();
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

void 
signal_handlers_init (void)
{
    sigset_t blockset;

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGPIPE);
    sigaddset(&blockset, SIGSEGV);  
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGTERM);

    if(pthread_sigmask(SIG_BLOCK, &blockset, NULL))
        g_print("pthread_sigmask() failed.\n");

    g_thread_create(signal_process_signals, NULL, FALSE, NULL);
}
