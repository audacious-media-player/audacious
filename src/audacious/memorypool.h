/*
 * audacious: Cross-platform multimedia player.
 * memorypool.h: Memory pooling.
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

#ifndef AUDACIOUS_MEMORYPOOL_H
#define AUDACIOUS_MEMORYPOOL_H

typedef struct _MemoryPool MemoryPool;

MemoryPool * memory_pool_new(void);
MemoryPool * memory_pool_with_custom_destructor(GDestroyNotify notify);

gpointer memory_pool_add(MemoryPool * pool, gpointer ptr);
gpointer memory_pool_allocate(MemoryPool * pool, gsize sz);
void memory_pool_release(MemoryPool * pool, gpointer addr);

void memory_pool_cleanup(MemoryPool * pool);

void memory_pool_destroy(MemoryPool * pool);

gchar * memory_pool_strdup(MemoryPool * pool, gchar * src);

#define memory_pool_alloc_object(pool, obj) \
	memory_pool_allocate(pool, sizeof(obj))

#endif
