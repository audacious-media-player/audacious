/*
 * mainloop.cc
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

#include "mainloop.h"

#ifdef USE_QT

#include <QCoreApplication>

class QueuedFuncEvent : public QEvent
{
public:
    QueuedFuncEvent (QueuedFunc * func) :
        QEvent (User),
        func (func) {}

    void run ()
        { func->run (); }

private:
    QueuedFunc * func;
};

class QueuedFuncReceiver : public QObject
{
protected:
    void customEvent (QEvent * event)
        { dynamic_cast<QueuedFuncEvent *> (event)->run (); }
};

static QueuedFuncReceiver queued_receiver;

class TimedFuncReceiver : public QObject
{
public:
    TimedFuncReceiver (int interval_ms, TimedFunc * func) :
        interval_ms (interval_ms),
        func (func)
    {
        // tell Qt to process events for this object in the main thread
        moveToThread (queued_receiver.thread ());

        // The timer cannot be started until QCoreApplication is instantiated.
        // Send ourselves an event and wait till it comes back, then start the timer.
        QCoreApplication::postEvent (this, new QEvent (QEvent::User), Qt::HighEventPriority);
    }

protected:
    void customEvent (QEvent * event)
        { startTimer (interval_ms); }
    void timerEvent (QTimerEvent * event)
        { func->run (); }

private:
    int interval_ms;
    TimedFunc * func;
};

#else // ! USE_QT

#include <glib.h>

static int queued_wrapper (void * ptr)
{
    (* (QueuedFunc *) ptr).run ();
    return G_SOURCE_REMOVE;
}

static int timed_wrapper (void * ptr)
{
    (* (TimedFunc *) ptr).run ();
    return G_SOURCE_CONTINUE;
}

#endif // ! USE_QT

void QueuedFunc::queue (Func func, void * data)
{
    tiny_lock (& lock);

    if (! stored_func)
#ifdef USE_QT
        QCoreApplication::postEvent (& queued_receiver,
         new QueuedFuncEvent (this), Qt::HighEventPriority);
#else
        g_idle_add_full (G_PRIORITY_HIGH, queued_wrapper, this, nullptr);
#endif

    stored_func = func;
    stored_data = data;

    tiny_unlock (& lock);
}

void QueuedFunc::cancel ()
{
    tiny_lock (& lock);

    stored_func = nullptr;
    stored_data = nullptr;

    tiny_unlock (& lock);
}

bool QueuedFunc::queued ()
{
    return stored_func;
}

void QueuedFunc::run ()
{
    tiny_lock (& lock);

    Func func = stored_func;
    void * data = stored_data;
    stored_func = nullptr;
    stored_data = nullptr;

    tiny_unlock (& lock);

    if (func)
        func (data);
}

void TimedFunc::start (int interval_ms, Func func, void * data)
{
    tiny_lock (& lock);

#ifdef USE_QT
    if (impl)
        delete (TimedFuncReceiver *) impl;

    impl = new TimedFuncReceiver (interval_ms, this);
#else
    if (impl)
        g_source_remove (GPOINTER_TO_INT (impl));

    impl = GINT_TO_POINTER (g_timeout_add (interval_ms, timed_wrapper, this));
#endif

    stored_func = func;
    stored_data = data;

    tiny_unlock (& lock);
}

void TimedFunc::stop ()
{
    tiny_lock (& lock);

    if (impl)
#ifdef USE_QT
        delete (TimedFuncReceiver *) impl;
#else
        g_source_remove (GPOINTER_TO_INT (impl));
#endif

    impl = nullptr;

    stored_func = nullptr;
    stored_data = nullptr;

    tiny_unlock (& lock);
}

bool TimedFunc::running ()
{
    return impl;
}

void TimedFunc::run ()
{
    tiny_lock (& lock);

    Func func = stored_func;
    void * data = stored_data;

    tiny_unlock (& lock);

    if (func)
        func (data);
}

#ifdef USE_QT

static QCoreApplication * app;

void mainloop_run ()
{
    int dummy_argc = 0;
    app = new QCoreApplication (dummy_argc, nullptr);
    app->exec ();
    delete app;
    app = nullptr;
}

void mainloop_quit ()
{
    app->quit ();
}

bool mainloop_running ()
{
    return app;
}

#else // ! USE_QT

static GMainLoop * mainloop;

void mainloop_run ()
{
    mainloop = g_main_loop_new (nullptr, true);
    g_main_loop_run (mainloop);
    g_main_loop_unref (mainloop);
    mainloop = nullptr;
}

void mainloop_quit ()
{
    g_main_loop_quit (mainloop);
}

bool mainloop_running ()
{
    return g_main_loop_is_running (mainloop);
}

#endif // ! USE_QT
