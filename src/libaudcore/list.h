/*
 * list.h
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

#ifndef LIBAUDCORE_LIST_H
#define LIBAUDCORE_LIST_H

struct ListNode
{
    friend class ListBase;

private:
    ListNode * prev = nullptr;
    ListNode * next = nullptr;
};

class ListBase
{
protected:
    typedef void (* DestroyFunc) (ListNode *);

    ListNode * head = nullptr;
    ListNode * tail = nullptr;

    void insert_after (ListNode * prev, ListNode * node);
    void remove (ListNode * node);
    void clear (DestroyFunc destroy);

    static ListNode * prev (ListNode * node)
        { return node->prev; }
    static ListNode * next (ListNode * node)
        { return node->next; }
};

template<class C>
class List : private ListBase
{
public:
    C * head () const
        { return (C *) ListBase::head; }
    C * tail () const
        { return (C *) ListBase::tail; }

    static C * prev (C * node)
        { return (C *) ListBase::prev (node); }
    static C * next (C * node)
        { return (C *) ListBase::next (node); }

    void insert_after (C * prev, C * node)
        { ListBase::insert_after (prev, node); }
    void remove (C * node)
        { ListBase::remove (node); }

    void prepend (C * node)
        { insert_after (nullptr, node); }
    void append (C * node)
        { insert_after (tail (), node); }

    void clear ()
        { ListBase::clear (destroy); }

    template<class MatchFunc>
    C * find (MatchFunc match)
    {
        for (C * node = head (); node; node = next (node))
        {
            if (match (* node))
                return node;
        }

        return nullptr;
    }

private:
    static void destroy (ListNode * node)
        { delete (C *) node; }
};

#endif // LIBAUDCORE_LIST_H
