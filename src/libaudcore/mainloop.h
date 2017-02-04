/*
 * mainloop.h
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

/* Main loop abstraction layer which can use either GLib or Qt as a backend.
 * The API is completely thread-safe and can thus be used as a means to call
 * back into the main thread from a worker thread. */

#ifndef LIBAUDCORE_MAINLOOP_H
#define LIBAUDCORE_MAINLOOP_H

struct QueuedFuncHelper;
struct QueuedFuncParams;

class QueuedFunc
{
    friend struct QueuedFuncHelper;

public:
    typedef void (* Func) (void * data);

    // one-time idle callback
    void queue (Func func, void * data);

    // one-time delayed callback
    void queue (int delay_ms, Func func, void * data);

    // periodic timer callback
    void start (int interval_ms, Func func, void * data);

    // stops any type of callback
    // note that queue() and start() also stop any previous callback
    void stop ();

    // true if a periodic timer is running
    // does not apply to one-time callbacks
    bool running () const
        { return _running; }

    constexpr QueuedFunc () = default;
    QueuedFunc (const QueuedFunc &) = delete;
    void operator= (const QueuedFunc &) = delete;

    ~QueuedFunc ()
        { stop (); }

private:
    struct Starter;
    struct Stopper;

    bool _running = false;

    void start (const QueuedFuncParams & params);
};

void mainloop_run ();
void mainloop_quit ();

#endif // LIBAUDCORE_MAINLOOP_H
