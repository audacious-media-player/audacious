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

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "objects.h"

using namespace std;

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

        assert (stack != MAP_FAILED);

        stack->top = stack->buf;
        pthread_setspecific (key, stack);
    }

    return stack;
}

EXPORT void StringBuf::resize (int size)
{
    if (! stack)
    {
        stack = get_stack ();
        m_data = stack->top;
    }

    assert (m_data + m_size == stack->top);

    if (size < 0)
        size = stack->avail (m_data);
    else
        assert (size <= stack->avail (m_data));

    m_size = size;
    stack->top = m_data + size;
}

EXPORT StringBuf::~StringBuf ()
{
    if (m_data)
    {
        assert (m_data + m_size == stack->top);
        stack->top = m_data;
    }
}

EXPORT void StringBuf::steal (StringBuf && other)
{
    if (! other.m_data)
    {
        this->~StringBuf ();
        stack = nullptr;
        m_data = 0;
        m_size = 0;
        return;
    }

    assert (m_data + m_size == other.m_data);
    assert (other.m_data + other.m_size == stack->top);

    m_size = other.m_size;
    memmove (m_data, other.m_data, m_size);
    stack->top = m_data + m_size;

    other.stack = nullptr;
    other.m_data = 0;
    other.m_size = 0;
}
