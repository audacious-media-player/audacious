/*
 * index.cc
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

#include "index.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>  /* for g_qsort_with_data */

#include "objects.h"

EXPORT void IndexBase::clear (EraseFunc erase_func)
{
    if (erase_func)
        erase_func (m_data, m_len);

    free (m_data);

    m_data = nullptr;
    m_len = 0;
    m_size = 0;
}

EXPORT void * IndexBase::insert (int pos, int len)
{
    assert (pos <= m_len);
    assert (len >= 0);

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

        m_data = new_data;
        m_size = new_size;
    }

    memmove ((char *) m_data + pos + len, (char *) m_data + pos, m_len - pos);
    m_len += len;

    return (char *) m_data + pos;
}

EXPORT void IndexBase::insert (int pos, int len, FillFunc fill_func)
{
    void * to = insert (pos, len);

    if (fill_func)
        fill_func (to, len);
    else
        memset (to, 0, len);
}

EXPORT void IndexBase::insert (const void * from, int pos, int len, CopyFunc copy_func)
{
    void * to = insert (pos, len);

    if (copy_func)
        copy_func (from, to, len);
    else
        memcpy (to, from, len);
}

EXPORT void IndexBase::remove (int pos, int len, EraseFunc erase_func)
{
    assert (pos >= 0 && pos <= m_len);
    assert (len <= m_len - pos);

    if (len < 0)
        len = m_len - pos;  /* remove all following */

    if (erase_func)
        erase_func ((char *) m_data + pos, len);
    memmove ((char *) m_data + pos, (char *) m_data + pos + len, m_len - pos - len);
    memset ((char *) m_data + m_len - len, 0, len);
    m_len -= len;
}

EXPORT void IndexBase::erase (int pos, int len, EraseFunc erase_func)
{
    assert (pos >= 0 && pos <= m_len);
    assert (len <= m_len - pos);

    if (len < 0)
        len = m_len - pos;  /* erase all following */

    if (erase_func)
        erase_func ((char *) m_data + pos, len);
    memset ((char *) m_data + pos, 0, len);
}

EXPORT void IndexBase::shift (int from, int to, int len, EraseFunc erase_func)
{
    assert (len >= 0 && len <= m_len);
    assert (from >= 0 && from + len <= m_len);
    assert (to >= 0 && to + len <= m_len);

    int erase_len = aud::min (len, abs (to - from));

    if (erase_func)
    {
        if (to < from)
            erase_func ((char *) m_data + to, erase_len);
        else
            erase_func ((char *) m_data + to + len - erase_len, erase_len);
    }

    memmove ((char *) m_data + to, (char *) m_data + from, len);

    if (to < from)
        memset ((char *) m_data + from + len - erase_len, 0, erase_len);
    else
        memset ((char *) m_data + from, 0, erase_len);
}

EXPORT void IndexBase::move_from (IndexBase & b, int from, int to, int len,
 bool expand, bool collapse, EraseFunc erase_func)
{
    assert (this != & b);
    assert (from >= 0 && from <= b.m_len);
    assert (len <= b.m_len - from);

    if (len < 0)
        len = b.m_len - from;  /* copy all following */

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
        erase (to, len, erase_func);
    }

    memcpy ((char *) m_data + to, (char *) b.m_data + from, len);

    if (collapse)
    {
        memmove ((char *) b.m_data + from, (char *) b.m_data + from + len, b.m_len - from - len);
        memset ((char *) b.m_data + b.m_len - len, 0, len);
        b.m_len -= len;
    }
    else
        memset ((char *) b.m_data + from, 0, len);
}

EXPORT void IndexBase::sort (CompareFunc compare, int elemsize, void * userdata)
{
    g_qsort_with_data (m_data, m_len / elemsize, elemsize, compare, userdata);
}
