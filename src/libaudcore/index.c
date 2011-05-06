/*
 * index.c
 * Copyright 2009-2010 John Lindgren
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "index.h"

struct index
{
    void * * data;
    gint count, size;
    gint (* compare) (const void * a, const void * b, void * data);
    void * compare_data;
};

struct index * index_new (void)
{
    struct index * index = g_slice_new (struct index);

    index->data = NULL;
    index->count = 0;
    index->size = 0;
    index->compare = NULL;
    index->compare_data = NULL;

    return index;
}

void index_free (struct index * index)
{
    g_free (index->data);
    g_slice_free (struct index, index);
}

gint index_count (struct index * index)
{
    return index->count;
}

void index_allocate (struct index * index, gint size)
{
    if (size <= index->size)
        return;

    if (! index->size)
        index->size = 64;

    while (size > index->size)
        index->size <<= 1;

    index->data = g_realloc (index->data, sizeof (void *) * index->size);
}

void index_set (struct index * index, gint at, void * value)
{
    index->data[at] = value;
}

void * index_get (struct index * index, gint at)
{
    return index->data[at];
}

static void make_room (struct index * index, gint at, gint count)
{
    index_allocate (index, index->count + count);

    if (at < index->count)
        memmove (index->data + at + count, index->data + at, sizeof (void *) *
         (index->count - at));

    index->count += count;
}

void index_insert (struct index * index, gint at, void * value)
{
    make_room (index, at, 1);
    index->data[at] = value;
}

void index_append (struct index * index, void * value)
{
    index_insert (index, index->count, value);
}

void index_copy_set (struct index * source, gint from, struct index * target,
 gint to, gint count)
{
    memcpy (target->data + to, source->data + from, sizeof (void *) * count);
}

void index_copy_insert (struct index * source, gint from, struct index * target,
 gint to, gint count)
{
    make_room (target, to, count);
    memcpy (target->data + to, source->data + from, sizeof (void *) * count);
}

void index_copy_append (struct index * source, gint from, struct index * target,
 gint count)
{
    index_copy_insert (source, from, target, target->count, count);
}

void index_merge_insert (struct index * first, gint at, struct index * second)
{
    index_copy_insert (second, 0, first, at, second->count);
}

void index_merge_append (struct index * first, struct index * second)
{
    index_copy_insert (second, 0, first, first->count, second->count);
}

void index_move (struct index * index, gint from, gint to, gint count)
{
    memmove (index->data + to, index->data + from, sizeof (void *) * count);
}

void index_delete (struct index * index, gint at, gint count)
{
    index->count -= count;
    memmove (index->data + at, index->data + at + count, sizeof (void *) *
     (index->count - at));
}

static gint index_compare (const void * a, const void * b, void * _compare)
{
    gint (* compare) (const void *, const void *) = _compare;

    return compare (* (const void * *) a, * (const void * *) b);
}

void index_sort (struct index * index, gint (* compare) (const void *, const
 void *))
{
    g_qsort_with_data (index->data, index->count, sizeof (void *),
     index_compare, compare);
}

static gint index_compare_with_data (const void * a, const void * b, void *
 _index)
{
    struct index * index = _index;

    return index->compare (* (const void * *) a, * (const void * *) b,
     index->compare_data);
}

void index_sort_with_data (struct index * index, gint (* compare)
 (const void * a, const void * b, void * data), void * data)
{
    index->compare = compare;
    index->compare_data = data;
    g_qsort_with_data (index->data, index->count, sizeof (void *),
     index_compare_with_data, index);
    index->compare = NULL;
    index->compare_data = NULL;
}
