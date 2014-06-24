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

#include <libaudcore/core.h>

// Smart pointer.  Deletes object pointed to when the pointer goes out of scope.

template<class T>
class SmartPtr
{
public:
    constexpr SmartPtr (T * ptr = nullptr) :
        ptr (ptr) {}

    ~SmartPtr ()
    {
        delete ptr;
    }

    SmartPtr (SmartPtr && b) :
        ptr (b.ptr)
    {
        b.ptr = nullptr;
    }

    void operator= (SmartPtr && b)
    {
        if (this != & b)
        {
            delete ptr;
            ptr = b.ptr;
            b.ptr = nullptr;
        }
    }

    explicit operator bool () const
        { return (bool) ptr; }

    void swap (SmartPtr & b)
    {
        T * temp = ptr;
        ptr = b.ptr;
        b.ptr = temp;
    }

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

// Wrapper class for a string stored in the string pool.  Use this instead of
// str_get() and str_unref() wherever possible.

class String
{
public:
    constexpr String () :
        raw (nullptr) {}

    ~String ()
        { str_unref (raw); }

    String (const String & b) :
        raw (str_ref (b.raw)) {}

    void operator= (const String & b)
    {
        if (this != & b)
        {
            str_unref (raw);
            raw = str_ref (b.raw);
        }
    }

    String (String && b) :
        raw (b.raw)
    {
        b.raw = nullptr;
    }

    void operator= (String && b)
    {
        if (this != & b)
        {
            str_unref (raw);
            raw = b.raw;
            b.raw = nullptr;
        }
    }

    bool operator== (const String & b) const
        { return str_equal (raw, b.raw); }

    explicit String (const char * str) :
        raw (str_get (str)) {}

    String (decltype (nullptr)) = delete;

    operator const char * () const
        { return raw; }

    unsigned hash () const
        { return str_hash (raw); }

    // please don't use
    static String from_c (char * str)
    {
        String s;
        s.raw = str;
        return s;
    }

    // please don't use
    char * to_c ()
    {
        char * str = raw;
        raw = nullptr;
        return str;
    }

private:
    char * raw;
};

struct StringStack;

// Mutable string buffer, allocated on a stack to allow fast allocation.  The
// price for this speed is that only the top string in the stack (i.e. the one
// most recently allocated) can be resized or deleted.  The string is always
// null-terminated (i.e. str[str.len ()] == 0).  Rules for the correct use of
// StringBuf can be summarized as follows:
//
//   1. Always declare StringBufs within function or block scope, never at file
//      or class scope.  Do not attempt to create a StringBuf with new or
//      malloc().
//   2. Only the first StringBuf declared in a function can be used as the
//      return value.  It is possible to create a second StringBuf and then
//      transfer its contents to the first with steal(), but doing so carries
//      a performance penalty.
//   3. Do not truncate the StringBuf by inserting null characters manually;
//      instead, use resize().

class StringBuf
{
public:
    constexpr StringBuf () :
        stack (nullptr),
        m_data (nullptr),
        m_len (0) {}

    // A length of -1 means to use all available space.  This can be useful when
    // the final length of the string is not known in advance, but keep in mind
    // that you will not be able to create any further StringBufs until you call
    // resize().  Also, the string will not be null-terminated in this case.
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

    // only allowed for top (or null) string
    ~StringBuf ();

    // only allowed for top (or null) string
    void resize (int size);

    // only allowed for top two strings (or when one string is null)
    void steal (StringBuf && other);

    // only allowed for top two strings
    void combine (StringBuf && other);

    int len () const
        { return m_len; }

    operator char * ()
        { return m_data; }

private:
    StringStack * stack;
    char * m_data;
    int m_len;
};

#endif // LIBAUDCORE_OBJECTS_H
