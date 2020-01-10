/*
 * timer.c
 * Copyright 2015 John Lindgren
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

#include "hook.h"
#include "index.h"
#include "internal.h"
#include "mainloop.h"
#include "runtime.h"
#include "threads.h"

static const aud::array<TimerRate, int> rate_to_ms = {1000, 250, 100, 33};

struct TimerItem
{
    TimerFunc func;
    void * data;
};

struct TimerList
{
    QueuedFunc source;
    Index<TimerItem> items;
    int use_count = 0;

    bool contains(TimerFunc func, void * data) const
    {
        for (auto & item : items)
        {
            if (item.func == func && item.data == data)
                return true;
        }

        return false;
    }

    void check_stop()
    {
        if (!use_count)
        {
            auto is_empty = [](const TimerItem & item) { return !item.func; };

            items.remove_if(is_empty, true);

            if (!items.len() && source.running())
                source.stop();
        }
    }
};

static aud::mutex mutex;
static aud::array<TimerRate, TimerList> lists;

static void timer_run(void * list_)
{
    auto & list = *(TimerList *)list_;
    auto mh = mutex.take();

    list.use_count++;

    /* note: the list may grow (but not shrink) during the call */
    for (int i = 0; i < list.items.len(); i++)
    {
        /* copy locally to prevent race condition */
        TimerItem item = list.items[i];

        if (item.func)
        {
            mh.unlock();
            item.func(item.data);
            mh.lock();
        }
    }

    list.use_count--;
    list.check_stop();
}

EXPORT void timer_add(TimerRate rate, TimerFunc func, void * data)
{
    auto & list = lists[rate];
    auto mh = mutex.take();

    if (!list.contains(func, data))
    {
        list.items.append(func, data);

        if (!list.source.running())
            list.source.start(rate_to_ms[rate], timer_run, &list);
    }
}

EXPORT void timer_remove(TimerRate rate, TimerFunc func, void * data)
{
    auto & list = lists[rate];
    auto mh = mutex.take();

    for (TimerItem & item : list.items)
    {
        if (item.func == func && (!data || item.data == data))
            item.func = nullptr;
    }

    list.check_stop();
}

void timer_cleanup()
{
    auto mh = mutex.take();

    int timers_running = 0;
    for (TimerList & list : lists)
        timers_running += list.items.len();

    if (timers_running)
        AUDWARN("%d timers still registered at exit\n", timers_running);
}
