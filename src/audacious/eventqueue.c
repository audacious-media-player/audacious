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

#include "eventqueue.h"

static gboolean eventqueue_handle(gpointer udata)
{
    HookCallQueue *hq = (HookCallQueue *) udata;

    hook_call(hq->name, hq->user_data);

    g_free(hq->name);
    g_slice_free(HookCallQueue, hq);

    return FALSE;
}

void event_queue(const gchar *name, gpointer user_data)
{
    HookCallQueue *hq;

    g_return_if_fail(name != NULL);
    g_return_if_fail(user_data != NULL);

    hq = g_slice_new0(HookCallQueue);
    hq->name = g_strdup(name);
    hq->user_data = user_data;

    g_idle_add_full(G_PRIORITY_HIGH_IDLE, eventqueue_handle, hq, NULL);
}
