/*
 * dbus.h
 * Copyright 2007 Ben Tucker
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

#ifndef AUDACIOUS_DBUS_H
#define AUDACIOUS_DBUS_H

#include <dbus/dbus-glib.h>

#define AUDACIOUS_DBUS_SERVICE      "org.atheme.audacious"
#define AUDACIOUS_DBUS_PATH         "/org/atheme/audacious"
#define AUDACIOUS_DBUS_INTERFACE    "org.atheme.audacious"
#define AUDACIOUS_DBUS_SERVICE_MPRIS    "org.mpris.audacious"
#define AUDACIOUS_DBUS_INTERFACE_MPRIS 	"org.freedesktop.MediaPlayer"
#define AUDACIOUS_DBUS_PATH_MPRIS_ROOT      "/"
#define AUDACIOUS_DBUS_PATH_MPRIS_PLAYER    "/Player"
#define AUDACIOUS_DBUS_PATH_MPRIS_TRACKLIST "/TrackList"

#define NONE                  = 0
#define CAN_GO_NEXT           = 1 << 0
#define CAN_GO_PREV           = 1 << 1
#define CAN_PAUSE             = 1 << 2
#define CAN_PLAY              = 1 << 3
#define CAN_SEEK              = 1 << 4
#define CAN_RESTORE_CONTEXT   = 1 << 5
#define CAN_PROVIDE_METADATA  = 1 << 6
#define PROVIDES_TIMING       = 1 << 7

void init_dbus (void);
void cleanup_dbus (void);
DBusGProxy * audacious_get_dbus_proxy (void);

#endif /* AUDACIOUS_DBUS_H */
