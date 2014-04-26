/*
 * hook.h
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

#ifndef LIBAUDCORE_HOOK_H
#define LIBAUDCORE_HOOK_H

typedef void (* HookFunction) (void * data, void * user);

/* Adds <func> to the list of functions to be called when the hook <name> is
 * triggered. */
void hook_associate (const char * name, HookFunction func, void * user);

/* Removes all instances matching <func> and <user> from the list of functions
 * to be called when the hook <name> is triggered.  If <user> is NULL, all
 * instances matching <func> are removed. */
void hook_dissociate_full (const char * name, HookFunction func, void * user);

#define hook_dissociate(n, f) hook_dissociate_full (n, f, NULL)

/* Triggers the hook <name>. */
void hook_call (const char * name, void * data);

/* Schedules a call of the hook <name> from the program's main loop, to be
 * executed in <time> milliseconds.  If <destroy> is not NULL, it will be called
 * on <data> after the hook is called. */
void event_queue_full (int time, const char * name, void * data, void (* destroy) (void *));

#define event_queue(n, d) event_queue_full (0, n, d, NULL)

/* Cancels pending hook calls matching <name> and <data>.  If <data> is NULL,
 * all hook calls matching <name> are canceled. */
void event_queue_cancel (const char * name, void * data);

/* Cancels all pending hook calls. */
void event_queue_cancel_all (void);

#endif /* LIBAUDCORE_HOOK_H */
