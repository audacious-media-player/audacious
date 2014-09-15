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

#include <libaudcore/templates.h>

/*
 * Index is a lightweight list class similar to std::vector, but with the
 * following differences:
 *  - The base implementation is type-agnostic, so it only needs to be compiled
 *    once.  Type-safety is provided by a thin template subclass.
 *  - Objects are moved in memory without calling any assignment operator.
 *    Be careful to use only objects that can handle this.
 */

class IndexBase
{
public:
    typedef void (* FillFunc) (void * data, int len);
    typedef void (* CopyFunc) (const void * from, void * to, int len);
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

    void * insert (int pos, int len);
    void insert (int pos, int len, FillFunc fill_func);
    void insert (const void * from, int pos, int len, CopyFunc copy_func);
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
        { IndexBase::clear (erase_func ()); }
    ~Index ()
        { clear (); }

    Index (Index && b) :
        IndexBase (std::move (b)) {}
    void operator= (Index && b)
        { steal (std::move (b), erase_func ()); }

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
        { IndexBase::insert (raw (pos), raw (len), fill_func ()); }
    void insert (const T * from, int pos, int len)
        { IndexBase::insert (from, raw (pos), raw (len), copy_func ()); }
    void remove (int pos, int len)
        { IndexBase::remove (raw (pos), raw (len), erase_func ()); }
    void erase (int pos, int len)
        { IndexBase::erase (raw (pos), raw (len), erase_func ()); }
    void shift (int from, int to, int len)
        { IndexBase::shift (raw (from), raw (to), raw (len), erase_func ()); }

    void move_from (Index<T> & b, int from, int to, int len, bool expand, bool collapse)
        { IndexBase::move_from (b, raw (from), raw (to), raw (len), expand,
           collapse, erase_func ()); }

    template<typename ... Args>
    T & append (Args && ... args)
    {
        return * aud::construct<T>::make (IndexBase::insert (-1, sizeof (T)),
         std::forward<Args> (args) ...);
    }

    int find (const T & val)
    {
        for (const T * iter = begin (); iter != end (); iter ++)
        {
            if (* iter == val)
                return iter - begin ();
        }

        return -1;
    }

    void sort (CompareFunc compare, void * userdata)
    {
        struct state_t {
            CompareFunc compare;
            void * userdata;
        };

        auto wrapper = [] (const void * a, const void * b, void * userdata) -> int
        {
            auto state = (const state_t *) userdata;
            return state->compare (* (const T *) a, * (const T *) b, state->userdata);
        };

        const state_t state = {compare, userdata};
        IndexBase::sort (wrapper, sizeof (T), (void *) & state);
    }

private:
    static constexpr int raw (int len)
        { return len * sizeof (T); }
    static constexpr int cooked (int len)
        { return len / sizeof (T); }

    static constexpr FillFunc fill_func ()
    {
        return std::is_trivial<T>::value ? (FillFunc) nullptr : [] (void * data, int len)
        {
            T * iter = (T *) data;
            T * end = (T *) ((char *) data + len);
            while (iter < end)
                new (iter ++) T ();
        };
    }

    static constexpr CopyFunc copy_func ()
    {
        return std::is_trivial<T>::value ? (CopyFunc) nullptr : [] (const void * from, void * to, int len)
        {
            const T * src = (const T *) from;
            T * dest = (T *) to;
            T * end = (T *) ((char *) to + len);
            while (dest < end)
                new (dest ++) T (* src ++);
        };
    }

    static constexpr EraseFunc erase_func ()
    {
        return std::is_trivial<T>::value ? (EraseFunc) nullptr : [] (void * data, int len)
        {
            T * iter = (T *) data;
            T * end = (T *) ((char *) data + len);
            while (iter < end)
                (* iter ++).~T ();
        };
    }
};

#endif // LIBAUDCORE_INDEX_H
