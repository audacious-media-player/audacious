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

// #undef POSIX and Windows macros to avoid name conflicts
#undef abs
#undef min
#undef max

namespace aud {

// Utility functions
// =================

// minimum of two numbers
template<class T>
constexpr T min (T a, T b)
    { return a < b ? a : b; }

// maximum of two numbers
template<class T>
constexpr T max (T a, T b)
    { return a > b ? a : b; }

// make sure a number is within the given range
template<class T>
constexpr T clamp (T x, T low, T high)
    { return min (max (x, low), high); }

// absolute value
template<class T>
constexpr T abs (T x)
    { return x < 0 ? -x : x; }

// change the sign of x to the sign of s
template<class T>
constexpr T chsign (T x, T s)
    { return (x < 0) ^ (s < 0) ? -x : x; }

// integer division with rounding
template<class T>
constexpr T rdiv (T x, T y)
    { return (x + chsign (y / 2, x)) / y; }

// convert integer from one scale to another, with rounding
template<class T>
constexpr T rescale (T x, T old_scale, T new_scale)
    { return rdiv (x * new_scale, old_scale); }

// number of characters needed to represent an integer (including minus sign)
template<class T>
constexpr T n_digits (T x)
    { return x < 0 ? 1 + n_digits (-x) : x < 10 ? 1 : 1 + n_digits (x / 10); }

// number of elements in an array
template<class T, int N>
constexpr int n_elems (const T (&) [N])
    { return N; }

// Casts for storing various data in a void pointer
// ================================================

template<class T>
inline void * to_ptr (T t)
{
    union { void * v; T t; } u = {nullptr};
    static_assert (sizeof u == sizeof (void *), "Type cannot be stored in a pointer");
    u.t = t; return u.v;
}

template<class T>
inline T from_ptr (void * v)
{
    union { void * v; T t; } u = {v};
    static_assert (sizeof u == sizeof (void *), "Type cannot be stored in a pointer");
    return u.t;
}

// Move-assignment implemented via move-constructor
// ================================================

template<class T>
T & move_assign (T & a, T && b)
{
    if (& a != & b)
    {
        a.~T ();
        new (& a) T (std::move (b));
    }
    return a;
}

// Function wrappers (or "casts") for interaction with C-style APIs
// ================================================================

template<class T>
void delete_obj (void * obj)
    { (void) sizeof (T); delete (T *) obj; }

template<class T>
void delete_typed (T * obj)
    { (void) sizeof (T); delete obj; }

template<class T, void (* func) (void *)>
void typed_func (T * obj)
    { func (obj); }

template<class T, void (T::* func) ()>
static void obj_member (void * obj)
    { (((T *) obj)->* func) (); }

// Wrapper class allowing enumerations to be used as array indexes;
// the enumeration must begin with zero and have a "count" constant
// ================================================================

template<class T, class V>
struct array
{
    // cannot use std::forward here; it is not constexpr until C++14
    template<class ... Args>
    constexpr array (Args && ... args) :
        vals { static_cast<Args &&> (args) ...} {}

    // Due to GCC bug #63707, the forwarding constructor given above cannot be
    // used to initialize the array when V is a class with no copy constructor.
    // As a very limited workaround, provide a second constructor which can be
    // used to initialize the array to default values in this case.
    constexpr array () :
        vals () {}

    constexpr const V & operator[] (T t) const
        { return vals[(int) t]; }
    constexpr const V * begin () const
        { return vals; }
    constexpr const V * end () const
        { return vals + (int) T::count; }
    V & operator[] (T t)
        { return vals[(int) t]; }
    V * begin ()
        { return vals; }
    V * end ()
        { return vals + (int) T::count; }

private:
    V vals[(int) T::count];
};

// Wrapper class allowing enumerations to be used in range-based for loops
// =======================================================================

template<class T, T first = (T) 0, T last = (T) ((int) T::count - 1)>
struct range
{
    struct iter {
        T v;
        constexpr T operator* () const
            { return v; }
        constexpr bool operator!= (iter other) const
            { return v != other.v; }
        void operator++ ()
            { v = (T) ((int) v + 1); }
    };

    static constexpr iter begin ()
        { return {first}; }
    static constexpr iter end ()
        { return {(T) ((int) last + 1)}; }
};

// Replacement for std::allocator::construct, which also supports aggregate
// initialization.  For background, see:
//     http://cplusplus.github.io/LWG/lwg-active.html#2089
// ========================================================================

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

// Convert an integer constant to a string at compile-time; can be used for
// #defines, enums, constexpr calculations, etc.
// ========================================================================

// "metaprogramming" string type: each different string is a unique type
template<char... args>
struct metastring {
    char data[sizeof... (args) + 1] = {args..., '\0'};
};

// recursive number-printing template, general case (three or more digits)
template<int size, int x, char... args>
struct numeric_builder {
    typedef typename numeric_builder<size - 1, x / 10, '0' + abs (x) % 10, args...>::type type;
};

// special case for two digits; minus sign is handled here
template<int x, char... args>
struct numeric_builder<2, x, args...> {
    typedef metastring<x < 0 ? '-' : '0' + x / 10, '0' + abs (x) % 10, args...> type;
};

// special case for one digit (positive numbers only)
template<int x, char... args>
struct numeric_builder<1, x, args...> {
    typedef metastring<'0' + x, args...> type;
};

// convenience wrapper for numeric_builder
template<int x>
class numeric_string
{
private:
    // generate a unique string type representing this number
    typedef typename numeric_builder<n_digits (x), x>::type type;

    // declare a static string of that type (instantiated later at file scope)
    static constexpr type value {};

public:
    // pointer to the instantiated string
    static constexpr const char * str = value.data;
};

// instantiate numeric_string::value as needed for different numbers
template<int x>
constexpr typename numeric_string<x>::type numeric_string<x>::value;

// Functions for creating/copying/destroying objects en masse;
// these will be nullptr for basic types (use memset/memcpy instead)
// =================================================================

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
