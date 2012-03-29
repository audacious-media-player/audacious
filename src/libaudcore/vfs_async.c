/*
 * vfs_async.c
 * Copyright 2010 William Pitcock
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

#include <glib.h>

#include "config.h"
#include "vfs_async.h"

typedef struct {
    char *filename;
    void *buf;
    int64_t size;
    GThread *thread;
    void * userdata;

    VFSConsumer cons_f;
} VFSAsyncTrampoline;

bool_t
vfs_async_file_get_contents_trampoline(void * data)
{
    VFSAsyncTrampoline *tr = data;

    tr->cons_f(tr->buf, tr->size, tr->userdata);
    g_slice_free(VFSAsyncTrampoline, tr);

    return FALSE;
}

void *
vfs_async_file_get_contents_worker(void * data)
{
    VFSAsyncTrampoline *tr = data;

    vfs_file_get_contents(tr->filename, &tr->buf, &tr->size);

    g_idle_add_full(G_PRIORITY_HIGH_IDLE, vfs_async_file_get_contents_trampoline, tr, NULL);
    g_thread_exit(NULL);
    return NULL;
}

EXPORT void
vfs_async_file_get_contents(const char *filename, VFSConsumer cons_f, void * userdata)
{
    VFSAsyncTrampoline *tr;

    tr = g_slice_new0(VFSAsyncTrampoline);
    tr->filename = g_strdup(filename);
    tr->cons_f = cons_f;
    tr->userdata = userdata;
    tr->thread = g_thread_create(vfs_async_file_get_contents_worker, tr, FALSE, NULL);
}
