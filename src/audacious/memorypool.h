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
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
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
