/*
 * multihash.h
 * Copyright 2013-2017 John Lindgren
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

#ifndef LIBAUDCORE_MULTIHASH_H
#define LIBAUDCORE_MULTIHASH_H

#include <utility>
#include <libaudcore/tinylock.h>

/* HashBase is a low-level hash table implementation.  It is used as a backend
 * for SimpleHash as well as for a single channel of MultiHash. */

class HashBase
{
public:
    /* Skeleton structure containing internal members of a hash node (except for
     * "refs", which is not used internally and included here only to fill an
     * alignment gap).  Actual node structures should subclass Node. */
    struct Node {
        Node * next;
        unsigned hash;
        unsigned refs;
    };

    /* Represents the location of a node within the table. */
    struct NodeLoc {
        Node * * ptr;
        Node * next;
    };

    /* Callback.  Returns true if <node> matches <data>, otherwise false. */
    typedef bool (* MatchFunc) (const Node * node, const void * data);

    /* Callback.  Called when a node is found.  Returns true if <node> is to be
     * removed, otherwise false. */
    typedef bool (* FoundFunc) (Node * node, void * state);

    constexpr HashBase () :
        buckets (nullptr),
        size (0),
        used (0) {}

    void clear ()  // use as destructor
    {
        delete[] buckets;
        * this = HashBase ();
    }

    int n_items () const
        { return used; }

    /* Adds a node.  Does not check for duplicates. */
    void add (Node * node, unsigned hash);

    /* Locates the node matching <data>.  Returns null if no node is found.  If
     * <loc> is not null, also returns the location of the node, which can be
     * used to remove it from the table. */
    Node * lookup (MatchFunc match, const void * data, unsigned hash,
     NodeLoc * loc = nullptr) const;

    /* Removes a node, given a location returned by lookup_full(). */
    void remove (const NodeLoc & loc);

    /* Iterates over all nodes in the table, removing them as desired. */
    void iterate (FoundFunc func, void * state);

private:
    static constexpr unsigned InitialSize = 16;

    void resize (unsigned new_size);

    Node * * buckets;
    unsigned size, used;
};

/* MultiHash is a generic, thread-safe hash table.  It scales well to multiple
 * processors by the use of multiple channels, each with a separate lock.  The
 * hash value of a given node decides what channel it is stored in.  Hence,
 * different processors will tend to hit different channels, keeping lock
 * contention to a minimum.  The all-purpose lookup function enables a variety
 * of atomic operations, such as allocating and adding a node only if not
 * already present. */

class MultiHash
{
public:
    static constexpr int Found = 1 << 0;
    static constexpr int Added = 1 << 1;
    static constexpr int Removed = 1 << 2;

    typedef HashBase::Node Node;
    typedef HashBase::MatchFunc MatchFunc;
    typedef HashBase::FoundFunc FoundFunc;
    typedef void (* FinalFunc) (void * state);

    /* Callback.  May create a new node representing <data> to be added to the
     * table.  Returns the new node or null. */
    typedef Node * (* AddFunc) (const void * data, void * state);

    MultiHash (MatchFunc match) :
        match (match),
        locks (),
        channels () {}

    /* There is no destructor.  In some instances, such as the string pool, it
     * is never safe to destroy the hash table, since it can be referenced from
     * the destructors of other static objects.  It is left to the operating
     * system to reclaim the memory used by the hash table upon process exit. */

    /* All-purpose lookup function.  The caller passes in the data to be looked
     * up along with its hash value.  The two callbacks are optional.  <add> is
     * called if no matching node is found, and may return a new node to add to
     * the table.  <found> is called if a matching node is found, and may return
     * true to remove the node from the table.  Returns the status of the lookup
     * as a bitmask of Found, Added, and Removed. */
    int lookup (const void * data, unsigned hash, AddFunc add, FoundFunc found, void * state);

    /* All-purpose iteration function.  All channels of the table are locked
     * simultaneously during the iteration to freeze the table in a consistent
     * state.  <func> is called on each node in order, and may return true to
     * remove the node from the table. */
    void iterate (FoundFunc func, void * state);

    /* Variant of iterate() which runs a second callback after the iteration
     * is complete, while the table is still locked.  This is useful when some
     * operation needs to be performed with the table in a known state. */
    void iterate (FoundFunc func, void * state, FinalFunc final, void * fstate);

private:
    static constexpr int Channels = 16;  /* must be a power of two */
    static constexpr int Shift = 24;  /* bit shift for channel selection */

