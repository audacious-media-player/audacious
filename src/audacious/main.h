/*
 * main.h
 * Copyright 2011-2013 John Lindgren
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

#ifndef _AUDACIOUS_MAIN_H
#define _AUDACIOUS_MAIN_H

#include <libaudcore/objects.h>

/* dbus-server.c */
#ifdef USE_DBUS

enum class StartupType {
    Server,
    Client,
    Unknown
};

StringBuf dbus_server_name ();
StartupType dbus_server_init ();
void dbus_server_cleanup ();

#endif

/* signals.c */
#ifdef HAVE_SIGWAIT
void signals_init_one ();
void signals_init_two ();
#endif

#endif
