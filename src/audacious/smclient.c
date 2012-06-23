/*
 * smclient.c
 * Copyright 2008-2009 Ivan Zlatev and David Mohr
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

#include <gdk/gdk.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "main.h"

#ifdef USE_EGGSM
#include "eggsmclient.h"

static void
signal_session_quit_cb(EggSMClient *client, void * user_data)
{
    const char * argv[2];

    g_print("Session quit requested. Saving state and shutting down.\n");

    argv[0] = "audacious";
    argv[1] = g_strdup_printf ("--display=%s", gdk_display_get_name (gdk_display_get_default()));
    egg_sm_client_set_restart_command (client, 2, argv);

    hook_call ("quit", NULL);
}

static void
signal_session_save_cb(EggSMClient *client, GKeyFile *state_file, void * user_data)
{
    const char * argv[2];

    g_print("Session save requested. Saving state.\n");

    argv[0] = "audacious";
    argv[1] = g_strdup_printf ("--display=%s", gdk_display_get_name (gdk_display_get_default()));
    egg_sm_client_set_restart_command (client, 2, argv);

    do_autosave ();
}
#endif

void smclient_init (void)
{
#ifdef USE_EGGSM
    EggSMClient *client;

    client = egg_sm_client_get ();
    if (client != NULL)
    {
        g_signal_connect (client, "quit",
                          G_CALLBACK (signal_session_quit_cb), NULL);
        g_signal_connect (client, "save-state",
                          G_CALLBACK (signal_session_save_cb), NULL);

    }
#endif
}
