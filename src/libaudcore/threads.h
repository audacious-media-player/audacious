/*
 * threads.h
 * Copyright 2019 John Lindgren
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

#ifndef LIBAUDCORE_THREADS_H
#define LIBAUDCORE_THREADS_H

#include <libaudcore/templates.h>
#include <libaudcore/tinylock.h>

#include <condition_variable>
#include <mutex>
#include <thread>

namespace aud
{

/* A wrapper class around TinyLock, encouraging correct usage */
class spinlock
{
public:
    spinlock() = default;

    /* Not copyable or movable */
    spinlock(const spinlock &) = delete;
    spinlock & operator=(const spinlock &) = delete;

    /* Explicit lock/unlock */
    void lock() { tiny_lock(&m_lock); }
    void unlock() { tiny_unlock(&m_lock); }

    /* Scope-based lock ownership */
    typedef owner<spinlock, &spinlock::lock, &spinlock::unlock> holder;
    /* Convenience method for taking ownership of the lock */
    holder take() __attribute__((warn_unused_result)) { return holder(this); }

private:
    TinyLock m_lock = 0;
};

/* A wrapper class around TinyRWLock, encouraging correct usage */
class spinlock_rw
{
public:
    spinlock_rw() = default;

    /* Not copyable or movable */
    spinlock_rw(const spinlock_rw &) = delete;
    spinlock_rw & operator=(const spinlock_rw &) = delete;

    /* Explicit lock/unlock */
    void lock_r() { tiny_lock_read(&m_lock); }
    void unlock_r() { tiny_unlock_read(&m_lock); }
    void lock_w() { tiny_lock_write(&m_lock); }
    void unlock_w() { tiny_unlock_write(&m_lock); }

    /* Scope-based lock ownership */
    typedef owner<spinlock_rw, &spinlock_rw::lock_r, &spinlock_rw::unlock_r>
        reader;
    typedef owner<spinlock_rw, &spinlock_rw::lock_w, &spinlock_rw::unlock_w>
        writer;
    /* Convenience methods for taking ownership of the lock */
    reader read() __attribute__((warn_unused_result)) { return reader(this); }
    writer write() __attribute__((warn_unused_result)) { return writer(this); }

private:
    TinyRWLock m_lock = 0;
};

/* An alias for std::mutex */
class mutex : public std::mutex
{
public:
    /* Scope-based lock ownership */
    typedef std::unique_lock<std::mutex> holder;
    /* Convenience method for taking ownership of the lock */
    holder take() __attribute__((warn_unused_result)) { return holder(*this); }
};

/* An alias for std::condition_variable */
typedef std::condition_variable condvar;

} // namespace aud

#endif // LIBAUDCORE_THREADS_H
