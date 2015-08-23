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

struct ArtItem {
    int refcount;
    int flag;

    /* album art as JPEG or PNG data */
    Index<char> data;

    /* album art as (possibly a temporary) file */
    String art_file;
    bool is_temp;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static SimpleHash<String, ArtItem> art_items;
static String current_ref;
static QueuedFunc queued_requests;

static void get_queued_cb (const String & key, ArtItem & item, void * list)
{
    if (item.flag == FLAG_DONE)
    {
        ((Index<String> *) list)->append (key);
        item.flag = FLAG_SENT;
    }
}

static Index<String> get_queued ()
{
    Index<String> queued;
    pthread_mutex_lock (& mutex);

    art_items.iterate (get_queued_cb, & queued);

    queued_requests.stop ();

    pthread_mutex_unlock (& mutex);
    return queued;
}

static void send_requests (void *)
{
    Index<String> queued = get_queued ();

    for (const String & file : queued)
    {
        hook_call ("art ready", (void *) (const char *) file);

        /* this hook is deprecated in 3.7 but kept for compatibility */
        if (file == current_ref)
            hook_call ("current art ready", (void *) (const char *) file);

        aud_art_unref (file); /* release temporary reference */
    }
}

static void finish_item (ArtItem * item, Index<char> && data, String && art_file)
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

    ArtItem * item = art_items.lookup (request->filename);

    if (item)
        finish_item (item, std::move (request->image_data), std::move (request->image_file));

    pthread_mutex_unlock (& mutex);
}

static ArtItem * art_item_get (const String & file, bool * queued)
{
    if (queued)
        * queued = false;

    // blacklist stdin
    if (! strncmp (file, "stdin://", 8))
        return nullptr;

    ArtItem * item = art_items.lookup (file);

    if (item && item->flag)
    {
        item->refcount ++;
        return item;
    }

    if (! item)
    {
        item = art_items.add (file, ArtItem ());
        item->refcount = 1; /* temporary reference */

        scanner_request (new ScanRequest (file, SCAN_IMAGE, request_callback));
    }

    if (queued)
        * queued = true;

    return nullptr;
}

static void art_item_unref (const String & file, ArtItem * item)
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

        art_items.remove (file);
    }
}

static void clear_current_locked ()
{
    if (current_ref)
    {
        ArtItem * item = art_items.lookup (current_ref);
        assert (item);

        art_item_unref (current_ref, item);
        current_ref = String ();
    }
}

void art_cache_current (const String & filename, Index<char> && data, String && art_file)
{
    pthread_mutex_lock (& mutex);

    clear_current_locked ();

    ArtItem * item = art_items.lookup (filename);

    if (! item)
    {
        item = art_items.add (filename, ArtItem ());
        item->refcount = 1; /* temporary reference */
    }

    finish_item (item, std::move (data), std::move (art_file));

    item->refcount ++;
    current_ref = filename;

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
    Index<String> queued = get_queued ();
    for (const String & file : queued)
        aud_art_unref (file); /* release temporary reference */

    /* playback should already be stopped */
    assert (! current_ref);

    if (art_items.n_items ())
        AUDWARN ("Album art reference count not zero at exit!\n");
}

EXPORT const Index<char> * aud_art_request_data (const char * file, bool * queued)
{
    const Index<char> * data = nullptr;
    pthread_mutex_lock (& mutex);

    String key (file);
    ArtItem * item = art_item_get (key, queued);

    if (! item)
        goto UNLOCK;

    /* load data from external image file */
    if (! item->data.len () && item->art_file)
    {
        VFSFile file (item->art_file, "r");
        if (file)
            item->data = file.read_all ();
    }

    if (item->data.len ())
        data = & item->data;
    else
        art_item_unref (key, item);

UNLOCK:
    pthread_mutex_unlock (& mutex);
    return data;
}

EXPORT const char * aud_art_request_file (const char * file, bool * queued)
{
    const char * art_file = nullptr;
    pthread_mutex_lock (& mutex);

    String key (file);
    ArtItem * item = art_item_get (key, queued);

    if (! item)
        goto UNLOCK;

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

    if (item->art_file)
        art_file = item->art_file;
    else
        art_item_unref (key, item);

UNLOCK:
    pthread_mutex_unlock (& mutex);
    return art_file;
}

EXPORT void aud_art_unref (const char * file)
{
    pthread_mutex_lock (& mutex);

    String key (file);
    ArtItem * item = art_items.lookup (key);
    assert (item);

    art_item_unref (key, item);

    pthread_mutex_unlock (& mutex);
}
