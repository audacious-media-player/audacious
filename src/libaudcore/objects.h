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

    SmartPtr (const SmartPtr &) = delete;
    void operator= (const SmartPtr &) = delete;

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

    operator bool () const
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

class String
{
public:
    constexpr String () :
        raw (nullptr) {}

    ~String ()
    {
        str_unref (raw);
    }

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

    operator const char * () const
        { return raw; }

    unsigned hash () const
        { return str_hash (raw); }

    /* considered harmful */
    static String from_c (char * str)
    {
        String s;
        s.raw = str;
        return s;
    }

    /* considered harmful */
    char * to_c ()
    {
        char * str = raw;
        raw = nullptr;
        return str;
    }

private:
    char * raw;
};

/* somewhat out of place here */
struct PlaylistAddItem {
    String filename;
    Tuple * tuple;
    PluginHandle * decoder;
};

#endif // LIBAUDCORE_OBJECTS_H
