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

template<class T>
class SmartPtr
{
public:
    constexpr SmartPtr (T * ptr = nullptr) :
        ptr (ptr) {}

    ~SmartPtr ()
    {
        delete ptr;
        ptr = nullptr;
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

#endif // LIBAUDCORE_OBJECTS_H
