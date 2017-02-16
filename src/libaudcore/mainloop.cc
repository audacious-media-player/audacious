/*
 * mainloop.cc
 * Copyright 2014-2015 John Lindgren
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

#include <pthread.h>
#include <stdlib.h>

#include <glib.h>

#ifdef USE_QT
#include <QCoreApplication>
#endif

#include "internal.h"
#include "multihash.h"
#include "runtime.h"

static pthread_mutex_t mainloop_mutex = PTHREAD_MUTEX_INITIALIZER;
static GMainLoop * glib_mainloop;

#ifdef USE_QT
static QCoreApplication * qt_mainloop;
#endif

struct QueuedFuncParams {
    QueuedFunc::Func func;
    void * data;
    int interval_ms;
    bool repeat;
};

// Helper class that is created when the QueuedFunc is activated.  The base
// class contains the GLib implementation.  The Qt implementation, which is more
// complex, is contained in the QueuedFuncEvent and QueuedFuncTimer subclasses.

struct QueuedFuncHelper
{
    QueuedFuncParams params;
    QueuedFunc * queued = nullptr;

    QueuedFuncHelper (const QueuedFuncParams & params) :
        params (params) {}

    virtual ~QueuedFuncHelper () {}

    void run ();
    void start_for (QueuedFunc * queued_);

    virtual bool can_stop ()
        { return true; }

    virtual void start ();
    virtual void stop ();

private:
    struct RunCheck;

    int glib_source = 0;
};

// The following hash table implements a thread-safe "registry" of active
// QueuedFuncs.  This allows a callback executing in the main thread to safely
// look up the information it needs, without accessing the QueuedFunc directly
// and risking use-after-free.

struct QueuedFuncNode : public MultiHash::Node
{
    QueuedFunc * queued;
    QueuedFuncHelper * helper;
    bool can_stop;

    bool match (const QueuedFunc * q) const
        { return q == queued; }
};

static MultiHash_T<QueuedFuncNode, QueuedFunc> func_table;

// Helper logic common between GLib and Qt

// "run" logic executed within the hash table lock
struct QueuedFuncHelper::RunCheck
{
    QueuedFuncHelper * helper;
    bool valid;

    QueuedFuncNode * add (const QueuedFunc *)
        { return nullptr; }

    bool found (const QueuedFuncNode * node)
    {
        // Check that the same helper is installed as when the function was
        // first queued.  If a new helper is installed, then we are processing
        // a "stale" event and should not run the callback.
        valid = (node->helper == helper);

        // Uninstall the QueuedFunc and helper if this is a one-time (non-
        // periodic) callback.  Do NOT uninstall the QueuedFunc if has already
        // been reinstalled with a new helper (see previous comment).
        bool remove = valid && ! helper->params.repeat;

        // Clean up the helper if this is a one-time (non-periodic) callback OR
        // a new helper has already been installed.  No fields or methods of the
        // helper may be accessed after this point.
        if (! valid || ! helper->params.repeat)
            helper->stop ();

        return remove;
    }
};

void QueuedFuncHelper::run ()
{
    // Check whether it's okay to run.  The actual check is performed within
    // the MultiHash lock to eliminate race conditions.
    RunCheck r = {this, false};
    func_table.lookup (queued, ptr_hash (queued), r);

    if (r.valid)
        params.func (params.data);
}

void QueuedFuncHelper::start_for (QueuedFunc * queued_)
{
    queued = queued_;
    queued->_running = params.repeat;

    start (); // branch to GLib or Qt implementation
}

// GLib implementation -- simple wrapper around g_timeout_add_full()

void QueuedFuncHelper::start ()
{
    auto callback = [] (void * me) -> gboolean {
        (static_cast<QueuedFuncHelper *> (me))->run ();
        return G_SOURCE_CONTINUE;
    };

    glib_source = g_timeout_add_full (G_PRIORITY_HIGH, params.interval_ms,
     callback, this, aud::delete_obj<QueuedFuncHelper>);
}

void QueuedFuncHelper::stop ()
{
    if (glib_source)
        g_source_remove (glib_source);  // deletes the QueuedFuncHelper
}

#ifdef USE_QT

// Qt implementation -- rather more complicated

// One-time callbacks are implemented through custom events posted to a "router"
// object.  In this case, the QueuedFuncHelper class is the QEvent itself.  The
// router object is created in, and associated with, the main thread, thus
// events are executed in the main thread regardless of their origin.  Events
// can be queued even before the QApplication has been created.  Note that
// QEvents cannot be cancelled once posted; therefore it's necessary to check
// on our side that the QueuedFunc is still active when the event is received.

class QueuedFuncEvent : public QueuedFuncHelper, public QEvent
{
public:
    QueuedFuncEvent (const QueuedFuncParams & params) :
        QueuedFuncHelper (params),
        QEvent (User) {}

    bool can_stop ()
        { return false; }

    void start ();
};

class QueuedFuncRouter : public QObject
{
protected:
    void customEvent (QEvent * event)
        { dynamic_cast<QueuedFuncEvent *> (event)->run (); }
};

static QueuedFuncRouter router;

void QueuedFuncEvent::start ()
{
    QCoreApplication::postEvent (& router, this, Qt::HighEventPriority);
}

// Periodic callbacks are implemented through QObject's timer capability.  In
// this case, the QueuedFuncHelper is a QObject that is re-associated with the
// main thread immediately after creation.  The timer is started in a roundabout
// fashion due to the fact that QObject::startTimer() does not work until the
// QApplication has been created.  To work around this, a single QEvent is first
// posted.  When this initial event is executed (in the main thread), we know
// that the QApplication is up.  We then start the timer and run our own
// callback from the QTimerEvents we receive.

class QueuedFuncTimer : public QueuedFuncHelper, public QObject
{
public:
    QueuedFuncTimer (const QueuedFuncParams & params) :
        QueuedFuncHelper (params) {}

    void start ()
    {
        moveToThread (router.thread ());  // main thread
        QCoreApplication::postEvent (this, new QEvent (QEvent::User), Qt::HighEventPriority);
    }

    void stop ()
        { deleteLater (); }

protected:
    void customEvent (QEvent *)
        { startTimer (params.interval_ms); }
    void timerEvent (QTimerEvent *)
        { run (); }
};

#endif // USE_QT

// creates the appropriate helper subclass
static QueuedFuncHelper * create_helper (const QueuedFuncParams & params)
{
#ifdef USE_QT
    if (aud_get_mainloop_type () == MainloopType::Qt)
    {
        if (params.interval_ms > 0)
            return new QueuedFuncTimer (params);
        else
            return new QueuedFuncEvent (params);
    }
#endif

    return new QueuedFuncHelper (params);
}

// "start" logic executed within the hash table lock
struct QueuedFunc::Starter
{
    QueuedFunc * queued;
    QueuedFuncHelper * helper;

    // QueuedFunc not yet installed
    // install, then create a helper and start it
    QueuedFuncNode * add (const QueuedFunc *)
    {
        auto node = new QueuedFuncNode;
        node->queued = queued;
        node->helper = helper;
        node->can_stop = helper->can_stop ();

        helper->start_for (queued);

        return node;
    }

    // QueuedFunc already installed
    // first clean up the existing helper
    // then create a new helper and start it
    bool found (QueuedFuncNode * node)
    {
        if (node->can_stop)
            node->helper->stop ();

        node->helper = helper;
        node->can_stop = helper->can_stop ();

        helper->start_for (queued);

        return false; // do not remove
    }
};

// common entry point used by all queue() and start() variants
void QueuedFunc::start (const QueuedFuncParams & params)
{
    Starter s = {this, create_helper (params)};
    func_table.lookup (this, ptr_hash (this), s);
}

EXPORT void QueuedFunc::queue (Func func, void * data)
{
    start ({func, data, 0, false});
}

EXPORT void QueuedFunc::queue (int delay_ms, Func func, void * data)
{
    g_return_if_fail (delay_ms >= 0);
    start ({func, data, delay_ms, false});
}

EXPORT void QueuedFunc::start (int interval_ms, Func func, void * data)
{
    g_return_if_fail (interval_ms > 0);
    start ({func, data, interval_ms, true});
}

// "stop" logic executed within the hash table lock
struct QueuedFunc::Stopper
{
    // not installed, do nothing
    QueuedFuncNode * add (const QueuedFunc *)
        { return nullptr; }

    // installed, clean up the helper and uninstall
    bool found (QueuedFuncNode * node)
    {
        if (node->can_stop)
            node->helper->stop ();

        node->queued->_running = false;
        delete node;

        return true; // remove
    }
};

EXPORT void QueuedFunc::stop ()
{
    Stopper s;
    func_table.lookup (this, ptr_hash (this), s);
}

// main loop implementation follows

EXPORT void mainloop_run ()
{
    pthread_mutex_lock (& mainloop_mutex);

#ifdef USE_QT
    if (aud_get_mainloop_type () == MainloopType::Qt)
    {
        if (! qt_mainloop)
        {
            static char app_name[] = "audacious";
            static int dummy_argc = 1;
            static char * dummy_argv[] = {app_name, nullptr};

            qt_mainloop = new QCoreApplication (dummy_argc, dummy_argv);
            atexit ([] () { delete qt_mainloop; });
        }

        pthread_mutex_unlock (& mainloop_mutex);
        qt_mainloop->exec ();
    }
    else
#endif
    {
        if (! glib_mainloop)
        {
            glib_mainloop = g_main_loop_new (nullptr, true);
            atexit ([] () { g_main_loop_unref (glib_mainloop); });
        }

        pthread_mutex_unlock (& mainloop_mutex);
        g_main_loop_run (glib_mainloop);
    }
}

EXPORT void mainloop_quit ()
{
    pthread_mutex_lock (& mainloop_mutex);

#ifdef USE_QT
    if (aud_get_mainloop_type () == MainloopType::Qt)
    {
        if (qt_mainloop)
            qt_mainloop->quit ();
    }
    else
#endif
    {
        if (glib_mainloop)
            g_main_loop_quit (glib_mainloop);
    }

    pthread_mutex_unlock (& mainloop_mutex);
}
