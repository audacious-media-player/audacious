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
constexpr T rescale (T x, T old_scale, T new_scale)
    { return (x * new_scale + old_scale / 2) / old_scale; }

template<class T, int N>
constexpr int n_elems(const T (&) [N])
    { return N; }

// This is a replacement for std::allocator::construct, also supporting
// aggregate initialization.  For background, see:
//     http://cplusplus.github.io/LWG/lwg-active.html#2089

// class constructor proxy
template<typename T, bool aggregate>
struct construct_base {
    template<typename ... Args>
    static T * make (void * loc, Args && ... args)
        { return new (loc) T (std::forward<Args> (args) ...); }
};

// aggregate constructor proxy
template<typename T>
struct construct_base<T, true> {
    template<typename ... Args>
    static T * make (void * loc, Args && ... args)
        { return new (loc) T {std::forward<Args> (args) ...}; }
};

// generic constructor proxy
template<typename T>
struct construct {
    template<typename ... Args>
    static T * make (void * loc, Args && ... args)
    {
        constexpr bool aggregate = ! std::is_constructible<T, Args && ...>::value;
        return construct_base<T, aggregate>::make (loc, std::forward<Args> (args) ...);
    }
};

} // namespace aud

#endif // LIBAUDCORE_TEMPLATES_H
