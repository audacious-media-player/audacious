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
#include "playlist.h"
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

    String current_wanted;
    if (! current_ref)
    {
        int playlist = aud_playlist_get_playing ();
        int entry = aud_playlist_get_position (playlist);
        current_wanted = aud_playlist_entry_get_filename (playlist, entry);
    }

    for (const String & file : queued)
    {
        hook_call ("art ready", (void *) (const char *) file);

        if (file == current_wanted)
        {
            hook_call ("current art ready", (void *) (const char *) file);
            current_ref = file;
        }
        else
            aud_art_unref (file); /* release temporary reference */
    }
}

static void request_callback (ScanRequest * request)
{
    pthread_mutex_lock (& mutex);

    ArtItem * item = art_items.lookup (request->filename);
    assert (item && ! item->flag);

    item->data = std::move (request->image_data);
    item->art_file = std::move (request->image_file);
    item->flag = FLAG_DONE;

    queued_requests.queue (send_requests, nullptr);

    pthread_mutex_unlock (& mutex);
}

static ArtItem * art_item_get (const String & file)
{
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

static void release_current (void)
{
    if (current_ref)
    {
        aud_art_unref (current_ref);
        current_ref = String ();
    }
}

void art_init (void)
{
    hook_associate ("playlist position", (HookFunction) release_current, nullptr);
    hook_associate ("playlist set playing", (HookFunction) release_current, nullptr);
}

void art_cleanup (void)
{
    hook_dissociate ("playlist position", (HookFunction) release_current);
    hook_dissociate ("playlist set playing", (HookFunction) release_current);

    Index<String> queued = get_queued ();
    for (const String & file : queued)
        aud_art_unref (file); /* release temporary reference */

    release_current ();

    if (art_items.n_items ())
        AUDWARN ("Album art reference count not zero at exit!\n");
}

EXPORT const Index<char> * aud_art_request_data (const char * file, bool * queued)
{
    const Index<char> * data = nullptr;
    pthread_mutex_lock (& mutex);

    String key (file);
    ArtItem * item = art_item_get (key);

    if (queued)
        * queued = ! item;

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
    ArtItem * item = art_item_get (key);

    if (queued)
        * queued = ! item;

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
