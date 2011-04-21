/*
 * Audacious
 * Copyright (c) 2010 William Pitcock <nenolod@dereferenced.org>
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

#include <libaudcore/vfs_async.h>

typedef struct {
    gchar *filename;
    void *buf;
    gint64 size;
    GThread *thread;
    gpointer userdata;

    VFSConsumer cons_f;
} VFSAsyncTrampoline;

gboolean
vfs_async_file_get_contents_trampoline(gpointer data)
{
    VFSAsyncTrampoline *tr = data;

    tr->cons_f(tr->buf, tr->size, tr->userdata);
    g_slice_free(VFSAsyncTrampoline, tr);

    return FALSE;
}

gpointer
vfs_async_file_get_contents_worker(gpointer data)
{
    VFSAsyncTrampoline *tr = data;

    vfs_file_get_contents(tr->filename, &tr->buf, &tr->size);

    g_idle_add_full(G_PRIORITY_HIGH_IDLE, vfs_async_file_get_contents_trampoline, tr, NULL);
    g_thread_exit(NULL);
    return NULL;
}

void
vfs_async_file_get_contents(const gchar *filename, VFSConsumer cons_f, gpointer userdata)
{
    VFSAsyncTrampoline *tr;

    tr = g_slice_new0(VFSAsyncTrampoline);
    tr->filename = g_strdup(filename);
    tr->cons_f = cons_f;
    tr->userdata = userdata;
    tr->thread = g_thread_create(vfs_async_file_get_contents_worker, tr, FALSE, NULL);
}
