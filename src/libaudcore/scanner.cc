/*
 * scanner.c
 * Copyright 2012-2016 John Lindgren
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

#include "audstrings.h"
#include "cue-cache.h"
#include "i18n.h"
#include "internal.h"
#include "plugins.h"
#include "probe.h"
#include "tuple.h"
#include "vfs.h"

static GThreadPool * pool;

ScanRequest::ScanRequest (const String & filename, int flags, Callback callback,
 PluginHandle * decoder, Tuple && tuple) :
    filename (filename),
    flags (flags),
    callback (callback),
    decoder (decoder),
    tuple (std::move (tuple)),
    ip (nullptr)
{
    /* If this is a cuesheet entry (and it has not already been loaded), capture
     * a reference to the cache immediately.  During a playlist scan, requests
     * have overlapping lifecycles--each new ScanRequest is created by the
     * callback of the previous request--so the cached cuesheet persists as long
     * as consecutive playlist entries reference it. */
    if (! this->tuple.valid () && is_cuesheet_entry (filename))
        cue_cache.capture (new CueCacheRef (strip_subtune (filename)));
}

void ScanRequest::read_cuesheet_entry ()
{
    for (auto & item : cue_cache->load ())
    {
        if (item.filename == filename)
        {
            decoder = item.decoder;
            tuple = item.tuple.ref ();
            break;
        }
    }
}

void ScanRequest::run ()
{
    /* load cuesheet entry (possibly cached) */
    if (cue_cache)
        read_cuesheet_entry ();

    /* for a cuesheet entry, determine the source filename */
    String audio_file = tuple.get_str (Tuple::AudioFile);
    if (! audio_file)
        audio_file = filename;

    bool need_tuple = (flags & SCAN_TUPLE) && ! tuple.valid ();
    bool need_image = (flags & SCAN_IMAGE);

    if (! decoder)
        decoder = aud_file_find_decoder (audio_file, false, file, & error);
    if (! decoder)
        goto err;

    if (need_tuple || need_image)
    {
        if (! (ip = load_input_plugin (decoder, & error)))
            goto err;

        Tuple dummy_tuple;
        /* don't overwrite tuple if already valid (e.g. from a cuesheet) */
        Tuple & rtuple = need_tuple ? tuple : dummy_tuple;
        Index<char> * pimage = need_image ? & image_data : nullptr;
        if (! aud_file_read_tag (audio_file, decoder, file, rtuple, pimage, & error))
            goto err;

        if (need_image && ! image_data.len ())
            image_file = art_search (audio_file);
    }

    /* rewind/reopen the input file */
    if ((flags & SCAN_FILE))
        open_input_file (audio_file, "r", ip, file, & error);
    else
    {
    err:
        /* close file if not needed or if an error occurred */
        file = VFSFile ();
    }

    callback (this);
}

static void scan_worker (void * data, void *)
{
    ((ScanRequest *) data)->run ();
    delete (ScanRequest *) data;
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
