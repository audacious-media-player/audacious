/*
 * mpris-signals.c
 * Copyright 2011 John Lindgren
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

#ifdef USE_DBUS

#include <libaudcore/hook.h>

#include "dbus-service.h"
#include "main.h"

static void mpris_status_cb (void * hook_data, void * user_data)
{
    mpris_emit_status_change (mpris, GPOINTER_TO_INT (user_data));
}
#endif

void mpris_signals_init (void)
{
#ifdef USE_DBUS
    hook_associate ("playback begin", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PLAY));
    hook_associate ("playback pause", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PAUSE));
    hook_associate ("playback unpause", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_PLAY));
    hook_associate ("playback stop", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_STOP));

    hook_associate ("set shuffle", mpris_status_cb, GINT_TO_POINTER (MPRIS_STATUS_INVALID));
    hook_associate ("set repeat", mpris_status_cb, GINT_TO_POINTER (MPRIS_STATUS_INVALID));
    hook_associate ("set no_playlist_advance", mpris_status_cb, GINT_TO_POINTER
     (MPRIS_STATUS_INVALID));
#endif
}

void mpris_signals_cleanup (void)
{
#ifdef USE_DBUS
    hook_dissociate ("playback begin", mpris_status_cb);
    hook_dissociate ("playback pause", mpris_status_cb);
    hook_dissociate ("playback unpause", mpris_status_cb);
    hook_dissociate ("playback stop", mpris_status_cb);

    hook_dissociate ("set shuffle", mpris_status_cb);
    hook_dissociate ("set repeat", mpris_status_cb);
    hook_dissociate ("set no_playlist_advance", mpris_status_cb);
#endif
}
