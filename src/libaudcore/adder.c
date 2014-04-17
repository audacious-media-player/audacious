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
#include "vfs.h"

typedef struct {
    int playlist_id, at;
    bool_t play;
    Index * filenames, * tuples;
    PlaylistFilterFunc filter;
    void * user;
} AddTask;

typedef struct {
    int playlist_id, at;
    bool_t play;
    char * title;
    Index * filenames, * tuples, * decoders;
} AddResult;

static GList * add_tasks = NULL;
static GList * add_results = NULL;
static int current_playlist_id = -1;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static bool_t add_quit;
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

static AddTask * add_task_new (int playlist_id, int at, bool_t play,
 Index * filenames, Index * tuples, PlaylistFilterFunc filter,
 void * user)
{
    AddTask * task = g_slice_new (AddTask);
    task->playlist_id = playlist_id;
    task->at = at;
    task->play = play;
    task->filenames = filenames;
    task->tuples = tuples;
    task->filter = filter;
    task->user = user;
    return task;
}

static void add_task_free (AddTask * task)
{
    if (task->filenames)
        index_free_full (task->filenames, (IndexFreeFunc) str_unref);
    if (task->tuples)
        index_free_full (task->tuples, (IndexFreeFunc) tuple_unref);

    g_slice_free (AddTask, task);
}

static AddResult * add_result_new (int playlist_id, int at, bool_t play)
{
    AddResult * result = g_slice_new (AddResult);
    result->playlist_id = playlist_id;
    result->at = at;
    result->play = play;
    result->title = NULL;
    result->filenames = index_new ();
    result->tuples = index_new ();
    result->decoders = index_new ();
    return result;
}

static void add_result_free (AddResult * result)
{
    str_unref (result->title);

    if (result->filenames)
        index_free_full (result->filenames, (IndexFreeFunc) str_unref);
    if (result->tuples)
        index_free_full (result->tuples, (IndexFreeFunc) tuple_unref);
    if (result->decoders)
        index_free (result->decoders);

    g_slice_free (AddResult, result);
}

static void add_file (char * filename, Tuple * tuple, PluginHandle * decoder,
 PlaylistFilterFunc filter, void * user, AddResult * result, bool_t validate)
{
    g_return_if_fail (filename);
    if (filter && ! filter (filename, user))
    {
        str_unref (filename);
        return;
    }

    status_update (filename, index_count (result->filenames));

    if (! tuple && ! decoder)
    {
        decoder = aud_file_find_decoder (filename, TRUE);
        if (validate && ! decoder)
        {
            str_unref (filename);
            return;
        }
    }

    if (! tuple && decoder && input_plugin_has_subtunes (decoder) && ! strchr (filename, '?'))
        tuple = aud_file_read_tuple (filename, decoder);

    int n_subtunes = tuple ? tuple_get_n_subtunes (tuple) : 0;

    if (n_subtunes)
    {
        for (int sub = 0; sub < n_subtunes; sub ++)
        {
            char * subname = str_printf ("%s?%d", filename,
             tuple_get_nth_subtune (tuple, sub));
            add_file (subname, NULL, decoder, filter, user, result, FALSE);
        }

        str_unref (filename);
        tuple_unref (tuple);
        return;
    }

    index_insert (result->filenames, -1, filename);
    index_insert (result->tuples, -1, tuple);
    index_insert (result->decoders, -1, decoder);
}

static void add_folder (char * filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool_t is_single)
{
    char * path = NULL;

    g_return_if_fail (filename);

    if (filter && ! filter (filename, user))
        goto DONE;

    status_update (filename, index_count (result->filenames));

    if (! (path = uri_to_filename (filename)))
        goto DONE;

    GList * files = NULL;
    GDir * folder = g_dir_open (path, 0, NULL);
    if (! folder)
        goto DONE;

    const char * name;
    while ((name = g_dir_read_name (folder)))
    {
        char * filepath = filename_build (path, name);
        files = g_list_prepend (files, filepath);
    }

    g_dir_close (folder);

    if (files && is_single)
    {
        char * last = last_path_element (path);
        result->title = str_get (last ? last : path);
    }

    files = g_list_sort (files, (GCompareFunc) str_compare);

    while (files)
    {
        GStatBuf info;
        if (g_lstat (files->data, & info) < 0)
            goto NEXT;

        if (S_ISREG (info.st_mode))
        {
            char * item_name = filename_to_uri (files->data);
            if (item_name)
                add_file (item_name, NULL, NULL, filter, user, result, TRUE);
        }
        else if (S_ISDIR (info.st_mode))
        {
            char * item_name = filename_to_uri (files->data);
            if (item_name)
                add_folder (item_name, filter, user, result, FALSE);
        }

    NEXT:
        str_unref (files->data);
        files = g_list_delete_link (files, files);
    }

DONE:
    str_unref (filename);
    str_unref (path);
}

static void add_playlist (char * filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool_t is_single)
{
    g_return_if_fail (filename);
    if (filter && ! filter (filename, user))
    {
        str_unref (filename);
        return;
    }

    status_update (filename, index_count (result->filenames));

    char * title = NULL;
    Index * filenames, * tuples;
    if (! playlist_load (filename, & title, & filenames, & tuples))
    {
        str_unref (filename);
        return;
    }

    if (is_single)
        result->title = title;
    else
        str_unref (title);

    int count = index_count (filenames);
    for (int i = 0; i < count; i ++)
        add_file (index_get (filenames, i), tuples ? index_get (tuples, i) :
         NULL, NULL, filter, user, result, FALSE);

    str_unref (filename);
    index_free (filenames);
    if (tuples)
        index_free (tuples);
}

