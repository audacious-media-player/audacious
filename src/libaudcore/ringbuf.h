/*
 * ringbuf.h
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

#ifndef LIBAUDCORE_RINGBUF_H
#define LIBAUDCORE_RINGBUF_H

#include <libaudcore/index.h>

/*
 * RingBuf is a lightweight ring buffer class, with the following properties:
 *  - The base implementation is type-agnostic, so it only needs to be compiled
 *    once.  Type-safety is provided by a thin template subclass.
 *  - Objects are moved in memory without calling any assignment operator.
 *    Be careful to use only objects that can handle this.
 *  - The head and tail pointers within the ring buffer stay aligned as one
 *    would expect: that is, if data is only written and read in multiples of n
 *    bytes, and the size of the ring buffer is also a multiple of n bytes, then
 *    no n-byte block will ever wrap around the end of the ring buffer.  The
 *    alignment of the buffer is reset whenever the buffer becomes empty.
 */

class RingBufBase
{
public:
    constexpr RingBufBase () :
        m_data (nullptr),
        m_size (0),
        m_offset (0),
        m_len (0) {}

    RingBufBase (RingBufBase && b) :
        m_data (b.m_data),
        m_size (b.m_size),
        m_offset (b.m_offset),
        m_len (b.m_len)
    {
        b.m_data = nullptr;
        b.m_size = 0;
        b.m_offset = 0;
        b.m_len = 0;
    }

    void steal (RingBufBase && b, aud::EraseFunc erase_func)
    {
        if (this != & b)
        {
            destroy (erase_func);
            new (this) RingBufBase (std::move (b));
        }
    }

    // allocated size of the buffer
    int size () const
        { return m_size; }

    // number of bytes currently used
    int len () const
        { return m_len; }

    // number of bytes that can be read linearly
    int linear () const
        { return aud::min (m_len, m_size - m_offset); }

    void * at (int pos) const
        { return (char *) m_data + (m_offset + pos) % m_size; }

    void alloc (int size);
    void destroy (aud::EraseFunc erase_func);

    void add (int len);  // no fill
    void remove (int len);  // no erase

    void copy_in (const void * from, int len, aud::CopyFunc copy_func);
    void move_in (void * from, int len, aud::FillFunc fill_func);
    void move_out (void * to, int len, aud::EraseFunc erase_func);
    void discard (int len, aud::EraseFunc erase_func);

    void move_in (IndexBase & index, int from, int len);
    void move_out (IndexBase & index, int to, int len);

private:
    struct Areas {
        void * area1, * area2;
        int len1, len2;
    };

    void get_areas (int pos, int len, Areas & areas);
    void do_realloc (int size);

    void * m_data;
    int m_size, m_offset, m_len;
};

template<class T>
class RingBuf : private RingBufBase
{
public:
    constexpr RingBuf () :
        RingBufBase () {}

    ~RingBuf ()
        { destroy (); }

    RingBuf (RingBuf && b) :
        RingBufBase (std::move (b)) {}
    void operator= (RingBuf && b)
        { steal (std::move (b), aud::erase_func<T> ()); }

    int size () const
        { return cooked (RingBufBase::size ()); }
    int len () const
        { return cooked (RingBufBase::len ()); }
    int linear () const
        { return cooked (RingBufBase::linear ()); }
    int space () const
        { return size () - len (); }

    T & operator[] (int i)
        { return * (T *) RingBufBase::at (raw (i)); }
    const T & operator[] (int i) const
        { return * (const T *) RingBufBase::at (raw (i)); }

    void alloc (int size)
        { RingBufBase::alloc (raw (size)); }
    void destroy ()
        { RingBufBase::destroy (aud::erase_func<T> ()); }

    void copy_in (const T * from, int len)
        { RingBufBase::copy_in (from, raw (len), aud::copy_func<T> ()); }
    void move_in (T * from, int len)
        { RingBufBase::move_in (from, raw (len), aud::fill_func<T> ()); }
    void move_out (T * to, int len)
        { RingBufBase::move_out (to, raw (len), aud::erase_func<T> ()); }
    void discard (int len = -1)
        { RingBufBase::discard (raw (len), aud::erase_func<T> ()); }

    void move_in (Index<T> & index, int from, int len)
        { RingBufBase::move_in (index.base (), raw (from), raw (len)); }
    void move_out (Index<T> & index, int to, int len)
        { RingBufBase::move_out (index.base (), raw (to), raw (len)); }

    template<class ... Args>
    T & push (Args && ... args)
    {
        add (raw (1));
        return * aud::construct<T>::make (at (raw (len () - 1)), std::forward<Args> (args) ...);
    }

    T & head ()
        { return * (T *) at (raw (0)); }

    void pop ()
    {
        head ().~T ();
        remove (raw (1));
    }

private:
    static constexpr int raw (int len)
        { return len * sizeof (T); }
    static constexpr int cooked (int len)
        { return len / sizeof (T); }
};

#endif // LIBAUDCORE_RINGBUF_H
