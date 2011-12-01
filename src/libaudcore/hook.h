/*  Audacious
 *  Copyright (c) 2006-2007 William Pitcock
 *  Copyright (c) 2011 John Lindgren
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef LIBAUDCORE_HOOK_H
#define LIBAUDCORE_HOOK_H

typedef void (*HookFunction)(void * hook_data, void * user_data);

int hook_associate(const char *name, HookFunction func, void * user_data);
int hook_dissociate(const char *name, HookFunction func);
int hook_dissociate_full(const char *name, HookFunction func, void * user_data);
void hook_call(const char *name, void * hook_data);

/* Schedules a call of the hook <name> from the program's main loop, to be
 * executed in <time> milliseconds.  If <destroy> is not NULL, it will be called
 * on <data> after the hook is called. */
void event_queue_full (int time, const char * name, void * data, void (* destroy) (void *));

#define event_queue(n, d) event_queue_full (0, n, d, NULL)

/* Cancels pending hook calls matching <name> and <data>.  If <data> is null,
 * all hook calls matching <name> are canceled. */
void event_queue_cancel (const char * name, void * data);

#endif /* LIBAUDCORE_HOOK_H */
