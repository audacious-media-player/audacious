/*
 * Audacious
 * Copyright (c) 2005-2007 Yoshiki Yazawa
 * Copyright 2009 John Lindgren (POSIX threaded signal handling)
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

#include <signal.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <libaudcore/eventqueue.h>

#include "audconfig.h"
#include "config.h"
#include "main.h"
#include "signals.h"

#ifdef USE_EGGSM
#include "eggsmclient.h"
#endif

static sigset_t signal_set;

#ifdef USE_EGGSM
static void
signal_session_quit_cb(EggSMClient *client, gpointer user_data)
{
    const gchar * argv[2];

    g_print("Session quit requested. Saving state and shutting down.\n");

    argv[0] = "audacious";
    argv[1] = g_strdup_printf ("--display=%s", gdk_display_get_name (gdk_display_get_default()));
    egg_sm_client_set_restart_command (client, 2, argv);

    aud_quit();
}

static void
signal_session_save_cb(EggSMClient *client, GKeyFile *state_file, gpointer user_data)
{
    const gchar * argv[2];

    g_print("Session save requested. Saving state.\n");

    argv[0] = "audacious";
    argv[1] = g_strdup_printf ("--display=%s", gdk_display_get_name (gdk_display_get_default()));
    egg_sm_client_set_restart_command (client, 2, argv);

    aud_config_save();
}
#endif

static void * signal_thread (void * data)
{
    gint signal;

    while (! sigwait (& signal_set, & signal))
        event_queue ("quit", 0);

    return NULL;
}

/* Must be called before any threads are created. */
void signal_handlers_init (void)
{
#ifdef USE_EGGSM
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
#endif

    sigemptyset (& signal_set);
    sigaddset (& signal_set, SIGHUP);
    sigaddset (& signal_set, SIGINT);
    sigaddset (& signal_set, SIGQUIT);
    sigaddset (& signal_set, SIGTERM);

    sigprocmask (SIG_BLOCK, & signal_set, NULL);
    g_thread_create (signal_thread, NULL, FALSE, NULL);
}
