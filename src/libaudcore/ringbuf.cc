/*
 * ringbuf.cc
 * Copyright 2014 John Lindgren
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

#include "ringbuf.h"
#include "internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void RingBufBase::get_areas (int pos, int len, Areas & areas)
{
    assert (pos >= 0 && len >= 0 && pos + len <= m_len);

    int start = (m_offset + pos) % m_size;
    int part = aud::min (len, m_size - start);

    areas.area1 = (char *) m_data + start;
    areas.area2 = m_data;
    areas.len1 = part;
    areas.len2 = len - part;
}

void RingBufBase::do_realloc (int size)
{
    void * mem = realloc (m_data, size);
    if (size && ! mem)
        throw std::bad_alloc ();  /* nothing changed yet */

    m_data = mem;
}

EXPORT void RingBufBase::alloc (int size)
{
    assert (size >= m_len);

    if (size == m_size)
        return;

    /* reallocate first when growing */
    if (size > m_size)
        do_realloc (size);

    __sync_add_and_fetch (& misc_bytes_allocated, size - m_size);

    int old_size = m_size;
    int to_end = m_size - m_offset;

    m_size = size;

    if (to_end < m_len)
    {
        int new_offset = size - to_end;
        memmove ((char *) m_data + new_offset, (char *) m_data + m_offset, to_end);
        m_offset = new_offset;
    }

    /* reallocate last when shrinking */
    if (size < old_size)
        do_realloc (size);
}

EXPORT void RingBufBase::destroy (aud::EraseFunc erase_func)
{
    if (! m_data)
        return;

    __sync_sub_and_fetch (& misc_bytes_allocated, m_size);

    discard (-1, erase_func);

    free (m_data);
    m_data = nullptr;
    m_size = 0;
}

EXPORT void RingBufBase::add (int len)
{
    assert (len >= 0 && m_len + len <= m_size);
    m_len += len;
}

EXPORT void RingBufBase::remove (int len)
{
    assert (len >= 0 && len <= m_len);

    if (len == m_len)
        m_offset = m_len = 0;
    else
    {
        m_offset = (m_offset + len) % m_size;
        m_len -= len;
    }
}

EXPORT void RingBufBase::copy_in (const void * from, int len, aud::CopyFunc copy_func)
{
    Areas areas;
    add (len);
    get_areas (m_len - len, len, areas);

    if (copy_func)
    {
        copy_func (from, areas.area1, areas.len1);
        copy_func ((const char *) from + areas.len1, areas.area2, areas.len2);
    }
    else
    {
        memcpy (areas.area1, from, areas.len1);
        memcpy (areas.area2, (const char *) from + areas.len1, areas.len2);
    }
}

EXPORT void RingBufBase::move_in (void * from, int len, aud::FillFunc fill_func)
{
    Areas areas;
    add (len);
    get_areas (m_len - len, len, areas);

    memcpy (areas.area1, from, areas.len1);
    memcpy (areas.area2, (const char *) from + areas.len1, areas.len2);

    if (fill_func)
        fill_func (from, len);
}

EXPORT void RingBufBase::move_out (void * to, int len, aud::EraseFunc erase_func)
{
    Areas areas;
    get_areas (0, len, areas);

    if (erase_func)
        erase_func (to, len);

    memcpy (to, areas.area1, areas.len1);
    memcpy ((char *) to + areas.len1, areas.area2, areas.len2);

    remove (len);
}

EXPORT void RingBufBase::discard (int len, aud::EraseFunc erase_func)
{
    if (! m_data)
        return;

    if (len < 0)
        len = m_len;

    if (erase_func)
    {
        Areas areas;
        get_areas (0, len, areas);
        erase_func (areas.area1, areas.len1);
        erase_func (areas.area2, areas.len2);
    }

    remove (len);
}

EXPORT void RingBufBase::move_in (IndexBase & index, int from, int len)
{
    assert (from >= 0 && from <= index.len ());
    assert (len <= index.len () - from);

    if (len < 0)
        len = index.len () - from;

    move_in ((char *) index.begin () + from, len, nullptr);
    index.remove (from, len, nullptr);
}

EXPORT void RingBufBase::move_out (IndexBase & index, int to, int len)
{
    assert (len <= m_len);

    if (len < 0)
        len = m_len;

    void * ptr = index.insert (to, len);
    move_out (ptr, len, nullptr);
}
