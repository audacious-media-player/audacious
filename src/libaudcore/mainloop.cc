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
#include <glib.h>

#ifdef USE_QT
#include <QCoreApplication>
#endif

#include "internal.h"
#include "multihash.h"
#include "runtime.h"

static pthread_once_t once = PTHREAD_ONCE_INIT;
static MultiHash * func_table = nullptr;

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

struct QueuedFuncHelper
{
    QueuedFuncParams params;
    QueuedFunc * queued = nullptr;

    QueuedFuncHelper (const QueuedFuncParams & params) :
        params (params) {}

    virtual ~QueuedFuncHelper () {}

    void run ();
    void start_for (QueuedFunc * queued_);

    virtual void start ();
    virtual void stop ();

private:
    int glib_source = 0;
};

struct QueuedFuncNode : public MultiHash::Node {
    QueuedFunc * queued;
    QueuedFuncHelper * helper;
};

void QueuedFuncHelper::run ()
{
    struct State {
        QueuedFuncHelper * helper;
        bool valid;
    };

    auto found_cb = [] (MultiHash::Node * node_, void * s_)
    {
        auto node = (const QueuedFuncNode *) node_;
        auto s = (State *) s_;

        s->valid = (node->helper == s->helper);

        if (! s->valid || ! s->helper->params.repeat)
            s->helper->stop ();

        return s->valid && ! s->helper->params.repeat;
    };

    State s = {this, false};
    func_table->lookup (queued, ptr_hash (queued), nullptr, found_cb, & s);

    if (s.valid)
        params.func (params.data);
}

void QueuedFuncHelper::start_for (QueuedFunc * queued_)
{
    queued = queued_;
    queued->_running = params.repeat;

    start ();
}

void QueuedFuncHelper::start ()
{
    auto callback = [] (void * me) -> gboolean {
        ((QueuedFuncHelper *) me)->run ();
        return G_SOURCE_CONTINUE;
    };

    auto destroy = [] (void * me) {
        delete (QueuedFuncHelper *) me;
    };

    glib_source = g_timeout_add_full (G_PRIORITY_HIGH, params.interval_ms,
     callback, this, destroy);
}

void QueuedFuncHelper::stop ()
{
    if (glib_source)
    {
        g_source_remove (glib_source);
        glib_source = 0;
    }
}

#ifdef USE_QT

class QueuedFuncEvent : public QueuedFuncHelper, public QEvent
{
public:
    QueuedFuncEvent (const QueuedFuncParams & params) :
        QueuedFuncHelper (params),
        QEvent (User) {}

    void start ();
    void stop () {}
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

class QueuedFuncTimer : public QueuedFuncHelper, public QObject
{
public:
    QueuedFuncTimer (const QueuedFuncParams & params) :
        QueuedFuncHelper (params) {}

    void start ()
    {
        moveToThread (router.thread ());  // main thread

        // The timer cannot be started until QCoreApplication is instantiated.
        // Send ourselves an event and wait till it comes back, then start the timer.
        QCoreApplication::postEvent (this, new QEvent (QEvent::User), Qt::HighEventPriority);
    }

    void stop ()
        { deleteLater (); }

protected:
    void customEvent (QEvent * event)
        { startTimer (params.interval_ms); }
    void timerEvent (QTimerEvent * event)
        { run (); }
};

#endif // USE_QT

static void create_func_table ()
{
    auto match_cb = [] (const MultiHash::Node * node_, const void * queued_) {
        auto node = (const QueuedFuncNode *) node_;
        return node->queued == (QueuedFunc *) queued_;
    };

    func_table = new MultiHash (match_cb);
}

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

void QueuedFunc::start (const QueuedFuncParams & params)
{
    auto add_cb = [] (const void * me_, void * helper_) {
        auto me = (QueuedFunc *) me_;
        auto helper = (QueuedFuncHelper *) helper_;

        auto node = new QueuedFuncNode;
        node->queued = me;
        node->helper = helper;

        helper->start_for (me);

        return (MultiHash::Node *) node;
    };

    auto replace_cb = [] (MultiHash::Node * node_, void * helper_) {
        auto node = (QueuedFuncNode *) node_;
        auto helper = (QueuedFuncHelper *) helper_;

        node->helper->stop ();
        node->helper = helper;

        helper->start_for (node->queued);

        return false; // do not remove
    };

    pthread_once (& once, create_func_table);
    func_table->lookup (this, ptr_hash (this), add_cb, replace_cb, create_helper (params));
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

EXPORT void QueuedFunc::stop ()
{
    auto remove_cb = [] (MultiHash::Node * node_, void *) {
        auto node = (QueuedFuncNode *) node_;

        node->helper->stop ();
        node->queued->_running = false;
        delete node;

        return true; // remove
    };

    pthread_once (& once, create_func_table);
    func_table->lookup (this, ptr_hash (this), nullptr, remove_cb, nullptr);
}

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
        }

        pthread_mutex_unlock (& mainloop_mutex);
        qt_mainloop->exec ();
    }
    else
#endif
    {
        if (! glib_mainloop)
            glib_mainloop = g_main_loop_new (nullptr, true);

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