    const MatchFunc match;
    TinyLock locks[Channels];
    HashBase channels[Channels];
};

/* Type-safe version using templates. */

template<class Node_T, class Data_T>
class MultiHash_T : private MultiHash
{
public:
    // Required interfaces:
    //
    // class Node_T : public Node
    // {
    //     bool match (const Data_T * data) const;
    // };
    //
    // class Operation
    // {
    //     Node_T * add (const Data_T * data);
    //     bool found (Node_T * node);
    // };

    MultiHash_T () : MultiHash (match_cb) {}

    void clear ()
        { MultiHash::iterate (remove_cb, nullptr); }

    template<class Op>
    int lookup (const Data_T * data, unsigned hash, Op & op)
        { return MultiHash::lookup (data, hash, WrapOp<Op>::add, WrapOp<Op>::found, & op); }

    template<class F>
    void iterate (F func)
        { MultiHash::iterate (WrapIterate<F>::run, & func); }

    template<class F, class Final>
    void iterate (F func, Final final)
        { MultiHash::iterate (WrapIterate<F>::run, & func, WrapFinal<Final>::run, & final); }

private:
    static bool match_cb (const Node * node, const void * data)
        { return (static_cast<const Node_T *> (node))->match
                  (static_cast<const Data_T *> (data)); }

    static bool remove_cb (Node * node, void *)
    {
        delete static_cast<Node_T *> (node);
        return true;
    }

    template<class Op>
    struct WrapOp {
        static Node * add (const void * data, void * op)
            { return (static_cast<Op *> (op))->add
                      (static_cast<const Data_T *> (data)); }
        static bool found (Node * node, void * op)
            { return (static_cast<Op *> (op))->found
                      (static_cast<Node_T *> (node)); }
    };

    template<class F>
    struct WrapIterate {
        static bool run (Node * node, void * func)
            { return (* static_cast<F *> (func))
                      (static_cast<Node_T *> (node)); }
    };

    template<class Final>
    struct WrapFinal {
        static void run (void * func)
            { (* static_cast<Final *> (func)) (); }
    };
};

/* Simpler single-thread hash table. */

template<class Key, class Value>
class SimpleHash : private HashBase
{
public:
    typedef void (* IterFunc) (const Key & key, Value & value, void * state);

    ~SimpleHash ()
        { clear (); }

    using HashBase::n_items;

    Value * lookup (const Key & key)
    {
        auto node = static_cast<Node *> (HashBase::lookup (match_cb, & key, key.hash ()));
        return node ? & node->value : nullptr;
    }

    Value * add (const Key & key, Value && value)
    {
        unsigned hash = key.hash ();
        auto node = static_cast<Node *> (HashBase::lookup (match_cb, & key, hash));

        if (node)
            node->value = std::move (value);
        else
        {
            node = new Node (key, std::move (value));
            HashBase::add (node, hash);
        }

        return & node->value;
    }

    void remove (const Key & key)
    {
        NodeLoc loc;
        auto node = static_cast<Node *> (HashBase::lookup (match_cb, & key, key.hash (), & loc));

        if (node)
        {
            delete node;
            HashBase::remove (loc);
        }
    }

    void clear ()
    {
        HashBase::iterate (remove_cb, nullptr);
        HashBase::clear ();
    }

    template<class F>
    void iterate (F func)
        { HashBase::iterate (WrapIterate<F>::run, & func); }

private:
    struct Node : public HashBase::Node
    {
        Node (const Key & key, Value && value) :
            key (key),
            value (std::move (value)) {}

        Key key;
        Value value;
    };

    struct IterData {
        IterFunc func;
        void * state;
    };

    static bool match_cb (const HashBase::Node * node, const void * data)
        { return (static_cast<const Node *> (node))->key ==
                 * static_cast<const Key *> (data); }

    static bool remove_cb (HashBase::Node * node, void *)
    {
        delete static_cast<Node *> (node);
        return true;
    }

    // C-style callback wrapping generic iteration functor
    template<class F>
    struct WrapIterate
    {
        static bool run (HashBase::Node * node_, void * func)
        {
            auto node = static_cast<Node *> (node_);
            (* static_cast<F *> (func)) (node->key, node->value);
            return false;
        }
    };
};

#endif /* LIBAUDCORE_MULTIHASH_H */
