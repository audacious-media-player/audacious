/*
 * Audacious
 * Copyright (c) 2006-2007 Audacious development team.
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

#include <glib.h>

#include "audacious/hook.h"

#ifndef AUDACIOUS_EVENTQUEUE_H
#define AUDACIOUS_EVENTQUEUE_H

typedef struct {
    gchar *name;
    gpointer *user_data;
    gboolean free_data;
} HookCallQueue;

void event_queue(const gchar *name, gpointer user_data);
void event_queue_timed(gint time, const gchar *name, gpointer user_data);
void event_queue_with_data_free(const gchar *name, gpointer user_data);

#endif /* AUDACIOUS_EVENTQUEUE_H */
