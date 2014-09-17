/*
 * vfs_async.cc
 * Copyright 2010-2014 William Pitcock and John Lindgren
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

#include "list.h"
#include "mainloop.h"
#include "vfs.h"
#include "vfs_async.h"

struct QueuedData : public ListNode
{
    const String filename;
    const VFSConsumer cons_f;
    void * const user;

    pthread_t thread;

    Index<char> buf;

    QueuedData (const char * filename, VFSConsumer cons_f, void * user) :
        filename (filename),
        cons_f (cons_f),
        user (user) {}
};

static QueuedFunc queued_func;
static List<QueuedData> queue;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void send_data (void *)
{
    pthread_mutex_lock (& mutex);

    QueuedData * data;
    while ((data = queue.head ()))
    {
        queue.remove (data);

        pthread_mutex_unlock (& mutex);

        pthread_join (data->thread, nullptr);
        data->cons_f (data->filename, data->buf, data->user);
        delete data;

        pthread_mutex_lock (& mutex);
    }

    pthread_mutex_unlock (& mutex);
}

static void * read_worker (void * data0)
{
    auto data = (QueuedData *) data0;

    VFSFile file (data->filename, "r");
    if (file)
        data->buf = file.read_all ();

    pthread_mutex_lock (& mutex);

    if (! queue.head ())
        queued_func.queue (send_data, nullptr);

    queue.append (data);

    pthread_mutex_unlock (& mutex);
    return nullptr;
}

EXPORT void vfs_async_file_get_contents (const char * filename, VFSConsumer cons_f, void * user)
{
    auto data = new QueuedData (filename, cons_f, user);
    pthread_create (& data->thread, nullptr, read_worker, data);
}
