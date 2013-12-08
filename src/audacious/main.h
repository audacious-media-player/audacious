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

/* Header for all those files that have just one or two public identifiers. */

#ifndef _AUDACIOUS_MAIN_H
#define _AUDACIOUS_MAIN_H

#include <libaudcore/core.h>

/* adder.c */
void adder_init (void);
void adder_cleanup (void);

/* art.c */
void art_init (void);
void art_cleanup (void);

/* dbus-server.c */
#ifdef USE_DBUS
void dbus_server_init (void);
void dbus_server_cleanup (void);
#endif

/* chardet.c */
void chardet_init (void);
void chardet_cleanup (void);

/* config.c */
void config_load (void);
void config_save (void);
void config_cleanup (void);

/* history.c */
void history_cleanup (void);

/* main.c */
bool_t do_autosave (void);

/* signals.c */
#ifdef HAVE_SIGWAIT
void signals_init_one (void);
void signals_init_two (void);
#endif

/* ui_albumart.c */
char * get_associated_image_file (const char * filename); /* pooled */

#endif
