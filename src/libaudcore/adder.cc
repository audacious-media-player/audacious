/*
 * adder.c
 * Copyright 2011-2013 John Lindgren
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

#include "playlist-internal.h"
#include "internal.h"

#include <pthread.h>
#include <string.h>
#include <sys/stat.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "drct.h"
#include "hook.h"
#include "i18n.h"
#include "plugins-internal.h"
#include "probe.h"
#include "runtime.h"
#include "tuple.h"
#include "vfs.h"

struct AddTask
{
    int playlist_id, at;
    bool_t play;
    Index<PlaylistAddItem> items;
    PlaylistFilterFunc filter;
    void * user;
};

struct AddResult {
    int playlist_id, at;
    bool_t play;
    String title;
    Index<PlaylistAddItem> items;
};

static void * add_worker (void * unused);

static GList * add_tasks = NULL;
static GList * add_results = NULL;
static int current_playlist_id = -1;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool_t add_thread_started = FALSE;
static bool_t add_thread_exited = FALSE;
static pthread_t add_thread;
static int add_source = 0;

static int status_source = 0;
static char status_path[512];
static int status_count;

static bool_t status_cb (void * unused)
{
    pthread_mutex_lock (& mutex);

    char scratch[128];
    snprintf (scratch, sizeof scratch, dngettext (PACKAGE, "%d file found",
     "%d files found", status_count), status_count);

    if (aud_get_headless_mode ())
    {
        printf ("Searching, %s ...\r", scratch);
        fflush (stdout);
    }
    else
    {
        hook_call ("ui show progress", status_path);
        hook_call ("ui show progress 2", scratch);
    }

    pthread_mutex_unlock (& mutex);
    return TRUE;
}

static void status_update (const char * filename, int found)
{
    pthread_mutex_lock (& mutex);

    snprintf (status_path, sizeof status_path, "%s", filename);
    status_count = found;

    if (! status_source)
        status_source = g_timeout_add (250, status_cb, NULL);

    pthread_mutex_unlock (& mutex);
}

static void status_done_locked (void)
{
    if (status_source)
    {
        g_source_remove (status_source);
        status_source = 0;
    }

    if (aud_get_headless_mode ())
        printf ("\n");
    else
        hook_call ("ui hide progress", NULL);
}

static void add_file (const String & filename, Tuple && tuple,
 PluginHandle * decoder, PlaylistFilterFunc filter, void * user,
 AddResult * result, bool_t validate)
{
    g_return_if_fail (filename);

    if (filter && ! filter (filename, user))
        return;

    status_update (filename, result->items.len ());

    if (! tuple && ! decoder)
    {
        decoder = aud_file_find_decoder (filename, TRUE);
        if (validate && ! decoder)
            return;
    }

    if (! tuple && decoder && input_plugin_has_subtunes (decoder) && ! strchr (filename, '?'))
        tuple = aud_file_read_tuple (filename, decoder);

    int n_subtunes = tuple.get_n_subtunes ();

    if (n_subtunes)
    {
        for (int sub = 0; sub < n_subtunes; sub ++)
        {
            String subname = str_printf ("%s?%d", (const char *) filename,
             tuple.get_nth_subtune (sub));
            add_file (subname, Tuple (), decoder, filter, user, result, FALSE);
        }

        return;
    }

    result->items.append ({filename, std::move (tuple), decoder});
}

static int compare_wrapper (const String & a, const String & b, void *)
{
    return str_compare (a, b);
}

static void add_folder (const String & filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool_t is_single)
{
    String path;
    Index<String> files;
    GDir * folder;

    g_return_if_fail (filename);

    if (filter && ! filter (filename, user))
        return;

    status_update (filename, result->items.len ());

    path = uri_to_filename (filename);
    if (! path)
        return;

    if (! (folder = g_dir_open (path, 0, NULL)))
        return;

    const char * name;
    while ((name = g_dir_read_name (folder)))
        files.append (filename_build (path, name));

    g_dir_close (folder);

    if (! files.len ())
        return;

    if (is_single)
    {
        const char * last = last_path_element (path);
        result->title = String (last ? last : path);
    }

    files.sort (compare_wrapper, nullptr);

    for (const String & filepath : files)
    {
        GStatBuf info;
        if (g_lstat (filepath, & info) < 0)
            continue;

        if (S_ISREG (info.st_mode))
        {
            String item_name = filename_to_uri (filepath);
            if (item_name)
                add_file (item_name, Tuple (), NULL, filter, user, result, TRUE);
        }
        else if (S_ISDIR (info.st_mode))
        {
            String item_name = filename_to_uri (filepath);
            if (item_name)
                add_folder (item_name, filter, user, result, FALSE);
        }
    }
}

static void add_playlist (const String & filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool_t is_single)
{
    g_return_if_fail (filename);

    if (filter && ! filter (filename, user))
        return;

    status_update (filename, result->items.len ());

    String title;
    Index<PlaylistAddItem> items;

    if (! playlist_load (filename, title, items))
        return;

    if (is_single)
        result->title = title;

    for (auto & item : items)
        add_file (item.filename, std::move (item.tuple), NULL, filter, user, result, FALSE);
}

static void add_generic (const String & filename, Tuple && tuple,
 PlaylistFilterFunc filter, void * user, AddResult * result, bool_t is_single)
{
    g_return_if_fail (filename);

    if (tuple)
        add_file (filename, std::move (tuple), NULL, filter, user, result, FALSE);
    else if (vfs_file_test (filename, G_FILE_TEST_IS_DIR))
        add_folder (filename, filter, user, result, is_single);
    else if (aud_filename_is_playlist (filename))
        add_playlist (filename, filter, user, result, is_single);
    else
        add_file (filename, Tuple (), NULL, filter, user, result, FALSE);
}

static void start_thread_locked (void)
{
    if (add_thread_exited)
    {
        pthread_mutex_unlock (& mutex);
        pthread_join (add_thread, NULL);
        pthread_mutex_lock (& mutex);
    }

    if (! add_thread_started || add_thread_exited)
    {
        pthread_create (& add_thread, NULL, add_worker, NULL);
        add_thread_started = TRUE;
        add_thread_exited = FALSE;
    }
}

static void stop_thread_locked (void)
{
    if (add_thread_started)
    {
        pthread_mutex_unlock (& mutex);
        pthread_join (add_thread, NULL);
        pthread_mutex_lock (& mutex);
        add_thread_started = FALSE;
        add_thread_exited = FALSE;
    }
}

static bool_t add_finish (void * unused)
{
    pthread_mutex_lock (& mutex);

    while (add_results)
    {
        AddResult * result = (AddResult *) add_results->data;
        add_results = g_list_delete_link (add_results, add_results);

        int playlist, count;

        playlist = aud_playlist_by_unique_id (result->playlist_id);
        if (playlist < 0) /* playlist deleted */
            goto FREE;

        count = aud_playlist_entry_count (playlist);
        if (result->at < 0 || result->at > count)
            result->at = count;

        if (result->title && ! count)
        {
            String old_title = aud_playlist_get_title (playlist);

            if (! strcmp (old_title, N_("New Playlist")))
                aud_playlist_set_title (playlist, result->title);
        }

        playlist_entry_insert_batch_raw (playlist, result->at, std::move (result->items));

        if (result->play && aud_playlist_entry_count (playlist) > count)
        {
            if (! aud_get_bool (NULL, "shuffle"))
                aud_playlist_set_position (playlist, result->at);

            aud_drct_play_playlist (playlist);
        }

    FREE:
        delete result;
    }

    if (add_source)
    {
        g_source_remove (add_source);
        add_source = 0;
    }

    if (add_thread_exited)
    {
        stop_thread_locked ();
        status_done_locked ();
    }

    pthread_mutex_unlock (& mutex);

    hook_call ("playlist add complete", NULL);
    return G_SOURCE_REMOVE;
}

