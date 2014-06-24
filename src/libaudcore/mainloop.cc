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

#include <pthread.h>

#include "mainloop.h"

struct QueuedFuncRunner
{
    QueuedFunc * queued;
    QueuedFunc::Func func;
    void * data;
    int serial;

    bool run ()
    {
        if (__sync_add_and_fetch (& queued->serial, 0) != serial)
            return false;

        func (data);
        return true;
    }
};

#ifdef USE_QT

#include <QCoreApplication>

class QueuedFuncEvent : public QEvent
{
public:
    QueuedFuncEvent (const QueuedFuncRunner & runner) :
        QEvent (User),
        runner (runner) {}

    void run ()
        { runner.run (); }

private:
    QueuedFuncRunner runner;
};

class QueuedFuncRouter : public QObject
{
protected:
    void customEvent (QEvent * event)
        { dynamic_cast<QueuedFuncEvent *> (event)->run (); }
};

static QueuedFuncRouter router;

class QueuedFuncTimer : public QObject
{
public:
    QueuedFuncTimer (int interval_ms, const QueuedFuncRunner & runner) :
        interval_ms (interval_ms),
        runner (runner)
    {
        moveToThread (router.thread ());  // main thread

        // The timer cannot be started until QCoreApplication is instantiated.
        // Send ourselves an event and wait till it comes back, then start the timer.
        QCoreApplication::postEvent (this, new QEvent (QEvent::User), Qt::HighEventPriority);
    }

protected:
    void customEvent (QEvent * event)
        { startTimer (interval_ms); }

    void timerEvent (QTimerEvent * event)
    {
        if (! runner.run ())
            deleteLater ();
    }

private:
    int interval_ms;
    QueuedFuncRunner runner;
};

static void ADD_QUEUED (const QueuedFuncRunner & r)
    { QCoreApplication::postEvent (& router, new QueuedFuncEvent (r), Qt::HighEventPriority); }
static void ADD_TIMED (int i, const QueuedFuncRunner & r)
    { new QueuedFuncTimer (i, r); }

typedef QCoreApplication MainLoop;

static QCoreApplication * MAINLOOP_NEW ()
{
    int dummy_argc = 0;
    return new QCoreApplication (dummy_argc, nullptr);
}

#define MAINLOOP_RUN(m)      (m)->exec ()
#define MAINLOOP_QUIT(m)     (m)->quit ()
#define MAINLOOP_FREE(m)     delete (m)

#else // ! USE_QT

#include <glib.h>

static int queued_wrapper (void * ptr)
{
    auto runner = (QueuedFuncRunner *) ptr;
    runner->run ();
    delete runner;
    return false;
}

static int timed_wrapper (void * ptr)
{
    auto runner = (QueuedFuncRunner *) ptr;
    if (runner->run ())
        return true;

    delete runner;
    return false;
}

static void ADD_QUEUED (const QueuedFuncRunner & r)
    { g_idle_add_full (G_PRIORITY_HIGH, queued_wrapper, new QueuedFuncRunner (r), nullptr); }
static void ADD_TIMED (int i, const QueuedFuncRunner & r)
    { g_timeout_add (i, timed_wrapper, new QueuedFuncRunner (r)); }

typedef GMainLoop MainLoop;

#define MAINLOOP_NEW()       g_main_loop_new (nullptr, true)
#define MAINLOOP_RUN(m)      g_main_loop_run (m)
#define MAINLOOP_QUIT(m)     g_main_loop_quit (m)
#define MAINLOOP_FREE(m)     g_main_loop_unref (m)

#endif // ! USE_QT

EXPORT void QueuedFunc::queue (Func func, void * data)
{
    int new_serial = __sync_add_and_fetch (& serial, 1);
    ADD_QUEUED ({this, func, data, new_serial});
}

EXPORT void QueuedFunc::start (int interval_ms, Func func, void * data)
{
    _running = true;
    int new_serial = __sync_add_and_fetch (& serial, 1);
    ADD_TIMED (interval_ms, {this, func, data, new_serial});
}

EXPORT void QueuedFunc::stop ()
{
    __sync_add_and_fetch (& serial, 1);
    _running = false;
}

static MainLoop * mainloop;
static pthread_mutex_t mainloop_mutex = PTHREAD_MUTEX_INITIALIZER;

EXPORT void mainloop_run ()
{
    pthread_mutex_lock (& mainloop_mutex);

    if (! mainloop)
    {
        mainloop = MAINLOOP_NEW ();
        pthread_mutex_unlock (& mainloop_mutex);

        MAINLOOP_RUN (mainloop);

        pthread_mutex_lock (& mainloop_mutex);
        MAINLOOP_FREE (mainloop);
        mainloop = nullptr;
    }

    pthread_mutex_unlock (& mainloop_mutex);
}

EXPORT void mainloop_quit ()
{
    pthread_mutex_lock (& mainloop_mutex);

    if (mainloop)
        MAINLOOP_QUIT (mainloop);

    pthread_mutex_unlock (& mainloop_mutex);
}
