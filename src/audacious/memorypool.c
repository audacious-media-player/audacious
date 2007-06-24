/*
 * audacious: Cross-platform multimedia player.
 * memorypool.c: Memory pooling.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
