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

#include "i18n.h"
#include "internal.h"
#include "plugins.h"
#include "tuple.h"
#include "vfs.h"

static GThreadPool * pool;

static void scan_worker (void * data, void *)
{
    auto r = (ScanRequest *) data;

    if (! r->decoder)
        r->decoder = file_find_decoder (r->filename, false, r->file, & r->error);
    if (! r->decoder)
        goto err;

    if ((r->flags & (SCAN_TUPLE | SCAN_IMAGE)))
    {
        if (! (r->ip = (InputPlugin *) aud_plugin_get_header (r->decoder)))
        {
            r->error = String (_("Error loading plugin"));
            goto err;
        }

        if (! open_input_file (r->filename, "r", r->ip, r->file, & r->error))
            goto err;
    }

    if ((r->flags & SCAN_TUPLE))
        r->tuple = file_read_tuple (r->filename, r->decoder, r->file, & r->error);

    if ((r->flags & SCAN_IMAGE))
    {
        r->image_data = file_read_image (r->filename, r->decoder, r->file);
        if (! r->image_data.len ())
            r->image_file = art_search (r->filename);
    }

err:
    r->callback (r);

    delete r;
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
