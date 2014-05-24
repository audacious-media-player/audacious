/*
 * stringbuf.cc
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <new>

#include "objects.h"

#ifndef MAP_ANONYMOUS
    #define MAP_ANONYMOUS MAP_ANON
#endif

struct StringStack
{
    static constexpr int Size = 1048576;  // 1 MB

    char * top;
    char buf[Size - sizeof (top)];

    inline int avail (char * pos) const
        { return buf + sizeof buf - pos; }
};

static pthread_key_t key;
static pthread_once_t once = PTHREAD_ONCE_INIT;

static void free_stack (void * stack)
{
    if (stack)
        munmap (stack, sizeof (StringStack));
}

static void make_key ()
{
    pthread_key_create (& key, free_stack);
}

static StringStack * get_stack ()
{
    pthread_once (& once, make_key);

    StringStack * stack = (StringStack *) pthread_getspecific (key);

    if (! stack)
    {
        stack = (StringStack *) mmap (nullptr, sizeof (StringStack),
         PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (stack == MAP_FAILED)
            throw std::bad_alloc ();

        stack->top = stack->buf;
        pthread_setspecific (key, stack);
    }

    return stack;
}

EXPORT void StringBuf::resize (int len)
{
    if (! stack)
    {
        stack = get_stack ();
        m_data = stack->top;
    }
    else
    {
        if (m_data + m_len + 1 != stack->top)
            throw std::bad_alloc ();
    }

    if (len < 0)
    {
        stack->top = stack->buf + sizeof stack->buf;
        m_len = stack->top - m_data - 1;

        if (m_len < 0)
            throw std::bad_alloc ();
    }
    else
    {
        stack->top = m_data + len + 1;

        if (stack->top - stack->buf > (int) sizeof stack->buf)
            throw std::bad_alloc ();

        m_data[len] = 0;
        m_len = len;
    }
}

EXPORT StringBuf::~StringBuf ()
{
    if (m_data)
    {
        if (m_data + m_len + 1 != stack->top)
            throw std::bad_alloc ();

        stack->top = m_data;
    }
}

EXPORT void StringBuf::steal (StringBuf && other)
{
    if (other.m_data)
    {
        if (m_data)
        {
            if (m_data + m_len + 1 != other.m_data || other.m_data + other.m_len + 1 != stack->top)
                throw std::bad_alloc ();

            m_len = other.m_len;
            memmove (m_data, other.m_data, m_len + 1);
            stack->top = m_data + m_len + 1;
        }
        else
        {
            stack = other.stack;
            m_data = other.m_data;
            m_len = other.m_len;
        }

        other.stack = nullptr;
        other.m_data = nullptr;
        other.m_len = 0;
    }
    else
    {
        if (m_data)
        {
            this->~StringBuf ();
            stack = nullptr;
            m_data = nullptr;
            m_len = 0;
        }
    }
}

EXPORT void StringBuf::combine (StringBuf && other)
{
    if (m_data + m_len + 1 != other.m_data || other.m_data + other.m_len + 1 != stack->top)
        throw std::bad_alloc ();

    memmove (m_data + m_len, other.m_data, other.m_len + 1);
    m_len += other.m_len;
    stack->top --;

    other.stack = nullptr;
    other.m_data = nullptr;
    other.m_len = 0;
}
