/*
 * mainloop.cc
 * Copyright 2014-2017 John Lindgren
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

#include <glib.h>

#ifdef USE_QT
#include <QCoreApplication>
#endif

#include "internal.h"
#include "multihash.h"
#include "runtime.h"

struct QueuedFuncParams
{
    QueuedFunc::Func2 func;
    int interval_ms;
    bool repeat;
};

// QueuedFunc itself is a tiny handle/identifier object; most of the related
// code lives in various "helper" objects.  This is the base class inherited
// by all the different types of helpers.

struct QueuedFuncHelper
{
    QueuedFunc * queued;
    QueuedFuncParams params;

    // Creates an appropriate helper subclass for the given parameters and
    // schedules it to run the QueuedFunc
    static QueuedFuncHelper * create(QueuedFunc * queued,
                                     const QueuedFuncParams & params);

    // Callback which runs the QueuedFunc, if still active
    void run();

    // Cancels any scheduled run of the QueuedFunc and marks the helper for
    // deletion (it may not deleted immediately, but should not be accessed
    // again after calling this function)
    virtual void cancel() = 0;

    virtual ~QueuedFuncHelper() {}

protected:
    QueuedFuncHelper(QueuedFunc * queued, const QueuedFuncParams & params)
        : queued(queued), params(params)
    {
    }
};

// The following hash table implements a thread-safe "registry" of active
// QueuedFuncs.  This allows a callback executing in the main thread to safely
// look up the information it needs, without accessing the QueuedFunc directly
// and risking use-after-free.

struct QueuedFuncNode : public MultiHash::Node
{
    // Creates a helper to be registered in the hash
    QueuedFuncNode(QueuedFunc * queued, const QueuedFuncParams & params)
        : helper(QueuedFuncHelper::create(queued, params))
    {
    }

    // Cancels a helper when it is unregistered from the hash
    ~QueuedFuncNode() { helper->cancel(); }

    // Replaces the registration of one helper with another
    void reset(QueuedFunc * queued, const QueuedFuncParams & params)
    {
        helper->cancel();
        helper = QueuedFuncHelper::create(queued, params);
    }

    // Checks whether a helper is still registered
    bool is_current(QueuedFuncHelper * test_helper) const
    {
        return test_helper == helper;
    }

    // Hash comparison function
    bool match(const QueuedFunc * queued) const
    {
        return queued == helper->queued;
    }

private:
    QueuedFuncHelper * helper;
};

static MultiHash_T<QueuedFuncNode, QueuedFunc> func_table;
static bool in_lockdown = false;

// Helper logic common between GLib and Qt

// "run" logic executed within the hash table lock
struct RunCheck
{
    QueuedFuncHelper * helper;
    bool okay_to_run;

    // Called if the QueuedFunc is not registered in the hash
    // This indicates a "stale" event that should not be processed
    QueuedFuncNode * add(const QueuedFunc *) { return nullptr; }

    bool found(const QueuedFuncNode * node)
    {
        // Check whether a different helper has been registered
        // This also indicates a "stale" event that should not be processed
        if (!node->is_current(helper))
            return false;

        // We are still registered and good to go
        okay_to_run = true;

        // Leave a periodic timer registered
        if (helper->params.repeat)
            return false;

        // Unregister and cancel a one-time callback
        delete node;
        return true;
    }
};

void QueuedFuncHelper::run()
{
    // Check whether it's okay to run.  The actual check is performed within
    // the MultiHash lock to eliminate race conditions.
    RunCheck r = {this, false};
    func_table.lookup(queued, ptr_hash(queued), r);

    if (r.okay_to_run)
        params.func();
}

// GLib implementation -- simple wrapper around g_timeout_add_full()

class HelperGLib : public QueuedFuncHelper
{
public:
    HelperGLib(QueuedFunc * queued, const QueuedFuncParams & params)
        : QueuedFuncHelper(queued, params)
    {
        glib_source =
            g_timeout_add_full(G_PRIORITY_HIGH, params.interval_ms, run_cb,
                               this, aud::delete_obj<HelperGLib>);
    }

    void cancel()
    {
        // GLib will delete the helper after we return to the main loop
        g_source_remove(glib_source);
    }

private:
    static gboolean run_cb(void * me)
    {
        (static_cast<HelperGLib *>(me))->run();
        return G_SOURCE_CONTINUE;
    }

    int glib_source = 0;
};

#ifdef USE_QT

// Qt implementation -- rather more complicated

// One-time callbacks are implemented through custom events posted to a "router"
// object.  In this case, the QueuedFuncHelper class is the QEvent itself.  The
// router object is created in, and associated with, the main thread, thus
// events are executed in the main thread regardless of their origin.  Events
// can be queued even before the QApplication has been created.  Note that
// QEvents cannot be cancelled once posted; therefore it's necessary to check
// on our side that the QueuedFunc is still active when the event is received.

class EventRouter : public QObject
{
protected:
    void customEvent(QEvent * event);
};

static EventRouter router;

class HelperQEvent : public QueuedFuncHelper, public QEvent
{
public:
    HelperQEvent(QueuedFunc * queued, const QueuedFuncParams & params)
        : QueuedFuncHelper(queued, params), QEvent(User)
    {
        QCoreApplication::postEvent(&router, this, Qt::HighEventPriority);
    }

    void cancel() {} // Qt will delete the event after it fires
};

void EventRouter::customEvent(QEvent * event)
{
    dynamic_cast<HelperQEvent *>(event)->run();
}

// Periodic callbacks are implemented through QObject's timer capability.  In
// this case, the QueuedFuncHelper is a QObject that is re-associated with the
// main thread immediately after creation.  The timer is started in a roundabout
// fashion due to the fact that QObject::startTimer() does not work until the
// QApplication has been created.  To work around this, a single QEvent is first
// posted.  When this initial event is executed (in the main thread), we know
// that the QApplication is up.  We then start the timer and run our own
// callback from the QTimerEvents we receive.

class HelperQTimer : public QueuedFuncHelper, public QObject
{
public:
    HelperQTimer(QueuedFunc * queued, const QueuedFuncParams & params)
        : QueuedFuncHelper(queued, params)
    {
        moveToThread(router.thread()); // main thread
        QCoreApplication::postEvent(this, new QEvent(QEvent::User),
                                    Qt::HighEventPriority);
    }

    void cancel() { deleteLater(); }

protected:
    void customEvent(QEvent *) { startTimer(params.interval_ms); }
    void timerEvent(QTimerEvent *) { run(); }
};

#endif // USE_QT

// creates the appropriate helper subclass
QueuedFuncHelper * QueuedFuncHelper::create(QueuedFunc * queued,
                                            const QueuedFuncParams & params)
{
#ifdef USE_QT
    if (aud_get_mainloop_type() == MainloopType::Qt)
    {
        if (params.interval_ms > 0)
            return new HelperQTimer(queued, params);
        else
            return new HelperQEvent(queued, params);
    }
#endif

    return new HelperGLib(queued, params);
}

// "start" logic executed within the hash table lock
struct Starter
{
    QueuedFunc * queued;
    const QueuedFuncParams & params;

    // register a new helper for this QueuedFunc
    QueuedFuncNode * add(const QueuedFunc *)
    {
        return in_lockdown ? nullptr : new QueuedFuncNode(queued, params);
    }

    // cancel the old helper and register a replacement
    bool found(QueuedFuncNode * node)
    {
        node->reset(queued, params);
        return false;
    }
};

// common entry point used by all queue() and start() variants
static void start_func(QueuedFunc * queued, const QueuedFuncParams & params)
{
    Starter s = {queued, params};
    func_table.lookup(queued, ptr_hash(queued), s);
}

EXPORT void QueuedFunc::queue(Func2 func)
{
    start_func(this, {func, 0, false});
    _running = false;
}

EXPORT void QueuedFunc::queue(Func func, void * data)
{
    queue(std::bind(func, data));
}

EXPORT void QueuedFunc::queue(int delay_ms, Func2 func)
{
    g_return_if_fail(delay_ms >= 0);
    start_func(this, {func, delay_ms, false});
    _running = false;
}

EXPORT void QueuedFunc::queue(int delay_ms, Func func, void * data)
{
    queue(delay_ms, std::bind(func, data));
}

EXPORT void QueuedFunc::start(int interval_ms, Func2 func)
{
    g_return_if_fail(interval_ms > 0);
    start_func(this, {func, interval_ms, true});
    _running = true;
}

EXPORT void QueuedFunc::start(int interval_ms, Func func, void * data)
{
    start(interval_ms, std::bind(func, data));
}

// "stop" logic executed within the hash table lock
struct Stopper
{
    // not registered, do nothing
    QueuedFuncNode * add(const QueuedFunc *) { return nullptr; }

    // unregister and cancel helper
    bool found(QueuedFuncNode * node)
    {
        delete node;
        return true;
    }
};

EXPORT void QueuedFunc::stop()
{
    Stopper s;
    func_table.lookup(this, ptr_hash(this), s);
    _running = false;
}

// unregister a pending callback at shutdown
static bool cleanup_node(QueuedFuncNode * node)
{
    delete node;
    return true;
}

// inhibit all future callbacks at shutdown
static void enter_lockdown() { in_lockdown = true; }

EXPORT void QueuedFunc::inhibit_all()
{
    func_table.iterate(cleanup_node, enter_lockdown);
}

// main loop implementation follows

static GMainLoop * glib_mainloop;

EXPORT void mainloop_run()
{
#ifdef USE_QT
    if (aud_get_mainloop_type() == MainloopType::Qt)
    {
        static char app_name[] = "audacious";
        static int dummy_argc = 1;
        static char * dummy_argv[] = {app_name, nullptr};

        // Create QCoreApplication instance if running in headless mode
        // (otherwise audqt will have created a QApplication already).
        if (!qApp)
            new QCoreApplication(dummy_argc, dummy_argv);

        qApp->exec();
    }
    else
#endif
    {
        glib_mainloop = g_main_loop_new(nullptr, true);
        g_main_loop_run(glib_mainloop);
        g_main_loop_unref(glib_mainloop);
        glib_mainloop = nullptr;
    }
}

EXPORT void mainloop_quit()
{
#ifdef USE_QT
    if (aud_get_mainloop_type() == MainloopType::Qt)
    {
        qApp->quit();
    }
    else
#endif
    {
        g_main_loop_quit(glib_mainloop);
    }
}

void mainloop_cleanup()
{
#ifdef USE_QT
    if (aud_get_mainloop_type() == MainloopType::Qt)
    {
        delete qApp;
    }
#endif
}
