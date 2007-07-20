/*  Audacious
 *  Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
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
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "memorypool.h"

/* visibility of this object is not available to the outside */
struct _MemoryPool {
    GList *stack;
    GDestroyNotify notify;
    GMutex *mutex;
};

MemoryPool *
memory_pool_new(void)
{
    MemoryPool *pool;

    pool = g_new0(MemoryPool, 1);
    pool->notify = g_free;
    pool->mutex = g_mutex_new();

    return pool;
}

MemoryPool *
memory_pool_with_custom_destructor(GDestroyNotify notify)
{
    MemoryPool *pool;

    pool = g_new0(MemoryPool, 1);
    pool->notify = notify;
    pool->mutex = g_mutex_new();

    return pool;
}

gpointer
memory_pool_add(MemoryPool * pool, gpointer ptr)
{
    g_mutex_lock(pool->mutex);
    pool->stack = g_list_append(pool->stack, ptr);
    g_mutex_unlock(pool->mutex);

    return ptr;
}

gpointer
memory_pool_allocate(MemoryPool * pool, gsize sz)
{
    gpointer addr;

    g_mutex_lock(pool->mutex);
    addr = g_malloc0(sz);
    pool->stack = g_list_append(pool->stack, addr);
    g_mutex_unlock(pool->mutex);

    return addr;
}

void
memory_pool_release(MemoryPool * pool, gpointer addr)
{
    g_mutex_lock(pool->mutex);

    pool->stack = g_list_remove(pool->stack, addr);
    pool->notify(addr);

    g_mutex_unlock(pool->mutex);
}

static void
memory_pool_cleanup_nolock(MemoryPool * pool)
{
    GList *iter;

    for (iter = pool->stack; iter != NULL; iter = g_list_next(iter))
    {
        pool->stack = g_list_delete_link(pool->stack, iter);
        g_warning("MemoryPool<%p> element at %p was not released until cleanup!", pool, iter->data);
        pool->notify(iter->data);
    }
}

void
memory_pool_cleanup(MemoryPool * pool)
{
    g_mutex_lock(pool->mutex);
    memory_pool_cleanup_nolock(pool);
    g_mutex_unlock(pool->mutex);
}

void
memory_pool_destroy(MemoryPool * pool)
{
    g_mutex_lock(pool->mutex);
    memory_pool_cleanup_nolock(pool);
    g_mutex_unlock(pool->mutex);

    g_mutex_free(pool->mutex);
    g_free(pool);
}

gchar *
memory_pool_strdup(MemoryPool * pool, gchar * src)
{
    gchar *out;
    gsize sz = strlen(src) + 1;

    out = memory_pool_allocate(pool, sz);
    g_strlcpy(out, src, sz);

    return out;
}
