/*
 * logger.cc
 * Copyright 2014 John Lindgren and William Pitcock
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

#include "audstrings.h"
#include "index.h"
#include "runtime.h"
#include "tinylock.h"

#include <stdio.h>

namespace audlog {

struct HandlerData {
    Handler handler;
    Level level;
};

static TinyRWLock lock;
static Index<HandlerData> handlers;
static Level stderr_level = Warning;
static Level min_level = Warning;

EXPORT void set_stderr_level (Level level)
{
    tiny_lock_write (& lock);

    min_level = stderr_level = level;

    for (const HandlerData & h : handlers)
    {
        if (h.level < min_level)
            min_level = h.level;
    }

    tiny_unlock_write (& lock);
}

EXPORT void subscribe (Handler handler, Level level)
{
    tiny_lock_write (& lock);

    handlers.append (handler, level);

    if (level < min_level)
        min_level = level;

    tiny_unlock_write (& lock);
}

EXPORT void unsubscribe (Handler handler)
{
    tiny_lock_write (& lock);

    auto is_match = [=] (const HandlerData & data)
        { return data.handler == handler; };

    handlers.remove_if (is_match, true);

    min_level = stderr_level;

    for (const HandlerData & h : handlers)
    {
        if (h.level < min_level)
            min_level = h.level;
    }

    tiny_unlock_write (& lock);
}

EXPORT const char * get_level_name (Level level)
{
    switch (level)
    {
        case Debug: return "DEBUG";
        case Info: return "INFO";
        case Warning: return "WARNING";
        case Error: return "ERROR";
    };

    return nullptr;
}

EXPORT void log (Level level, const char * file, int line, const char * func,
 const char * format, ...)
{
    tiny_lock_read (& lock);

    if (level >= min_level)
    {
        va_list args;
        va_start (args, format);
        StringBuf message = str_vprintf (format, args);
        va_end (args);

        if (level >= stderr_level)
            fprintf (stderr, "%s %s:%d [%s]: %s", get_level_name (level), file,
             line, func, (const char *) message);

        for (const HandlerData & h : handlers)
        {
            if (level >= h.level)
                h.handler (level, file, line, func, message);
        }
    }

    tiny_unlock_read (& lock);
}

} // namespace audlog
