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
#include "hook.h"
#include "i18n.h"
#include "list.h"
#include "mainloop.h"
#include "plugins-internal.h"
#include "probe.h"
#include "runtime.h"
#include "tuple.h"
#include "vfs.h"

struct AddTask : public ListNode
{
    int playlist_id, at;
    bool play;
    Index<PlaylistAddItem> items;
    PlaylistFilterFunc filter;
    void * user;
};

struct AddResult : public ListNode
{
    int playlist_id, at;
    bool play;
    String title;
    Index<PlaylistAddItem> items;
};

static void * add_worker (void * unused);

static List<AddTask> add_tasks;
static List<AddResult> add_results;
static int current_playlist_id = -1;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool add_thread_started = false;
static bool add_thread_exited = false;
static pthread_t add_thread;
static QueuedFunc queued_add;
static QueuedFunc status_timer;
static char status_path[512];
static int status_count;

static void status_cb (void * unused)
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
}

static void status_update (const char * filename, int found)
{
    pthread_mutex_lock (& mutex);

    snprintf (status_path, sizeof status_path, "%s", filename);
    status_count = found;

    if (! status_timer.running ())
        status_timer.start (250, status_cb, nullptr);

    pthread_mutex_unlock (& mutex);
}

static void status_done_locked (void)
{
    status_timer.stop ();

    if (aud_get_headless_mode ())
        printf ("\n");
    else
        hook_call ("ui hide progress", nullptr);
}

static void add_file (const char * filename, Tuple && tuple,
 PluginHandle * decoder, PlaylistFilterFunc filter, void * user,
 AddResult * result, bool validate)
{
    if (filter && ! filter (filename, user))
        return;

    AUDINFO ("Adding file: %s\n", filename);
    status_update (filename, result->items.len ());

    if (! tuple && ! decoder)
    {
        decoder = aud_file_find_decoder (filename, ! aud_get_bool (nullptr, "slow_probe"));
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
            StringBuf subname = str_printf ("%s?%d", filename, tuple.get_nth_subtune (sub));
            add_file (subname, Tuple (), decoder, filter, user, result, false);
        }
    }
    else
        result->items.append (String (filename), std::move (tuple), decoder);
}

static void add_playlist (const char * filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool is_single)
{
    if (filter && ! filter (filename, user))
        return;

    AUDINFO ("Adding playlist: %s\n", filename);
    status_update (filename, result->items.len ());

    String title;
    Index<PlaylistAddItem> items;

    if (! playlist_load (filename, title, items))
        return;

    if (is_single)
        result->title = title;

    for (auto & item : items)
        add_file (item.filename, std::move (item.tuple), nullptr, filter, user, result, false);
}

static void add_folder (const char * filename, PlaylistFilterFunc filter,
 void * user, AddResult * result, bool is_single)
{
    Index<String> cuesheets, files;
    GDir * folder;

    if (filter && ! filter (filename, user))
        return;

    AUDINFO ("Adding folder: %s\n", filename);
    status_update (filename, result->items.len ());

    StringBuf path = uri_to_filename (filename);
    if (! path)
        return;

    if (! (folder = g_dir_open (path, 0, nullptr)))
        return;

    const char * name;
    while ((name = g_dir_read_name (folder)))
    {
        if (str_has_suffix_nocase (name, ".cue"))
            cuesheets.append (name);
        else
            files.append (name);
    }

    g_dir_close (folder);

    for (const char * cuesheet : cuesheets)
    {
        AUDINFO ("Found cuesheet: %s\n", cuesheet);

        auto is_match = [=] (const char * name)
            { return same_basename (name, cuesheet); };

        files.remove_if (is_match);
    }

    files.move_from (cuesheets, 0, -1, -1, true, true);

    if (! files.len ())
        return;

    if (is_single)
    {
        const char * last = last_path_element (path);
        result->title = String (last ? last : path);
    }

    auto compare_wrapper = [] (const String & a, const String & b, void *)
        { return str_compare (a, b); };

    files.sort (compare_wrapper, nullptr);

    for (const char * name : files)
    {
        StringBuf filepath = filename_build ({path, name});
        StringBuf uri = filename_to_uri (filepath);
        if (! uri)
            continue;

        GStatBuf info;
        if (g_lstat (filepath, & info) < 0)
            continue;

        if (S_ISREG (info.st_mode))
        {
            if (str_has_suffix_nocase (name, ".cue"))
                add_playlist (uri, filter, user, result, false);
            else
                add_file (uri, Tuple (), nullptr, filter, user, result, true);
        }
        else if (S_ISDIR (info.st_mode))
            add_folder (uri, filter, user, result, false);
    }
}

