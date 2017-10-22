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
#include <stdint.h>
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

struct StringHeader
{
    StringHeader * next, * prev;
    int len;
};

struct StringStack
{
    static constexpr int Size = 1048576;  // 1 MB

    StringHeader * top;
    char buf[Size - sizeof top];
};

static constexpr intptr_t align (intptr_t ptr, intptr_t size)
{
    return (ptr + (size - 1)) / size * size;
}

static StringHeader * align_after (StringStack * stack, StringHeader * prev_header)
{
    char * base;
    if (prev_header)
        base = (char *) prev_header + sizeof (StringHeader) + prev_header->len + 1;
    else
        base = stack->buf;

    return (StringHeader *) align ((intptr_t) base, alignof (StringHeader));
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

        stack->top = nullptr;
        pthread_setspecific (key, stack);
    }

    return stack;
}

EXPORT void StringBuf::resize (int len)
{
    if (! stack)
        stack = get_stack ();

    StringHeader * header = nullptr;
    bool need_alloc = true;

    if (m_data)
    {
        header = (StringHeader *) (m_data - sizeof (StringHeader));

        /* check if there is enough space in the current location */
        char * limit = header->next ? (char *) header->next : (char *) stack + sizeof (StringStack);
        int max_len = limit - 1 - m_data;

        if ((len < 0 && ! header->next) || (len >= 0 && len < max_len))
        {
            m_len = header->len = (len < 0) ? max_len : len;
            need_alloc = false;
        }
    }

    if (need_alloc)
    {
        /* allocate a new string at the top of the stack */
        StringHeader * new_header = align_after (stack, stack->top);
        char * new_data = (char *) new_header + sizeof (StringHeader);
        char * limit = (char *) stack + sizeof (StringStack);
        int max_len = limit - 1 - new_data;

        if (max_len < aud::max (len, 0))
            throw std::bad_alloc ();

        int new_len = (len < 0) ? max_len : len;

        if (stack->top)
            stack->top->next = new_header;

        new_header->prev = stack->top;
        new_header->next = nullptr;
        new_header->len = new_len;

        stack->top = new_header;

        /* move the old data, if any */
        if (m_data)
        {
            int bytes_to_copy = aud::min (m_len, new_len);
            memcpy (new_data, m_data, bytes_to_copy);

            if (header->prev)
                header->prev->next = header->next;

            /* we know header != stack->top */
            header->next->prev = header->prev;
        }

        m_data = new_data;
        m_len = new_len;
    }

    /* Null-terminate the string except when the maximum length was requested
     * (to avoid paging in the entire 1 MB stack prematurely).  The caller is
     * expected to follow up with a more realistic resize() in this case. */
    if (len >= 0)
        m_data[len] = 0;
}

EXPORT StringBuf::~StringBuf ()
{
    if (m_data)
    {
        auto header = (StringHeader *) (m_data - sizeof (StringHeader));

        if (header->prev)
            header->prev->next = header->next;

        if (header == stack->top)
            stack->top = header->prev;
        else
            header->next->prev = header->prev;
    }
}

EXPORT void StringBuf::steal (StringBuf && other)
{
    (* this = std::move (other)).settle ();
}

EXPORT StringBuf && StringBuf::settle ()
{
    if (m_data)
    {
        /* collapse any space preceding this string */
        auto header = (StringHeader *) (m_data - sizeof (StringHeader));
        StringHeader * new_header = align_after (stack, header->prev);

        if (new_header != header)
        {
            if (header->prev)
                header->prev->next = new_header;

            if (header == stack->top)
                stack->top = new_header;
            else
                header->next->prev = new_header;

            memmove (new_header, header, sizeof (StringHeader) + m_len + 1);
            m_data = (char *) new_header + sizeof (StringHeader);
        }
    }

    return std::move (* this);
}

EXPORT void StringBuf::combine (StringBuf && other)
{
    if (! other.m_data)
        return;

    insert (m_len, other.m_data, other.m_len);
    other = StringBuf ();
    settle ();
}

EXPORT char * StringBuf::insert (int pos, const char * s, int len)
{
    int len0 = m_len;

    if (pos < 0)
        pos = len0;
    if (len < 0)
        len = strlen (s);

    resize (len0 + len);
    memmove (m_data + pos + len, m_data + pos, len0 - pos);

    if (s)
        memcpy (m_data + pos, s, len);

    return m_data + pos;
}

EXPORT void StringBuf::remove (int pos, int len)
{
    int after = m_len - pos - len;
    memmove (m_data + pos, m_data + pos + len, after);
    resize (pos + after);
}
