/*
 * art.c
 * Copyright 2011-2012 John Lindgren
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

#include "probe.h"
#include "internal.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "hook.h"
#include "mainloop.h"
#include "multihash.h"
#include "runtime.h"
#include "scanner.h"
#include "vfs.h"

#define FLAG_DONE 1
#define FLAG_SENT 2

struct AudArtItem {
    String filename;
    int refcount;
    int flag;

    /* album art as JPEG or PNG data */
    Index<char> data;

    /* album art as (possibly a temporary) file */
    String art_file;
    bool is_temp;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static SimpleHash<String, AudArtItem> art_items;
static AudArtItem * current_item;
static QueuedFunc queued_requests;

static Index<AudArtItem *> get_queued ()
{
    Index<AudArtItem *> queued;
    pthread_mutex_lock (& mutex);

    art_items.iterate ([&] (const String &, AudArtItem & item)
    {
        if (item.flag == FLAG_DONE)
        {
            queued.append (& item);
            item.flag = FLAG_SENT;
        }
    });

    queued_requests.stop ();

    pthread_mutex_unlock (& mutex);
    return queued;
}

static void send_requests (void *)
{
    auto queued = get_queued ();
    for (AudArtItem * item : queued)
    {
        hook_call ("art ready", (void *) (const char *) item->filename);
        aud_art_unref (item); /* release temporary reference */
    }
}

static void finish_item_locked (AudArtItem * item, Index<char> && data, String && art_file)
{
    /* already finished? */
    if (item->flag)
        return;

    item->data = std::move (data);
    item->art_file = std::move (art_file);
    item->flag = FLAG_DONE;

    queued_requests.queue (send_requests, nullptr);
}

static void request_callback (ScanRequest * request)
{
    pthread_mutex_lock (& mutex);

    AudArtItem * item = art_items.lookup (request->filename);

    if (item)
        finish_item_locked (item, std::move (request->image_data), std::move (request->image_file));

    pthread_mutex_unlock (& mutex);
}

static AudArtItem * art_item_get_locked (const String & filename, bool * queued)
{
    if (queued)
        * queued = false;

    // blacklist stdin
    if (! strncmp (filename, "stdin://", 8))
        return nullptr;

    AudArtItem * item = art_items.lookup (filename);

    if (item && item->flag)
    {
        item->refcount ++;
        return item;
    }

    if (! item)
    {
        item = art_items.add (filename, AudArtItem ());
        item->filename = filename;
        item->refcount = 1; /* temporary reference */

        scanner_request (new ScanRequest (filename, SCAN_IMAGE, request_callback));
    }

    if (queued)
        * queued = true;

    return nullptr;
}

static void art_item_unref_locked (AudArtItem * item)
{
    if (! -- item->refcount)
    {
        /* delete temporary file */
        if (item->art_file && item->is_temp)
        {
            StringBuf local = uri_to_filename (item->art_file);
            if (local)
                g_unlink (local);
        }

        art_items.remove (item->filename);
    }
}

static void clear_current_locked ()
{
    if (current_item)
    {
        art_item_unref_locked (current_item);
        current_item = nullptr;
    }
}

void art_cache_current (const String & filename, Index<char> && data, String && art_file)
{
    pthread_mutex_lock (& mutex);

    clear_current_locked ();

    AudArtItem * item = art_items.lookup (filename);

    if (! item)
    {
        item = art_items.add (filename, AudArtItem ());
        item->filename = filename;
        item->refcount = 1; /* temporary reference */
    }

    finish_item_locked (item, std::move (data), std::move (art_file));

    item->refcount ++;
    current_item = item;

    pthread_mutex_unlock (& mutex);
}

void art_clear_current ()
{
    pthread_mutex_lock (& mutex);
    clear_current_locked ();
    pthread_mutex_unlock (& mutex);
}

void art_cleanup ()
{
    auto queued = get_queued ();
    for (AudArtItem * item : queued)
        aud_art_unref (item); /* release temporary reference */

    /* playback should already be stopped */
    assert (! current_item);

    if (art_items.n_items ())
        AUDWARN ("Album art reference count not zero at exit!\n");
}

EXPORT AudArtPtr aud_art_request (const char * file, int format, bool * queued)
{
    pthread_mutex_lock (& mutex);

    String key (file);
    AudArtItem * item = art_item_get_locked (key, queued);
    bool good = true;

    if (! item)
        goto UNLOCK;

    if (format & AUD_ART_DATA)
    {
        /* load data from external image file */
        if (! item->data.len () && item->art_file)
        {
            VFSFile file (item->art_file, "r");
            if (file)
                item->data = file.read_all ();
        }

        if (! item->data.len ())
            good = false;
    }

    if (format & AUD_ART_FILE)
    {
        /* save data to temporary file */
        if (item->data.len () && ! item->art_file)
        {
            String local = write_temp_file (item->data.begin (), item->data.len ());
            if (local)
            {
                item->art_file = String (filename_to_uri (local));
                item->is_temp = true;
            }
        }

        if (! item->art_file)
            good = false;
    }

    if (! good)
    {
        art_item_unref_locked (item);
        item = nullptr;
    }

UNLOCK:
    pthread_mutex_unlock (& mutex);
    return AudArtPtr (item);
}

EXPORT const Index<char> * aud_art_data (const AudArtItem * item)
    { return & item->data; }
EXPORT const char * aud_art_file (const AudArtItem * item)
    { return item->art_file; }

EXPORT void aud_art_unref (AudArtItem * item)
{
    pthread_mutex_lock (& mutex);
    art_item_unref_locked (item);
    pthread_mutex_unlock (& mutex);
}
