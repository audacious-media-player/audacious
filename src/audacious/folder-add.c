/*
 * folder-add.c
 * Copyright 2009-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include <libaudcore/audstrings.h>

#include "audconfig.h"
#include "config.h"
#include "glib-compat.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"

typedef struct {
    gchar * filename;
    PluginHandle * decoder;
} AddFile;

typedef struct {
    gint playlist_id, at;
    GQueue folders; // (gchar *)
    gboolean play;
} AddGroup;

static GQueue add_queue = G_QUEUE_INIT; // (AddGroup *)
static gint add_source = 0;
static GtkWidget * add_window = NULL, * add_progress_path, * add_progress_count;

static void show_progress (const gchar * path, gint count)
{
    if (add_window == NULL)
    {
        GtkWidget * vbox;

        add_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_type_hint (GTK_WINDOW(add_window),
         GDK_WINDOW_TYPE_HINT_DIALOG);
        gtk_window_set_title (GTK_WINDOW(add_window), _("Searching ..."));
        gtk_window_set_resizable (GTK_WINDOW(add_window), FALSE);
        gtk_container_set_border_width (GTK_CONTAINER(add_window), 6);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER(add_window), vbox);

        add_progress_path = gtk_label_new ("");
        gtk_widget_set_size_request (add_progress_path, 320, -1);
        gtk_label_set_ellipsize (GTK_LABEL(add_progress_path),
         PANGO_ELLIPSIZE_MIDDLE);
        gtk_box_pack_start (GTK_BOX(vbox), add_progress_path, FALSE, FALSE, 0);

        add_progress_count = gtk_label_new ("");
        gtk_widget_set_size_request (add_progress_count, 320, -1);
        gtk_box_pack_start (GTK_BOX(vbox), add_progress_count, FALSE, FALSE, 0);

        gtk_widget_show_all (add_window);

        g_signal_connect (G_OBJECT(add_window), "destroy",
         G_CALLBACK(gtk_widget_destroyed), & add_window);
    }

    gtk_label_set_text (GTK_LABEL(add_progress_path), path);

    gchar scratch[128];
    snprintf (scratch, sizeof scratch, dngettext (PACKAGE, "%d file found",
     "%d files found", count), count);
    gtk_label_set_text (GTK_LABEL(add_progress_count), scratch);
}

static void show_done (void)
{
    gtk_widget_destroy (add_window);
}

static gint sort_cb (const AddFile * a, const AddFile * b)
{
    return string_compare_encoded (a->filename, b->filename);
}

static gboolean add_cb (void * unused)
{
    static GList * stack = NULL; // (gchar *) and (DIR *) alternately
    static GList * big_list = NULL, * little_list = NULL; // (AddFile *)

    if (! stack)
    {
        AddGroup * group = g_queue_peek_head (& add_queue);
        g_return_val_if_fail (group, FALSE);
        gchar * folder = g_queue_pop_head (& group->folders);
        g_return_val_if_fail (folder, FALSE);
        stack = g_list_prepend (NULL, folder);
    }

    show_progress (stack->data, g_list_length (big_list) + g_list_length
     (little_list));

    for (gint count = 0; count < 30; count ++)
    {
        struct stat info;

        /* top of stack is (gchar *) */

        if (! stat (stack->data, & info))
        {
            if (S_ISREG (info.st_mode))
            {
                gchar * filename = filename_to_uri (stack->data);
                PluginHandle * decoder = (filename == NULL) ? NULL :
                 file_find_decoder (filename, TRUE);

                if (decoder)
                {
                    AddFile * file = g_slice_new (AddFile);
                    file->filename = filename;
                    file->decoder = decoder;
                    little_list = g_list_prepend (little_list, file);
                }
                else
                    g_free (filename);
            }
            else if (S_ISDIR (info.st_mode))
            {
                DIR * folder = opendir (stack->data);

                if (folder)
                {
                    stack = g_list_prepend (stack, folder);
                    goto READ;
                }
            }
        }

        g_free (stack->data);
        stack = g_list_delete_link (stack, stack);

    READ:
        if (! stack)
            break;

        /* top of stack is (DIR *) */

        struct dirent * entry = readdir (stack->data);

        if (entry)
        {
            if (entry->d_name[0] == '.')
                goto READ;

            stack = g_list_prepend (stack, g_strdup_printf ("%s"
             G_DIR_SEPARATOR_S "%s", (gchar *) stack->next->data, entry->d_name));
        }
        else
        {
            closedir (stack->data);
            stack = g_list_delete_link (stack, stack);
            g_free (stack->data);
            stack = g_list_delete_link (stack, stack);
            goto READ;
        }
    }

    if (stack)
        return TRUE; /* not done yet */

    AddGroup * group = g_queue_peek_head (& add_queue);
    g_return_val_if_fail (group, FALSE);

    little_list = g_list_sort (little_list, (GCompareFunc) sort_cb);
    big_list = g_list_concat (big_list, little_list);
    little_list = NULL;

    if (! g_queue_is_empty (& group->folders))
        return TRUE; /* not done yet */

    gint playlist = playlist_by_unique_id (group->playlist_id);

    if (playlist < 0) /* ouch! playlist deleted */
    {
        for (GList * node = big_list; node; node = node->next)
        {
            AddFile * file = node->data;
            g_free (file->filename);
            g_slice_free (AddFile, file);
        }

        g_list_free (big_list);
        big_list = NULL;
        goto ADDED;
    }

    struct index * filenames = index_new ();
    struct index * decoders = index_new ();

    for (GList * node = big_list; node; node = node->next)
    {
        AddFile * file = node->data;
        index_append (filenames, file->filename);
        index_append (decoders, file->decoder);
        g_slice_free (AddFile, file);
    }

    g_list_free (big_list);
    big_list = NULL;

    gint count = playlist_entry_count (playlist);
    if (group->at < 0 || group->at > count)
        group->at = count;

    playlist_entry_insert_batch_with_decoders (playlist, group->at,
     filenames, decoders, NULL);

    if (group->play && playlist_entry_count (playlist) > count)
    {
        playlist_set_playing (playlist);
        if (! cfg.shuffle)
            playlist_set_position (playlist, group->at);
        playback_play (0, FALSE);
    }

ADDED:
    g_slice_free (AddGroup, group);
    g_queue_pop_head (& add_queue);

    if (! g_queue_is_empty (& add_queue))
        return TRUE; /* not done yet */

    show_done ();
    add_source = 0;
    return FALSE;
}

void playlist_insert_folder (gint playlist, gint at, const gchar * folder,
 gboolean play)
{
    gint playlist_id = playlist_get_unique_id (playlist);
    g_return_if_fail (playlist_id >= 0);

    gchar * unix_name = uri_to_filename (folder);
    g_return_if_fail (unix_name);

    if (unix_name[strlen (unix_name) - 1] == '/')
        unix_name[strlen (unix_name) - 1] = 0;

    AddGroup * group = NULL;

    for (GList * node = g_queue_peek_head_link (& add_queue); node; node =
     node->next)
    {
        AddGroup * test = node->data;
        if (test->playlist_id == playlist_id && test->at == at)
        {
            group = test;
            break;
        }
    }

    if (! group)
    {
        group = g_slice_new (AddGroup);
        group->playlist_id = playlist_id;
        group->at = at;
        g_queue_init (& group->folders);
        group->play = FALSE;
        g_queue_push_tail (& add_queue, group);
    }

    g_queue_push_tail (& group->folders, unix_name);
    group->play |= play;

    if (! add_source)
        add_source = g_idle_add (add_cb, NULL);
}
