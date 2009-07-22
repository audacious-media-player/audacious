/*
 * index.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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
    gint length, size;
};

struct index * index_new (void)
{
    struct index * index = g_malloc (sizeof (struct index));

    index->data = NULL;
    index->length = 0;
    index->size = 0;

    return index;
}

void index_free (struct index * index)
{
    g_free (index->data);
    g_free (index);
}

gint index_count (struct index * index)
{
    return index->length;
}

void index_set (struct index * index, gint at, void * value)
{
    index->data[at] = value;
}

void * index_get (struct index * index, gint at)
{
    return index->data[at];
}

static void resize_to (struct index * index, gint size)
{
    if (size < 100)
        size = (size + 9) / 10 * 10;
    else if (size < 1000)
        size = (size + 99) / 100 * 100;
    else
        size = (size + 999) / 1000 * 1000;

    if (index->size < size)
    {
        index->data = g_realloc (index->data, sizeof (void *) * size);
        index->size = size;
    }
}

static void make_room (struct index * index, gint at, gint length)
{
    resize_to (index, index->length + length);
    memmove (index->data + at + length, index->data + at, sizeof (void *) *
     (index->length - at));
    index->length += length;
}

void index_insert (struct index * index, gint at, void * value)
{
    make_room (index, at, 1);
    index->data[at] = value;
}

void index_append (struct index * index, void * value)
{
    resize_to (index, index->length + 1);
    index->data[index->length] = value;
    index->length ++;
}

void index_merge_insert (struct index * first, gint at, struct index * second)
{
    make_room (first, at, second->length);
    memcpy (first->data + at, second->data, sizeof (void *) * second->length);
}

void index_merge_append (struct index * first, struct index * second)
{
    resize_to (first, first->length + second->length);
    memcpy (first->data + first->length, second->data, sizeof (void *) *
     second->length);
    first->length += second->length;
}

void index_delete (struct index * index, gint at, gint length)
{
    index->length -= length;
    memmove (index->data + at, index->data + at + length, sizeof (void *) *
     (index->length - at));
}

void index_sort (struct index * index, gint (* compare) (const void * *, const
 void * *))
{
    qsort (index->data, index->length, sizeof (void *), (gint (*) (const void *,
     const void *)) compare);
}
