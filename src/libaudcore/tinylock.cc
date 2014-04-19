/*
 * tinylock.c
 * Copyright 2013 John Lindgren
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

#include "tinylock.h"

#ifndef VALGRIND_FRIENDLY

#include <limits.h>
#include <sched.h>

#define WRITE_BIT (SHRT_MAX + 1)

EXPORT void tiny_lock (TinyLock * lock)
{
    while (__builtin_expect (__sync_lock_test_and_set (lock, 1), 0))
        sched_yield ();
}

EXPORT void tiny_unlock (TinyLock * lock)
{
    __sync_lock_release (lock);
}

EXPORT void tiny_lock_read (TinyRWLock * lock)
{
    while (__builtin_expect (__sync_fetch_and_add (lock, 1) & WRITE_BIT, 0))
    {
        __sync_fetch_and_sub (lock, 1);
        sched_yield ();
    }
}

EXPORT void tiny_unlock_read (TinyRWLock * lock)
{
    __sync_fetch_and_sub (lock, 1);
}

EXPORT void tiny_lock_write (TinyRWLock * lock)
{
    while (! __builtin_expect (__sync_bool_compare_and_swap (lock, 0, WRITE_BIT), 1))
        sched_yield ();
}

EXPORT void tiny_unlock_write (TinyRWLock * lock)
{
    __sync_fetch_and_sub (lock, WRITE_BIT);
}

#endif /* ! VALGRIND_FRIENDLY */
