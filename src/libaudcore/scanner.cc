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

#include <glib.h>

#include "internal.h"
#include "probe.h"
#include "tuple.h"

struct ScanRequest {
    char * filename; /* pooled */
    int flags;
    PluginHandle * decoder;
    ScanCallback callback;
    Tuple * tuple;
    void * image_data;
    int64_t image_len;
    char * image_file; /* pooled */
};

static GThreadPool * pool;

ScanRequest * scan_request (const char * filename, int flags,
 PluginHandle * decoder, ScanCallback callback)
{
    ScanRequest * request = g_slice_new0 (ScanRequest);

    request->filename = str_get (filename);
    request->flags = flags;
    request->decoder = decoder;
    request->callback = callback;

    g_thread_pool_push (pool, request, NULL);

    return request;
}

static void scan_worker (void * data, void * unused)
{
    ScanRequest * request = (ScanRequest *) data;

    if (! request->decoder)
        request->decoder = aud_file_find_decoder (request->filename, FALSE);

    if (request->decoder && (request->flags & SCAN_TUPLE))
        request->tuple = aud_file_read_tuple (request->filename, request->decoder);

    if (request->decoder && (request->flags & SCAN_IMAGE))
    {
        aud_file_read_image (request->filename, request->decoder,
         & request->image_data, & request->image_len);

        if (! request->image_data)
            request->image_file = art_search (request->filename);
    }

    request->callback (request);

    tuple_unref (request->tuple);
    str_unref (request->filename);
    g_free (request->image_data);
    str_unref (request->image_file);

    g_slice_free (ScanRequest, request);
}

const char * scan_request_get_filename (ScanRequest * request)
{
    return request->filename;
}

PluginHandle * scan_request_get_decoder (ScanRequest * request)
{
    return request->decoder;
}

Tuple * scan_request_get_tuple (ScanRequest * request)
{
    Tuple * tuple = request->tuple;
    request->tuple = NULL;
    return tuple;
}

void scan_request_get_image_data (ScanRequest * request, void * * data, int64_t * len)
{
    * data = request->image_data;
    * len = request->image_len;
    request->image_data = NULL;
    request->image_len = 0;
}

const char * scan_request_get_image_file (ScanRequest * request)
{
    return request->image_file;
}

void scanner_init (void)
{
    pool = g_thread_pool_new (scan_worker, NULL, SCAN_THREADS, FALSE, NULL);
}

void scanner_cleanup (void)
{
    g_thread_pool_free (pool, FALSE, TRUE);
}
