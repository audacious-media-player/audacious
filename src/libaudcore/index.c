/*
 * index.c
 * Copyright 2009-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "core.h"
#include "index.h"

struct _Index {
    void * * data;
    int count, size;
    void * sdata[16];
};

typedef struct {
    int (* compare) (const void * a, const void * b);
} CompareWrapper;

typedef struct {
    int (* compare) (const void * a, const void * b, void * data);
    void * data;
} CompareWrapper2;

EXPORT Index * index_new (void)
{
    Index * index = g_new0 (Index, 1);
    index->data = index->sdata;
    index->size = ARRAY_LEN (index->sdata);
    return index;
}

EXPORT void index_free (Index * index)
{
    if (index->data != index->sdata)
        g_free (index->data);

    g_free (index);
}

EXPORT void index_free_full (Index * index, IndexFreeFunc func)
{
    for (int i = 0; i < index->count; i ++)
        func (index->data[i]);

    index_free (index);
}

EXPORT int index_count (Index * index)
{
    return index->count;
}

EXPORT void index_allocate (Index * index, int size)
{
    assert (size >= 0);

    if (index->size >= size)
        return;

    if (index->size < 64)
        index->size = 64;

    while (index->size < size)
        index->size <<= 1;

    if (index->data == index->sdata)
    {
        index->data = g_new (void *, index->size);
        memcpy (index->data, index->sdata, sizeof index->sdata);
    }
    else
        index->data = g_renew (void *, index->data, index->size);
}

EXPORT void * index_get (Index * index, int at)
{
    assert (at >= 0 && at < index->count);

    return index->data[at];
}

EXPORT void index_set (Index * index, int at, void * value)
{
    assert (at >= 0 && at < index->count);

    index->data[at] = value;
}

static void make_room (Index * index, int at, int count)
{
    index_allocate (index, index->count + count);

    if (at < index->count)
        memmove (index->data + at + count, index->data + at, sizeof (void *) * (index->count - at));

    index->count += count;
}

EXPORT void index_insert (Index * index, int at, void * value)
{
    if (at == -1)
        at = index->count;

    assert (at >= 0 && at <= index->count);

    make_room (index, at, 1);
    index->data[at] = value;
}

EXPORT void index_copy_set (Index * source, int from, Index * target, int to, int count)
{
    assert (count >= 0);
    assert (from >= 0 && from + count <= source->count);
    assert (to >= 0 && to + count <= target->count);

    memmove (target->data + to, source->data + from, sizeof (void *) * count);
}

EXPORT void index_copy_insert (Index * source, int from, Index * target, int to, int count)
{
    if (to == -1)
        to = target->count;
    if (count == -1)
        count = source->count - from;

    assert (count >= 0);
    assert (from >= 0 && from + count <= source->count);
    assert (to >= 0 && to <= target->count);

    make_room (target, to, count);

    if (source == target && to <= from)
        index_copy_set (source, from + count, target, to, count);
    else if (source == target && to <= from + count)
    {
        index_copy_set (source, from, target, to, to - from);
        index_copy_set (source, to + count, target, to + (to - from), count - (to - from));
    }
    else
        index_copy_set (source, from, target, to, count);
}

EXPORT void index_delete (Index * index, int at, int count)
{
    if (count == -1)
        count = index->count - at;

    assert (count >= 0);
    assert (at >= 0 && at + count <= index->count);

    index_copy_set (index, at + count, index, at, index->count - (at + count));

    index->count -= count;
}

EXPORT void index_delete_full (Index * index, int at, int count, IndexFreeFunc func)
{
    if (count == -1)
        count = index->count - at;

    assert (count >= 0);
    assert (at >= 0 && at + count <= index->count);

    for (int i = at; i < at + count; i ++)
        func (index->data[i]);

    index_delete (index, at, count);
}

static int index_compare (const void * ap, const void * bp, void * _wrapper)
{
    CompareWrapper * wrapper = _wrapper;
    return wrapper->compare (* (const void * *) ap, * (const void * *) bp);
}

EXPORT void index_sort (Index * index, int (* compare) (const void *, const void *))
{
    CompareWrapper wrapper = {compare};
    g_qsort_with_data (index->data, index->count, sizeof (void *), index_compare, & wrapper);
}

static int index_compare2 (const void * ap, const void * bp, void * _wrapper)
{
    CompareWrapper2 * wrapper = _wrapper;
    return wrapper->compare (* (const void * *) ap, * (const void * *) bp, wrapper->data);
}

EXPORT void index_sort_with_data (Index * index, int (* compare)
 (const void * a, const void * b, void * data), void * data)
{
    CompareWrapper2 wrapper = {compare, data};
    g_qsort_with_data (index->data, index->count, sizeof (void *), index_compare2, & wrapper);
}
