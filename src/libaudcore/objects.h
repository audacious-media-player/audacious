/*
 * objects.h
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

#ifndef LIBAUDCORE_OBJECTS_H
#define LIBAUDCORE_OBJECTS_H

#ifdef AUD_GLIB_INTEGRATION
#include <glib.h>
#endif

#include <libaudcore/templates.h>

// Stores array pointer together with deduced array length.

template<class T>
struct ArrayRef
{
    const T * data;
    int len;

    constexpr ArrayRef (decltype (nullptr) = nullptr) :
        data (nullptr),
        len (0) {}

    template<int N>
    constexpr ArrayRef (const T (& array) [N]) :
        data (array),
        len (N) {}

    constexpr ArrayRef (const T * data, int len) :
        data (data),
        len (len) {}

    const T * begin () const
        { return data; }
    const T * end () const
        { return data + len; }
};

// Smart pointer.  Deletes object pointed to when the pointer goes out of scope.

template<class T, void (* deleter) (T *) = aud::delete_typed>
class SmartPtr
{
public:
    constexpr SmartPtr () :
        ptr (nullptr) {}
    explicit constexpr SmartPtr (T * ptr) :
        ptr (ptr) {}

    ~SmartPtr ()
        { if (ptr) deleter (ptr); }

    void capture (T * ptr2)
    {
        if (ptr) deleter (ptr);
        ptr = ptr2;
    }

    T * release ()
    {
        T * ptr2 = ptr;
        ptr = nullptr;
        return ptr2;
    }

    void clear ()
        { capture (nullptr); }

    SmartPtr (SmartPtr && b) :
        ptr (b.ptr)
    {
        b.ptr = nullptr;
    }

    SmartPtr & operator= (SmartPtr && b)
        { return aud::move_assign (* this, std::move (b)); }

    explicit operator bool () const
        { return (bool) ptr; }

    T * get ()
        { return ptr; }
    const T * get () const
        { return ptr; }
    T & operator* ()
        { return (* ptr); }
    const T & operator* () const
        { return (* ptr); }
    T * operator-> ()
        { return ptr; }
    const T * operator-> () const
        { return ptr; }

private:
    T * ptr;
};

template<class T, class ... Args>
SmartPtr<T> SmartNew (Args && ... args)
{
    return SmartPtr<T> (aud::construct<T>::make (operator new (sizeof (T)),
     std::forward<Args> (args) ...));
}

// Convenience wrapper for a GLib-style string (char *).

#ifdef AUD_GLIB_INTEGRATION
class CharPtr : public SmartPtr<char, aud::typed_func<char, g_free>>
{
public:
    CharPtr () : SmartPtr () {}
    explicit CharPtr (char * ptr) : SmartPtr (ptr) {}

    // non-const operator omitted to prevent "CharPtr s; g_free(s);"
    operator const char * () const
        { return get (); }
};
#endif

// Wrapper class for a string stored in the string pool.

class String
{
public:
    constexpr String () :
        raw (nullptr) {}

    ~String ()
        { raw_unref (raw); }

    String (const String & b) :
        raw (raw_ref (b.raw)) {}

    String & operator= (const String & b)
    {
        if (this != & b)
        {
            raw_unref (raw);
            raw = raw_ref (b.raw);
        }
        return * this;
    }

    String (String && b) :
        raw (b.raw)
    {
        b.raw = nullptr;
    }

    String & operator= (String && b)
        { return aud::move_assign (* this, std::move (b)); }

    bool operator== (const String & b) const
        { return raw_equal (raw, b.raw); }

    explicit String (const char * str) :
        raw (raw_get (str)) {}

    String (decltype (nullptr)) = delete;

    operator const char * () const
        { return raw; }

    unsigned hash () const
        { return raw_hash (raw); }

private:
    static char * raw_get (const char * str);
    static char * raw_ref (const char * str);
    static void raw_unref (char * str);
    static unsigned raw_hash (const char * str);
    static bool raw_equal (const char * str1, const char * str2);

    char * raw;
};

struct StringStack;

// Mutable string buffer, allocated on a stack-like structure.  The intent is
// to provide fast allocation/deallocation for strings with a short lifespan.
// Note that some usage patterns (for example, repeatedly appending to multiple
// strings) can rapidly cause memory fragmentation and eventually lead to an
// out-of-memory exception.
//
// Some usage guidelines:
//
//   1. Always declare StringBufs within function or block scope, never at file
//      or class scope.  Do not attempt to create a StringBuf with new or
//      malloc().
//   2. If you need to return a StringBuf from a function that uses several
//      different strings internally, make sure all the other strings go out of
//      scope first and then call settle() on the string to be returned.
//   3. Never transfer StringBuf objects across thread boundaries.

class StringBuf
{
public:
    constexpr StringBuf () :
        stack (nullptr),
        m_data (nullptr),
        m_len (0) {}

    explicit StringBuf (int len) :
        stack (nullptr),
        m_data (nullptr),
        m_len (0)
    {
        resize (len);
    }

    StringBuf (StringBuf && other) :
        stack (other.stack),
        m_data (other.m_data),
        m_len (other.m_len)
    {
        other.stack = nullptr;
        other.m_data = nullptr;
        other.m_len = 0;
    }

    StringBuf & operator= (StringBuf && other)
        { return aud::move_assign (* this, std::move (other)); }

    ~StringBuf ();

    // Resizes to <len> bytes (not counting the terminating null byte) by
    // appended uninitialized bytes or truncating.  The resized string will be
    // null-terminated unless <len> is -1.  A length of -1 means to make the
    // string as large as possible.  This can be useful when the required length
    // is not known in advance.  However, it will be impossible to create any
    // further StringBufs until resize() is called again.
    void resize (int len);

    // Inserts the substring <s> at the given position, or appends it if <pos>
    // is -1.  If <len> is -1, <s> is assumed to be null-terminated; otherwise,
    // <len> indicates the number of bytes to insert.  If <s> is a null pointer,
    // uninitialized bytes are inserted and <len> must not be -1.  A pointer to
    // the inserted substring is returned for convenience.
    char * insert (int pos, const char * s, int len = -1);

    // Removes <len> bytes at the given position.
    void remove (int pos, int len);

    // Collapses any unused space preceding this string.
    // Judicious use can combat memory fragmentation.
    // Returns a move reference to allow e.g. "return str.settle();"
    StringBuf && settle ();

    int len () const
        { return m_len; }

    operator char * ()
        { return m_data; }

    // deprecated, use assignment
    void steal (StringBuf && other) __attribute__((deprecated));
    // deprecated, use insert()
    void combine (StringBuf && other) __attribute__((deprecated));

private:
    StringStack * stack;
    char * m_data;
    int m_len;
};

#endif // LIBAUDCORE_OBJECTS_H