static void * add_worker (void * unused)
{
    pthread_mutex_lock (& mutex);

    while (add_tasks)
    {
        AddTask * task = (AddTask *) add_tasks->data;
        add_tasks = g_list_delete_link (add_tasks, add_tasks);

        current_playlist_id = task->playlist_id;
        pthread_mutex_unlock (& mutex);

        AddResult * result = new AddResult ();

        result->playlist_id = task->playlist_id;
        result->at = task->at;
        result->play = task->play;

        bool_t is_single = (task->items.len () == 1);

        for (auto & item : task->items)
            add_generic (item.filename, std::move (item.tuple), task->filter,
             task->user, result, is_single);

        delete task;

        pthread_mutex_lock (& mutex);
        current_playlist_id = -1;

        add_results = g_list_append (add_results, result);

        if (! add_source)
            add_source = g_timeout_add (0, add_finish, NULL);
    }

    add_thread_exited = TRUE;
    pthread_mutex_unlock (& mutex);
    return NULL;
}

void adder_cleanup (void)
{
    pthread_mutex_lock (& mutex);

    for (GList * node = add_tasks; node; node = node->next)
        delete ((AddTask *) node->data);

    g_list_free (add_tasks);
    add_tasks = NULL;

    stop_thread_locked ();
    status_done_locked ();

    for (GList * node = add_results; node; node = node->next)
        delete ((AddResult *) node->data);

    g_list_free (add_results);
    add_results = NULL;

    if (add_source)
    {
        g_source_remove (add_source);
        add_source = 0;
    }

    pthread_mutex_unlock (& mutex);
}

