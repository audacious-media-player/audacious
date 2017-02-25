/*
 * strpool.c
 * Copyright 2011-2017 John Lindgren
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

#include <stdlib.h>
#include <string.h>

#include "audstrings.h"
#include "internal.h"
#include "multihash.h"
#include "objects.h"
#include "runtime.h"

#ifdef VALGRIND_FRIENDLY

#include <glib.h>

EXPORT char * String::raw_get (const char * str)
    { return g_strdup (str); }
EXPORT char * String::raw_ref (const char * str)
    { return g_strdup (str); }
EXPORT void String::raw_unref (char * str)
    { g_free (str); }

void string_leak_check () {}

EXPORT unsigned String::raw_hash (const char * str)
    { return str_calc_hash (str); }
EXPORT bool String::raw_equal (const char * str1, const char * str2)
    { return ! strcmp_safe (str1, str2); }

#else // ! VALGRIND_FRIENDLY

struct StrNode : public MultiHash::Node
{
    /* the characters of the string immediately follow the StrNode struct */
    const char * str () const
        { return reinterpret_cast<const char *> (this + 1); }
    char * str ()
        { return reinterpret_cast<char *> (this + 1); }

    static const StrNode * of (const char * s)
        { return reinterpret_cast<const StrNode *> (s) - 1; }
    static StrNode * of (char * s)
        { return reinterpret_cast<StrNode *> (s) - 1; }

    static StrNode * create (const char * s)
    {
        auto size = sizeof (StrNode) + strlen (s) + 1;
        auto node = static_cast<StrNode *> (malloc (size));
        if (! node)
            throw std::bad_alloc ();

        strcpy (node->str (), s);
        return node;
    }

    bool match (const char * data) const
        { return data == str () || ! strcmp (data, str ()); }
};

static MultiHash_T<StrNode, char> strpool_table;

struct Getter
{
    StrNode * node;

    StrNode * add (const char * data)
    {
        node = StrNode::create (data);
        node->refs = 1;
        return node;
    }

    bool found (StrNode * node_)
    {
        node = node_;
        __sync_fetch_and_add (& node->refs, 1);
        return false;
    }
};

struct Remover
{
    StrNode * add (const char *)
        { return nullptr; }

    bool found (StrNode * node)
    {
        if (! __sync_bool_compare_and_swap (& node->refs, 1, 0))
            return false;

        free (node);
        return true;
    }
};

/* If the pool contains a copy of <str>, increments its reference count.
 * Otherwise, adds a copy of <str> to the pool with a reference count of one.
 * In either case, returns the copy.  Because this copy may be shared by other
 * parts of the code, it should not be modified.  If <str> is null, simply
 * returns null with no side effects. */
EXPORT char * String::raw_get (const char * str)
{
    if (! str)
        return nullptr;

    Getter op;
    strpool_table.lookup (str, str_calc_hash (str), op);
    return op.node->str ();
}

/* Increments the reference count of <str>, where <str> is the address of a
 * string already in the pool.  Faster than calling raw_get() a second time.
 * Returns <str> for convenience.  If <str> is null, simply returns null with no
 * side effects. */
EXPORT char * String::raw_ref (const char * str_)
{
    auto str = const_cast<char *> (str_);
    if (! str)
        return nullptr;

    auto node = StrNode::of (str);
    __sync_fetch_and_add (& node->refs, 1);
    return str;
}

/* Decrements the reference count of <str>, where <str> is the address of a
 * string in the pool.  If the reference count drops to zero, releases the
 * memory used by <str>.   If <str> is null, simply returns null with no side
 * effects. */
EXPORT void String::raw_unref (char * str)
{
    if (! str)
        return;

    auto node = StrNode::of (str);

    while (1)
    {
        unsigned refs = __sync_fetch_and_add (& node->refs, 0);
        if (refs > 1)
        {
            if (__sync_bool_compare_and_swap (& node->refs, refs, refs - 1))
                break;
        }
        else
        {
            Remover op;
            int status = strpool_table.lookup (str, node->hash, op);
            if (! (status & MultiHash::Found))
                throw std::bad_alloc ();
            if (status & MultiHash::Removed)
                break;
        }
    }
}

void string_leak_check ()
{
    strpool_table.iterate ([] (const StrNode * node) {
        AUDWARN ("String leaked: %s\n", node->str ());
        return false;
    });
}

/* Returns the cached hash value of a pooled string (or 0 for null). */
EXPORT unsigned String::raw_hash (const char * str)
{
    return str ? StrNode::of (str)->hash : 0;
}

/* Checks whether two pooled strings are equal.  Since the pool never contains
 * duplicate strings, this is a simple pointer comparison and thus much faster
 * than strcmp().  null is considered equal to null but not equal to any string. */
EXPORT bool String::raw_equal (const char * str1, const char * str2)
{
    return str1 == str2;
}

#endif // ! VALGRIND_FRIENDLY
