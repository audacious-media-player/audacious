/*
 * list.cpp
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

#include "list.h"

EXPORT void ListBase::insert_after (ListNode * prev, ListNode * node)
{
    ListNode * next;

    if (prev)
    {
        next = prev->next;
        prev->next = node;
    }
    else
    {
        next = head;
        head = node;
    }

    node->prev = prev;
    node->next = next;

    if (next)
        next->prev = node;
    else
        tail = node;
}

EXPORT void ListBase::remove (ListNode * node)
{
    ListNode * prev = node->prev;
    ListNode * next = node->next;

    node->prev = nullptr;
    node->next = nullptr;

    if (prev)
        prev->next = next;
    else
        head = next;

    if (next)
        next->prev = prev;
    else
        tail = prev;
}

EXPORT void ListBase::clear (DestroyFunc destroy)
{
    ListNode * node = head;
    while (node)
    {
        ListNode * next = node->next;
        destroy (node);
        node = next;
    }

    head = nullptr;
    tail = nullptr;
}
