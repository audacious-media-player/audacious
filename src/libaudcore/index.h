/*
 * index.h
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

#ifndef LIBAUDCORE_INDEX_H
#define LIBAUDCORE_INDEX_H

#include <type_traits>
#include <utility>

/*
 * Index is a lightweight list class similar to std::vector, but with the
 * following differences:
 *  - The base implementation is type-agnostic, so it only needs to be compiled
 *    once.  Type-safety is provided by a thin template subclass.
 *  - Insertion of new objects into the list initializes them to zero; no
 *    constructors are called.  Be careful to use only objects that can handle
 *    this.
 *  - Objects are moved in memory without calling any assignment operator.
 *    Again, be careful to use only objects that can handle this.
 */

class IndexBase
{
public:
    typedef void (* EraseFunc) (void * data, int len);
    typedef int (* CompareFunc) (const void * a, const void * b, void * userdata);

    constexpr IndexBase () :
        m_data (nullptr),
        m_len (0),
        m_size (0) {}

    void clear (EraseFunc erase_func);  // use as destructor

    IndexBase (IndexBase && b) :
        m_data (b.m_data),
        m_len (b.m_len),
        m_size (b.m_size)
    {
        b.m_data = nullptr;
        b.m_len = 0;
        b.m_size = 0;
    }

    void steal (IndexBase && b, EraseFunc erase_func)
    {
        if (this != & b)
        {
            clear (erase_func);
            m_data = b.m_data;
            m_len = b.m_len;
            m_size = b.m_size;
            b.m_data = nullptr;
            b.m_len = 0;
            b.m_size = 0;
        }
    }

    void * begin ()
        { return m_data; }
    const void * begin () const
        { return m_data; }
    void * end ()
        { return (char *) m_data + m_len; }
    const void * end () const
        { return (char *) m_data + m_len; }

    int len () const
        { return m_len; }

    void insert (int pos, int len);
    void remove (int pos, int len, EraseFunc erase_func);
    void erase (int pos, int len, EraseFunc erase_func);
    void shift (int from, int to, int len, EraseFunc erase_func);

    void move_from (IndexBase & b, int from, int to, int len, bool expand,
     bool collapse, EraseFunc erase_func);

    void sort (CompareFunc compare, int elemsize, void * userdata);

private:
    void * m_data;
    int m_len, m_size;
};

template<class T>
class Index : private IndexBase
{
public:
    typedef int (* CompareFunc) (const T & a, const T & b, void * userdata);

    constexpr Index () :
        IndexBase () {}

    void clear ()
        { IndexBase::clear (erase_func); }
    ~Index ()
        { clear (); }

    Index (Index && b) :
        IndexBase (std::move (b)) {}
    void operator= (Index && b)
        { steal (std::move (b), erase_func); }

    T * begin ()
        { return (T *) IndexBase::begin (); }
    const T * begin () const
        { return (const T *) IndexBase::begin (); }
    T * end ()
        { return (T *) IndexBase::end (); }
    const T * end () const
        { return (const T *) IndexBase::end (); }

    int len () const
        { return cooked (IndexBase::len ()); }

    T & operator[] (int i)
        { return begin ()[i]; }
    const T & operator[] (int i) const
        { return begin ()[i]; }

    void insert (int pos, int len)
        { IndexBase::insert (raw (pos), raw (len)); }
    void remove (int pos, int len)
        { IndexBase::remove (raw (pos), raw (len), erase_func); }
    void erase (int pos, int len)
        { IndexBase::erase (raw (pos), raw (len), erase_func); }
    void shift (int from, int to, int len)
        { IndexBase::shift (raw (from), raw (to), raw (len), erase_func); }

    void move_from (Index<T> & b, int from, int to, int len, bool expand, bool collapse)
        { IndexBase::move_from (b, raw (from), raw (to), raw (len), expand, collapse, erase_func); }

    void sort (CompareFunc compare, void * userdata)
    {
        const CompareData data = {compare, userdata};
        IndexBase::sort (compare_wrapper, sizeof (T), (void *) & data);
    }

    T & append ()
    {
        insert (-1, 1);
        return end ()[-1];
    }

    void append (const T & val)
        { append () = val; }
    void append (T && val)
        { append () = std::move (val); }

private:
    struct CompareData {
        CompareFunc compare;
        void * userdata;
    };

    static constexpr int raw (int len)
        { return len * sizeof (T); }
    static constexpr int cooked (int len)
        { return len / sizeof (T); }

    static void erase_objects (void * data, int len)
    {
        T * iter = (T *) data;
        T * end = (T *) ((char *) data + len);
        while (iter < end)
            (* iter ++).~T ();
    }

    static constexpr EraseFunc erase_func = std::is_trivial<T>::value ? nullptr : erase_objects;

    static int compare_wrapper (const void * a, const void * b, void * userdata)
    {
        const CompareData & data = * (const CompareData *) userdata;
        return data.compare (* (const T *) a, * (const T *) b, data.userdata);
    }
};

#endif // LIBAUDCORE_INDEX_H
