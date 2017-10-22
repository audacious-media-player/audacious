/*
 * index.h
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
    typedef int (* CompareFunc) (const void * a, const void * b, void * userdata);

    constexpr IndexBase () :
        m_data (nullptr),
        m_len (0),
        m_size (0) {}

    void clear (aud::EraseFunc erase_func);  // use as destructor

    IndexBase (IndexBase && b) :
        m_data (b.m_data),
        m_len (b.m_len),
        m_size (b.m_size)
    {
        b.m_data = nullptr;
        b.m_len = 0;
        b.m_size = 0;
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

    void * insert (int pos, int len);  // no fill
    void insert (int pos, int len, aud::FillFunc fill_func);
    void insert (const void * from, int pos, int len, aud::CopyFunc copy_func);
    void remove (int pos, int len, aud::EraseFunc erase_func);
    void erase (int pos, int len, aud::FillFunc fill_func, aud::EraseFunc erase_func);
    void shift (int from, int to, int len, aud::FillFunc fill_func, aud::EraseFunc erase_func);

    void move_from (IndexBase & b, int from, int to, int len, bool expand,
     bool collapse, aud::FillFunc fill_func, aud::EraseFunc erase_func);

    void sort (CompareFunc compare, int elemsize, void * userdata);
    int bsearch (const void * key, CompareFunc search, int elemsize, void * userdata) const;

private:
    void * m_data;
    int m_len, m_size;
};

template<class T>
class Index : private IndexBase
{
private:
    // provides C-style callback to generic comparison functor
    template<class Key, class F>
    struct WrapCompare {
        static int run (const void * key, const void * val, void * func)
            { return (* (F *) func) (* (const Key *) key, * (const T *) val); }
    };

public:
    constexpr Index () :
        IndexBase () {}

    // use with care!
    IndexBase & base ()
        { return * this; }

    void clear ()
        { IndexBase::clear (aud::erase_func<T> ()); }
    ~Index ()
        { clear (); }

    Index (Index && b) :
        IndexBase (std::move (b)) {}
    Index & operator= (Index && b)
        { return aud::move_assign (* this, std::move (b)); }

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
        { IndexBase::insert (raw (pos), raw (len), aud::fill_func<T> ()); }
    void insert (const T * from, int pos, int len)
        { IndexBase::insert (from, raw (pos), raw (len), aud::copy_func<T> ()); }
    void remove (int pos, int len)
        { IndexBase::remove (raw (pos), raw (len), aud::erase_func<T> ()); }
    void erase (int pos, int len)
        { IndexBase::erase (raw (pos), raw (len), aud::fill_func<T> (), aud::erase_func<T> ()); }
    void shift (int from, int to, int len)
        { IndexBase::shift (raw (from), raw (to), raw (len), aud::fill_func<T> (), aud::erase_func<T> ()); }

    void move_from (Index<T> & b, int from, int to, int len, bool expand, bool collapse)
        { IndexBase::move_from (b, raw (from), raw (to), raw (len), expand,
           collapse, aud::fill_func<T> (), aud::erase_func<T> ()); }

    template<class ... Args>
    T & append (Args && ... args)
    {
        return * aud::construct<T>::make (IndexBase::insert (-1, sizeof (T)),
         std::forward<Args> (args) ...);
    }

    int find (const T & val) const
    {
        for (const T * iter = begin (); iter != end (); iter ++)
        {
            if (* iter == val)
                return iter - begin ();
        }

        return -1;
    }

    // func(val) returns true to remove val, false to keep it
    template<class F>
    bool remove_if (F func, bool clear_if_empty = false)
    {
        T * iter = begin ();
        bool changed = false;
        while (iter != end ())
        {
            if (func (* iter))
            {
                remove (iter - begin (), 1);
                changed = true;
            }
            else
                iter ++;
        }

        if (clear_if_empty && ! len ())
            clear ();

        return changed;
    }

    // compare(a, b) returns <0 if a<b, 0 if a=b, >0 if a>b
    template<class F>
    void sort (F compare)
        { IndexBase::sort (WrapCompare<T, F>::run, sizeof (T), & compare); }

    // compare(key, val) returns <0 if key<val, 0 if key=val, >0 if key>val
    template<class Key, class F>
    int bsearch (const Key & key, F compare)
        { return IndexBase::bsearch (& key, WrapCompare<Key, F>::run, sizeof (T), & compare); }

    // for use of Index as a raw data buffer
    // unlike insert(), does not zero-fill any added space
    void resize (int size)
    {
        static_assert (std::is_trivial<T>::value, "for basic types only");
        int diff = size - len ();
        if (diff > 0)
            IndexBase::insert (-1, raw (diff));
        else if (diff < 0)
            IndexBase::remove (raw (size), -1, nullptr);
    }

private:
    static constexpr int raw (int len)
        { return len * sizeof (T); }
    static constexpr int cooked (int len)
        { return len / sizeof (T); }
};

#endif // LIBAUDCORE_INDEX_H
