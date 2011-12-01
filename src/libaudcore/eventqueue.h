/*
 * eventqueue.h
 * Copyright 2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_EVENTQUEUE_H
#define AUDACIOUS_EVENTQUEUE_H

#include <glib.h>

/* Schedules a call of the hook <name> from the program's main loop, to be
 * executed in <time> milliseconds.  If <free_data> is nonzero, <data> will be
 * freed with g_free() after the hook is called. */
void event_queue_full (int time, const char * name, void * data, gboolean free_data);

/* convenience macro */
#define event_queue(n,d) event_queue_full (0, n, d, FALSE)

/* deprecated; use event_queue_full() instead */
#define event_queue_timed(t,n,d) event_queue_full (t, n, d, FALSE)
#define event_queue_with_data_free(n,d) event_queue_full (0, n, d, TRUE)

/* Cancels pending hook calls matching <name> and <data>.  If <data> is null,
 * all hook calls matching <name> are canceled. */
void event_queue_cancel (const char * name, void * data);

#endif
