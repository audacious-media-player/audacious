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

#include <new>

#include "objects.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

struct StringStack
{
    static constexpr int Size = 1048576;  // 1 MB

    char * top;
    char buf[Size - sizeof top];
};

// adds one byte for null character and rounds up to word boundary
static constexpr int align (int len)
{
    return (len + sizeof (void *)) & ~(sizeof (void *) - 1);
}

static pthread_key_t key;
static pthread_once_t once = PTHREAD_ONCE_INIT;

#ifdef _WIN32
static HANDLE mapping;
#endif

static void free_stack (void * stack)
{
    if (stack)
#ifdef _WIN32
        UnmapViewOfFile (stack);
#else
        munmap (stack, sizeof (StringStack));
#endif
}

static void make_key ()
{
    pthread_key_create (& key, free_stack);

#ifdef _WIN32
    mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
     0, sizeof (StringStack), nullptr);

    if (! mapping)
        throw std::bad_alloc ();
#endif
}

static StringStack * get_stack ()
{
    pthread_once (& once, make_key);

    StringStack * stack = (StringStack *) pthread_getspecific (key);

    if (! stack)
    {
#ifdef _WIN32
        stack = (StringStack *) MapViewOfFile (mapping, FILE_MAP_COPY, 0, 0, sizeof (StringStack));

        if (! stack)
            throw std::bad_alloc ();
#else
        stack = (StringStack *) mmap (nullptr, sizeof (StringStack),
         PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (stack == MAP_FAILED)
            throw std::bad_alloc ();
#endif

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
        if (m_data + align (m_len) != stack->top)
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
        stack->top = m_data + align (len);

        if (stack->top - stack->buf > (int) sizeof stack->buf)
            throw std::bad_alloc ();

        m_data[len] = 0;
        m_len = len;
    }
}

EXPORT StringBuf::~StringBuf () noexcept (false)
{
    if (m_data)
    {
        if (m_data + align (m_len) != stack->top)
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
            if (m_data + align (m_len) != other.m_data ||
             other.m_data + align (other.m_len) != stack->top)
                throw std::bad_alloc ();

            m_len = other.m_len;
            memmove (m_data, other.m_data, m_len + 1);
            stack->top = m_data + align (m_len);
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
    if (! other.m_data)
        return;

    if (m_data)
    {
        if (m_data + align (m_len) != other.m_data ||
         other.m_data + align (other.m_len) != stack->top)
            throw std::bad_alloc ();

        memmove (m_data + m_len, other.m_data, other.m_len + 1);
        m_len += other.m_len;
        stack->top = m_data + align (m_len);
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

EXPORT void StringBuf::insert (int pos, const char * s, int len)
{
    int len0 = m_len;

    if (pos < 0)
        pos = len0;
    if (len < 0)
        len = strlen (s);

    resize (len0 + len);
    memmove (m_data + pos + len, m_data + pos, len0 - pos);
    memcpy (m_data + pos, s, len);
}

EXPORT void StringBuf::remove (int pos, int len)
{
    int after = m_len - pos - len;
    memmove (m_data + pos, m_data + pos + len, after);
    resize (pos + after);
}