EXPORT void aud_playlist_entry_insert (int playlist, int at,
 const char * filename, Tuple && tuple, bool_t play)
{
    Index<PlaylistAddItem> items;
    items.append ({String (filename), std::move (tuple)});

    aud_playlist_entry_insert_batch (playlist, at, std::move (items), play);
}

EXPORT void aud_playlist_entry_insert_batch (int playlist, int at,
 Index<PlaylistAddItem> && items, bool_t play)
{
    aud_playlist_entry_insert_filtered (playlist, at, std::move (items), NULL, NULL, play);
}

EXPORT void aud_playlist_entry_insert_filtered (int playlist, int at,
 Index<PlaylistAddItem> && items, PlaylistFilterFunc filter, void * user,
 bool_t play)
{
    int playlist_id = aud_playlist_get_unique_id (playlist);
    g_return_if_fail (playlist_id >= 0);

    pthread_mutex_lock (& mutex);

    AddTask * task = new AddTask ();

    task->playlist_id = playlist_id;
    task->at = at;
    task->play = play;
    task->items = std::move (items);
    task->filter = filter;
    task->user = user;

    add_tasks = g_list_append (add_tasks, task);
    start_thread_locked ();

    pthread_mutex_unlock (& mutex);
}

EXPORT bool_t aud_playlist_add_in_progress (int playlist)
{
    pthread_mutex_lock (& mutex);

    if (playlist >= 0)
    {
        int playlist_id = aud_playlist_get_unique_id (playlist);

        for (GList * node = add_tasks; node; node = node->next)
        {
            if (((AddTask *) node->data)->playlist_id == playlist_id)
                goto YES;
        }

        if (current_playlist_id == playlist_id)
            goto YES;

        for (GList * node = add_results; node; node = node->next)
        {
            if (((AddResult *) node->data)->playlist_id == playlist_id)
                goto YES;
        }
    }
    else
    {
        if (add_tasks || current_playlist_id >= 0 || add_results)
            goto YES;
    }

    pthread_mutex_unlock (& mutex);
    return FALSE;

YES:
    pthread_mutex_unlock (& mutex);
    return TRUE;
}
