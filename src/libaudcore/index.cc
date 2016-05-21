/*
 * index.cc
 * Copyright 2014-2016 John Lindgren
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

#include "index.h"
#include "internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>  /* for g_qsort_with_data */

static void do_fill (void * data, int len, aud::FillFunc fill_func)
{
    if (fill_func)
        fill_func (data, len);
    else
        memset (data, 0, len);
}

static void do_erase (void * data, int len, aud::EraseFunc erase_func)
{
    if (erase_func)
        erase_func (data, len);
}

EXPORT void IndexBase::clear (aud::EraseFunc erase_func)
{
    if (! m_data)
        return;

    __sync_sub_and_fetch (& misc_bytes_allocated, m_size);

    do_erase (m_data, m_len, erase_func);
    free (m_data);

    m_data = nullptr;
    m_len = 0;
    m_size = 0;
}

EXPORT void * IndexBase::insert (int pos, int len)
{
    assert (pos <= m_len);
    assert (len >= 0);

    if (! len)
        goto out;

    if (pos < 0)
        pos = m_len;  /* insert at end */

    if (m_size < m_len + len)
    {
        /* never allocate less than 16 bytes */
        int new_size = aud::max (m_size, 16);

        /* next try 4/3 current size, biased toward multiples of 4 */
        if (new_size < m_len + len)
            new_size = (new_size + 2) / 3 * 4;

        /* use requested size if still too small */
        if (new_size < m_len + len)
            new_size = m_len + len;

        void * new_data = realloc (m_data, new_size);
        if (! new_data)
            throw std::bad_alloc ();  /* nothing changed yet */

        __sync_add_and_fetch (& misc_bytes_allocated, new_size - m_size);

        m_data = new_data;
        m_size = new_size;
    }

    memmove ((char *) m_data + pos + len, (char *) m_data + pos, m_len - pos);
    m_len += len;

out:
    return (char *) m_data + pos;
}

EXPORT void IndexBase::insert (int pos, int len, aud::FillFunc fill_func)
{
    void * to = insert (pos, len);

    if (! len)
        return;

    if (fill_func)
        fill_func (to, len);
    else
        memset (to, 0, len);
}

EXPORT void IndexBase::insert (const void * from, int pos, int len, aud::CopyFunc copy_func)
{
    void * to = insert (pos, len);

    if (! len)
        return;

    if (copy_func)
        copy_func (from, to, len);
    else
        memcpy (to, from, len);
}

EXPORT void IndexBase::remove (int pos, int len, aud::EraseFunc erase_func)
{
    assert (pos >= 0 && pos <= m_len);
    assert (len <= m_len - pos);

    if (len < 0)
        len = m_len - pos;  /* remove all following */

    if (! len)
        return;

    do_erase ((char *) m_data + pos, len, erase_func);
    memmove ((char *) m_data + pos, (char *) m_data + pos + len, m_len - pos - len);
    m_len -= len;
}

EXPORT void IndexBase::erase (int pos, int len, aud::FillFunc fill_func, aud::EraseFunc erase_func)
{
    assert (pos >= 0 && pos <= m_len);
    assert (len <= m_len - pos);

    if (len < 0)
        len = m_len - pos;  /* erase all following */

    if (! len)
        return;

    do_erase ((char *) m_data + pos, len, erase_func);
    do_fill ((char *) m_data + pos, len, fill_func);
}

EXPORT void IndexBase::shift (int from, int to, int len, aud::FillFunc fill_func, aud::EraseFunc erase_func)
{
    assert (len >= 0 && len <= m_len);
    assert (from >= 0 && from + len <= m_len);
    assert (to >= 0 && to + len <= m_len);

    if (! len)
        return;

    int erase_len = aud::min (len, abs (to - from));

    if (to < from)
        do_erase ((char *) m_data + to, erase_len, erase_func);
    else
        do_erase ((char *) m_data + to + len - erase_len, erase_len, erase_func);

    memmove ((char *) m_data + to, (char *) m_data + from, len);

    if (to < from)
        do_fill ((char *) m_data + from + len - erase_len, erase_len, fill_func);
    else
        do_fill ((char *) m_data + from, erase_len, fill_func);
}

EXPORT void IndexBase::move_from (IndexBase & b, int from, int to, int len,
 bool expand, bool collapse, aud::FillFunc fill_func, aud::EraseFunc erase_func)
{
    assert (this != & b);
    assert (from >= 0 && from <= b.m_len);
    assert (len <= b.m_len - from);

    if (len < 0)
        len = b.m_len - from;  /* copy all following */

    if (! len)
        return;

    if (expand)
    {
        assert (to <= m_len);
        if (to < 0)
            to = m_len;  /* insert at end */

        insert (to, len);
    }
    else
    {
        assert (to >= 0 && to <= m_len - len);
        do_erase ((char *) m_data + to, len, erase_func);
    }

    memcpy ((char *) m_data + to, (char *) b.m_data + from, len);

    if (collapse)
    {
        memmove ((char *) b.m_data + from, (char *) b.m_data + from + len, b.m_len - from - len);
        b.m_len -= len;
    }
    else
        do_fill ((char *) b.m_data + from, len, fill_func);
}

EXPORT void IndexBase::sort (CompareFunc compare, int elemsize, void * userdata)
{
    if (! m_len)
        return;

    // since we require GLib >= 2.32, g_qsort_with_data performs a stable sort
    g_qsort_with_data (m_data, m_len / elemsize, elemsize, compare, userdata);
}

EXPORT int IndexBase::bsearch (const void * key, CompareFunc compare,
 int elemsize, void * userdata) const
{
    int top = 0;
    int bottom = m_len / elemsize;

    while (top < bottom)
    {
        int middle = top + (bottom - top) / 2;
        int match = compare (key, (char *) m_data + middle * elemsize, userdata);

        if (match < 0)
            bottom = middle;
        else if (match > 0)
            top = middle + 1;
        else
            return middle;
    }

    return -1;
}
