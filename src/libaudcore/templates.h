/*
 * templates.h
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

#ifndef LIBAUDCORE_TEMPLATES_H
#define LIBAUDCORE_TEMPLATES_H

#include <new>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#undef min
#undef max
#endif

namespace aud {

// Utility functions.

template<class T>
constexpr T min (T a, T b)
    { return a < b ? a : b; }

template<class T>
constexpr T max (T a, T b)
    { return a > b ? a : b; }

template<class T>
constexpr T clamp (T x, T low, T high)
    { return min (max (x, low), high); }

template<class T>
constexpr T abs (T x)
    { return x < 0 ? -x : x; }

template<class T>
constexpr T chsign (T x, T s)
    { return (x < 0) ^ (s < 0) ? -x : x; }

template<class T>
constexpr T rdiv (T x, T y)
    { return (x + chsign (y / 2, x)) / y; }

template<class T>
constexpr T rescale (T x, T old_scale, T new_scale)
    { return rdiv (x * new_scale, old_scale); }

template<class T, int N>
constexpr int n_elems (const T (&) [N])
    { return N; }

// Wrapper class allowing enumerations to be used in range-based for loops.

template<class T, T first, T last>
struct range {
    struct iter {
        T v;
        T operator* ()
            { return v; }
        void operator++ ()
            { v = (T) (v + 1); }
        bool operator!= (iter other)
            { return v != other.v; }
    };
    static iter begin () { return {first}; }
    static iter end () { return {(T) (last + 1)}; }
};

// This is a replacement for std::allocator::construct, also supporting
// aggregate initialization.  For background, see:
//     http://cplusplus.github.io/LWG/lwg-active.html#2089

// class constructor proxy
template<class T, bool aggregate>
struct construct_base {
    template<class ... Args>
    static T * make (void * loc, Args && ... args)
        { return new (loc) T (std::forward<Args> (args) ...); }
};

// aggregate constructor proxy
template<class T>
struct construct_base<T, true> {
    template<class ... Args>
    static T * make (void * loc, Args && ... args)
        { return new (loc) T {std::forward<Args> (args) ...}; }
};

// generic constructor proxy
template<class T>
struct construct {
    template<class ... Args>
    static T * make (void * loc, Args && ... args)
    {
        constexpr bool aggregate = ! std::is_constructible<T, Args && ...>::value;
        return construct_base<T, aggregate>::make (loc, std::forward<Args> (args) ...);
    }
};

// Functions for creating/copying/destroying objects en masse.
// These will be nullptr for basic types (use memset/memcpy instead).

typedef void (* FillFunc) (void * data, int len);
typedef void (* CopyFunc) (const void * from, void * to, int len);
typedef void (* EraseFunc) (void * data, int len);

template<class T>
static constexpr FillFunc fill_func ()
{
    return std::is_trivial<T>::value ? (FillFunc) nullptr :
     [] (void * data, int len) {
        T * iter = (T *) data;
        T * end = (T *) ((char *) data + len);
        while (iter < end)
            new (iter ++) T ();
    };
}

template<class T>
static constexpr CopyFunc copy_func ()
{
    return std::is_trivial<T>::value ? (CopyFunc) nullptr :
     [] (const void * from, void * to, int len) {
        const T * src = (const T *) from;
        T * dest = (T *) to;
        T * end = (T *) ((char *) to + len);
        while (dest < end)
            new (dest ++) T (* src ++);
    };
}

template<class T>
static constexpr EraseFunc erase_func ()
{
    return std::is_trivial<T>::value ? (EraseFunc) nullptr :
     [] (void * data, int len) {
        T * iter = (T *) data;
        T * end = (T *) ((char *) data + len);
        while (iter < end)
            (* iter ++).~T ();
    };
}

} // namespace aud

#endif // LIBAUDCORE_TEMPLATES_H
