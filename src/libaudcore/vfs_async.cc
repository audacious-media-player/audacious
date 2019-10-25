/*
 * vfs_async.cc
 * Copyright 2010-2014 Ariadne Conill and John Lindgren
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

#include "list.h"
#include "mainloop.h"
#include "threads.h"
#include "vfs.h"
#include "vfs_async.h"

struct QueuedData : public ListNode
{
    const String filename;
    const VFSConsumer2 cons_f;

    std::thread thread;
    Index<char> buf;

    QueuedData (const char * filename, VFSConsumer2 cons_f) :
        filename (filename),
        cons_f (cons_f) {}
};

static QueuedFunc queued_func;
static List<QueuedData> queue;
static aud::mutex mutex;

static void send_data (void *)
{
    auto mh = mutex.take ();

    QueuedData * data;
    while ((data = queue.head ()))
    {
        queue.remove (data);

        mh.unlock ();

        data->thread.join ();
        data->cons_f (data->filename, data->buf);
        delete data;

        mh.lock ();
    }
}

static void read_worker (QueuedData * data)
{
    VFSFile file (data->filename, "r");
    if (file)
        data->buf = file.read_all ();

    auto mh = mutex.take ();

    if (! queue.head ())
        queued_func.queue (send_data, nullptr);

    queue.append (data);
}

EXPORT void vfs_async_file_get_contents (const char * filename, VFSConsumer2 cons_f)
{
    auto data = new QueuedData (filename, cons_f);
    data->thread = std::thread (read_worker, data);
}

EXPORT void vfs_async_file_get_contents (const char * filename, VFSConsumer cons_f, void * user)
{
    auto functor = [cons_f, user] (const char * filename, const Index<char> & buf)
        { cons_f (filename, buf, user); };

    vfs_async_file_get_contents (filename, functor);
}
