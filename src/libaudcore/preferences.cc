/*
 * preferences.cc
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

#include "preferences.h"
#include "runtime.h"

#include <assert.h>

EXPORT bool WidgetConfig::get_bool () const
{
    assert (type == Bool);

    if (value)
        return * (bool *) value;
    else if (name)
        return aud_get_bool (section, name);
    else
        return false;
}

EXPORT void WidgetConfig::set_bool (bool val) const
{
    assert (type == Bool);

    if (value)
        * (bool *) value = val;
    else if (name)
        aud_set_bool (section, name, val);

    if (callback)
        callback ();
}

EXPORT int WidgetConfig::get_int () const
{
    assert (type == Int);

    if (value)
        return * (int *) value;
    else if (name)
        return aud_get_int (section, name);
    else
        return 0;
}

EXPORT void WidgetConfig::set_int (int val) const
{
    assert (type == Int);

    if (value)
        * (int *) value = val;
    else if (name)
        aud_set_int (section, name, val);

    if (callback)
        callback ();
}

EXPORT double WidgetConfig::get_float () const
{
    assert (type == Float);

    if (value)
        return * (double *) value;
    else if (name)
        return aud_get_double (section, name);
    else
        return 0;
}

EXPORT void WidgetConfig::set_float (double val) const
{
    assert (type == Float);

    if (value)
        * (double *) value = val;
    else if (name)
        aud_set_double (section, name, val);

    if (callback)
        callback ();
}

EXPORT String WidgetConfig::get_string () const
{
    assert (type == String);

    if (value)
        return * (::String *) value;
    else if (name)
        return aud_get_str (section, name);
    else
        return ::String ();
}

EXPORT void WidgetConfig::set_string (const char * val) const
{
    assert (type == String);

    if (value)
        * (::String *) value = ::String (val);
    else if (name)
        aud_set_str (section, name, val);

    if (callback)
        callback ();
}
