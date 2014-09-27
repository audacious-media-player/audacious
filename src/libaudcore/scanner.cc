/*
 * scanner.c
 * Copyright 2012 John Lindgren
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

#include "scanner.h"

#include <glib.h>  /* for GThreadPool */

#include "internal.h"
#include "probe.h"
#include "tuple.h"

static GThreadPool * pool;

static void scan_worker (void * data, void *)
{
    ScanRequest * request = (ScanRequest *) data;

    if (! request->decoder)
        request->decoder = aud_file_find_decoder (request->filename, false, & request->error);

    if (request->decoder && (request->flags & SCAN_TUPLE))
        request->tuple = aud_file_read_tuple (request->filename, request->decoder, & request->error);

    if (request->decoder && (request->flags & SCAN_IMAGE))
    {
        request->image_data = aud_file_read_image (request->filename, request->decoder);

        if (! request->image_data.len ())
            request->image_file = art_search (request->filename);
    }

    request->callback (request);

    delete request;
}

void scanner_init ()
{
    pool = g_thread_pool_new (scan_worker, nullptr, SCAN_THREADS, false, nullptr);
}

void scanner_request (ScanRequest * request)
{
    g_thread_pool_push (pool, request, nullptr);
}

void scanner_cleanup ()
{
    g_thread_pool_free (pool, false, true);
}