static void add_generic (const char * filename, Tuple && tuple,
 PlaylistFilterFunc filter, void * user, AddResult * result, bool is_single)
{
    if (tuple)
        add_file (filename, std::move (tuple), nullptr, filter, user, result, false);
    else if (VFSFile::test_file (filename, VFS_IS_DIR))
        add_folder (filename, filter, user, result, is_single);
    else if (aud_filename_is_playlist (filename))
        add_playlist (filename, filter, user, result, is_single);
    else
        add_file (filename, Tuple (), nullptr, filter, user, result, false);
}

static void start_thread_locked (void)
{
    if (add_thread_exited)
    {
        pthread_mutex_unlock (& mutex);
        pthread_join (add_thread, nullptr);
        pthread_mutex_lock (& mutex);
    }

    if (! add_thread_started || add_thread_exited)
    {
        pthread_create (& add_thread, nullptr, add_worker, nullptr);
        add_thread_started = true;
        add_thread_exited = false;
    }
}

static void stop_thread_locked (void)
{
    if (add_thread_started)
    {
        pthread_mutex_unlock (& mutex);
        pthread_join (add_thread, nullptr);
        pthread_mutex_lock (& mutex);
        add_thread_started = false;
        add_thread_exited = false;
    }
}

static void add_finish (void * unused)
{
    pthread_mutex_lock (& mutex);

    AddResult * result;
    while ((result = add_results.head ()))
    {
        add_results.remove (result);

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
            if (! aud_get_bool (0, "shuffle"))
                aud_playlist_set_position (playlist, result->at);

            aud_playlist_play (playlist);
        }

    FREE:
        delete result;
    }

    if (add_thread_exited)
    {
        stop_thread_locked ();
        status_done_locked ();
    }

    pthread_mutex_unlock (& mutex);

    hook_call ("playlist add complete", nullptr);
}

static void * add_worker (void * unused)
{
    pthread_mutex_lock (& mutex);

    AddTask * task;
    while ((task = add_tasks.head ()))
    {
        add_tasks.remove (task);

        current_playlist_id = task->playlist_id;
        pthread_mutex_unlock (& mutex);

        AddResult * result = new AddResult ();

        result->playlist_id = task->playlist_id;
        result->at = task->at;
        result->play = task->play;

        bool is_single = (task->items.len () == 1);

        for (auto & item : task->items)
            add_generic (item.filename, std::move (item.tuple), task->filter,
             task->user, result, is_single);

        delete task;

        pthread_mutex_lock (& mutex);
        current_playlist_id = -1;

        if (! add_results.head ())
            queued_add.queue (add_finish, nullptr);

        add_results.append (result);
    }

    add_thread_exited = true;
    pthread_mutex_unlock (& mutex);
    return nullptr;
}

void adder_cleanup (void)
{
    pthread_mutex_lock (& mutex);

    add_tasks.clear ();

    stop_thread_locked ();
    status_done_locked ();

    add_results.clear ();

    queued_add.stop ();

    pthread_mutex_unlock (& mutex);
}

EXPORT void aud_playlist_entry_insert (int playlist, int at,
 const char * filename, Tuple && tuple, bool play)
{
    Index<PlaylistAddItem> items;
    items.append (String (filename), std::move (tuple));

    aud_playlist_entry_insert_batch (playlist, at, std::move (items), play);
}

EXPORT void aud_playlist_entry_insert_batch (int playlist, int at,
 Index<PlaylistAddItem> && items, bool play)
{
    aud_playlist_entry_insert_filtered (playlist, at, std::move (items), nullptr, nullptr, play);
}

EXPORT void aud_playlist_entry_insert_filtered (int playlist, int at,
 Index<PlaylistAddItem> && items, PlaylistFilterFunc filter, void * user,
 bool play)
{
    int playlist_id = aud_playlist_get_unique_id (playlist);

    pthread_mutex_lock (& mutex);

    AddTask * task = new AddTask ();

    task->playlist_id = playlist_id;
    task->at = at;
    task->play = play;
    task->items = std::move (items);
    task->filter = filter;
    task->user = user;

    add_tasks.append (task);
    start_thread_locked ();

    pthread_mutex_unlock (& mutex);
}

EXPORT bool aud_playlist_add_in_progress (int playlist)
{
    pthread_mutex_lock (& mutex);

    if (playlist >= 0)
    {
        int playlist_id = aud_playlist_get_unique_id (playlist);

        for (AddTask * task = add_tasks.head (); task; task = add_tasks.next (task))
        {
            if (task->playlist_id == playlist_id)
                goto YES;
        }

        if (current_playlist_id == playlist_id)
            goto YES;

        for (AddResult * result = add_results.head (); result; result = add_results.next (result))
        {
            if (result->playlist_id == playlist_id)
                goto YES;
        }
    }
    else
    {
        if (add_tasks.head () || current_playlist_id >= 0 || add_results.head ())
            goto YES;
    }

    pthread_mutex_unlock (& mutex);
    return false;

YES:
    pthread_mutex_unlock (& mutex);
    return true;
}
