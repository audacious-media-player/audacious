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

struct ScanRequest {
    String filename;
    int flags;
    PluginHandle * decoder;
    ScanCallback callback;
    Tuple tuple;
    Index<char> image_data;
    String image_file;
};

static GThreadPool * pool;

ScanRequest * scan_request (const char * filename, int flags,
 PluginHandle * decoder, ScanCallback callback)
{
    ScanRequest * request = new ScanRequest ();

    request->filename = String (filename);
    request->flags = flags;
    request->decoder = decoder;
    request->callback = callback;

    g_thread_pool_push (pool, request, nullptr);

    return request;
}

static void scan_worker (void * data, void *)
{
    ScanRequest * request = (ScanRequest *) data;

    if (! request->decoder)
        request->decoder = aud_file_find_decoder (request->filename, false);

    if (request->decoder && (request->flags & SCAN_TUPLE))
        request->tuple = aud_file_read_tuple (request->filename, request->decoder);

    if (request->decoder && (request->flags & SCAN_IMAGE))
    {
        request->image_data = aud_file_read_image (request->filename, request->decoder);

        if (! request->image_data.len ())
            request->image_file = art_search (request->filename);
    }

    request->callback (request);

    delete request;
}

String scan_request_get_filename (ScanRequest * request)
{
    return request->filename;
}

PluginHandle * scan_request_get_decoder (ScanRequest * request)
{
    return request->decoder;
}

Tuple scan_request_get_tuple (ScanRequest * request)
{
    return std::move (request->tuple);
}

Index<char> scan_request_get_image_data (ScanRequest * request)
{
    return std::move (request->image_data);
}

String scan_request_get_image_file (ScanRequest * request)
{
    return request->image_file;
}

void scanner_init ()
{
    pool = g_thread_pool_new (scan_worker, nullptr, SCAN_THREADS, false, nullptr);
}

void scanner_cleanup ()
{
    g_thread_pool_free (pool, false, true);
}