static void add_generic (char * filename, Tuple * tuple,
 PlaylistFilterFunc filter, void * user, AddResult * result, bool_t is_single)
{
    g_return_if_fail (filename);

    if (tuple)
        add_file (filename, tuple, NULL, filter, user, result, FALSE);
    else if (vfs_file_test (filename, G_FILE_TEST_IS_DIR))
        add_folder (filename, filter, user, result, is_single);
    else if (aud_filename_is_playlist (filename))
        add_playlist (filename, filter, user, result, is_single);
    else
        add_file (filename, NULL, NULL, filter, user, result, FALSE);
}

static bool_t add_finish (void * unused)
{
    pthread_mutex_lock (& mutex);

    while (add_results)
    {
        AddResult * result = add_results->data;
        add_results = g_list_delete_link (add_results, add_results);

        int playlist = aud_playlist_by_unique_id (result->playlist_id);
        if (playlist < 0) /* playlist deleted */
            goto FREE;

        int count = aud_playlist_entry_count (playlist);
        if (result->at < 0 || result->at > count)
            result->at = count;

        if (result->title && ! count)
        {
            char * old_title = aud_playlist_get_title (playlist);

            if (! strcmp (old_title, N_("New Playlist")))
                aud_playlist_set_title (playlist, result->title);

            str_unref (old_title);
        }

        playlist_entry_insert_batch_raw (playlist, result->at,
         result->filenames, result->tuples, result->decoders);
        result->filenames = NULL;
        result->tuples = NULL;
        result->decoders = NULL;

        if (result->play && aud_playlist_entry_count (playlist) > count)
        {
            if (! aud_get_bool (NULL, "shuffle"))
                aud_playlist_set_position (playlist, result->at);

            aud_drct_play_playlist (playlist);
        }

    FREE:
        add_result_free (result);
    }

    if (add_source)
    {
        g_source_remove (add_source);
        add_source = 0;
    }

    if (! add_tasks)
        status_done_locked ();

    pthread_mutex_unlock (& mutex);

    hook_call ("playlist add complete", NULL);
    return FALSE;
}

static void * add_worker (void * unused)
{
    pthread_mutex_lock (& mutex);

    while (! add_quit)
    {
        if (! add_tasks)
        {
            pthread_cond_wait (& cond, & mutex);
            continue;
        }

        AddTask * task = add_tasks->data;
        add_tasks = g_list_delete_link (add_tasks, add_tasks);

        current_playlist_id = task->playlist_id;
        pthread_mutex_unlock (& mutex);

        AddResult * result = add_result_new (task->playlist_id, task->at, task->play);

        int count = index_count (task->filenames);
        if (task->tuples)
            count = MIN (count, index_count (task->tuples));

        for (int i = 0; i < count; i ++)
        {
            add_generic (index_get (task->filenames, i), task->tuples ?
             index_get (task->tuples, i) : NULL, task->filter, task->user,
             result, (count == 1));

            index_set (task->filenames, i, NULL);
            if (task->tuples)
                index_set (task->tuples, i, NULL);
        }

        add_task_free (task);

        pthread_mutex_lock (& mutex);
        current_playlist_id = -1;

        add_results = g_list_append (add_results, result);

        if (! add_source)
            add_source = g_timeout_add (0, add_finish, NULL);
    }

    pthread_mutex_unlock (& mutex);
    return NULL;
}

void adder_init (void)
{
    pthread_mutex_lock (& mutex);
    add_quit = FALSE;
    pthread_create (& add_thread, NULL, add_worker, NULL);
    pthread_mutex_unlock (& mutex);
}

void adder_cleanup (void)
{
    pthread_mutex_lock (& mutex);
    add_quit = TRUE;
    pthread_cond_broadcast (& cond);
    pthread_mutex_unlock (& mutex);
    pthread_join (add_thread, NULL);

    g_list_free_full (add_tasks, (GDestroyNotify) add_task_free);
    add_tasks = NULL;
    g_list_free_full (add_results, (GDestroyNotify) add_result_free);
    add_results = NULL;

    if (add_source)
    {
        g_source_remove (add_source);
        add_source = 0;
    }

    status_done_locked ();
}

EXPORT void aud_playlist_entry_insert (int playlist, int at, const char * filename,
 Tuple * tuple, bool_t play)
{
    Index * filenames = index_new ();
    Index * tuples = index_new ();
    index_insert (filenames, -1, str_get (filename));
    index_insert (tuples, -1, tuple);

    aud_playlist_entry_insert_batch (playlist, at, filenames, tuples, play);
}

EXPORT void aud_playlist_entry_insert_batch (int playlist, int at,
 Index * filenames, Index * tuples, bool_t play)
{
    aud_playlist_entry_insert_filtered (playlist, at, filenames, tuples, NULL, NULL, play);
}

EXPORT void aud_playlist_entry_insert_filtered (int playlist, int at,
 Index * filenames, Index * tuples, PlaylistFilterFunc filter,
 void * user, bool_t play)
{
    int playlist_id = aud_playlist_get_unique_id (playlist);
    g_return_if_fail (playlist_id >= 0);

    AddTask * task = add_task_new (playlist_id, at, play, filenames, tuples, filter, user);

    pthread_mutex_lock (& mutex);
    add_tasks = g_list_append (add_tasks, task);
    pthread_cond_broadcast (& cond);
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
