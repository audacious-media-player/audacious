/*
 * folder-add.c
 * Copyright 2009 John Lindgren
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
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "i18n.h"
#include "playback.h"
#include "playlist-new.h"
#include "probe.h"

static GList * add_queue = NULL;
static gint add_source = 0;
static gint add_playlist, add_at;
static gboolean add_play = FALSE;
static GtkWidget * add_window = NULL, * add_progress_path, * add_progress_count;

static void show_progress (const gchar * path, gint count)
{
    gchar scratch[128];

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
    snprintf (scratch, sizeof scratch, dngettext (PACKAGE, "%d file found",
     "%d files found", count), count);
    gtk_label_set_text (GTK_LABEL(add_progress_count), scratch);
}

static void show_done (void)
{
    gtk_widget_destroy (add_window);
}

static gint compare (const void * * a, const void * * b)
{
    return string_compare (* (const gchar * *) a, * (const gchar * *) b);
}

static gboolean add_cb (void * unused)
{
    static GList * stack = NULL;
    static struct index * index;
    gint count;

    if (stack == NULL)
    {
        stack = g_list_prepend (stack, add_queue->data);
        index = index_new ();
    }

    show_progress ((gchar *) stack->data, index_count (index));

    for (count = 0; count < 30; count ++)
    {
        struct stat info;
        struct dirent * entry;

        if (stat (stack->data, & info) == 0)
        {
            if (S_ISREG (info.st_mode))
            {
                gchar * filename = g_filename_to_uri (stack->data, NULL, NULL);

                if (filename != NULL && file_probe (filename, TRUE) != NULL)
                    index_append (index, filename);
                else
                    g_free (filename);
            }
            else if (S_ISDIR (info.st_mode))
            {
                DIR * folder = opendir (stack->data);

                if (folder != NULL)
                {
                    stack = g_list_prepend (stack, folder);
                    goto READ;
                }
            }
        }

        g_free (stack->data);
        stack = g_list_delete_link (stack, stack);

    READ:
        if (stack == NULL)
            break;

        entry = readdir (stack->data);

        if (entry != NULL)
        {
            if (entry->d_name[0] == '.')
                goto READ;

            stack = g_list_prepend (stack, g_strdup_printf ("%s/%s", (gchar *)
             stack->next->data, entry->d_name));
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

    if (stack == NULL)
    {
        index_sort (index, compare);

        count = playlist_count ();

        if (add_playlist > count - 1)
            add_playlist = count - 1;

        count = playlist_entry_count (add_playlist);

        if (add_at < 0 || add_at > count)
            add_at = count;

        playlist_entry_insert_batch (add_playlist, add_at, index, NULL);

        if (add_play && playlist_entry_count (add_playlist) > count && !
         playback_get_playing ())
        {
            playlist_set_playing (add_playlist);

            if (add_at > 0)
                playlist_set_position (add_playlist, add_at);
            
            playback_initiate ();
        }

        add_at += playlist_entry_count (add_playlist) - count;

        add_queue = g_list_delete_link (add_queue, add_queue);

        if (add_queue == NULL)
        {
            show_done ();
            add_source = 0;
            add_play = FALSE;
            return FALSE;
        }
    }

    return TRUE;
}

void playlist_insert_folder (gint playlist, gint at, const gchar * folder)
{
    gchar * unix_name = g_filename_from_uri (folder, NULL, NULL);

    add_playlist = playlist;
    add_at = at;

    if (unix_name == NULL)
        return;

    if (unix_name[strlen (unix_name) - 1] == '/')
        unix_name[strlen (unix_name) - 1] = 0;

    add_queue = g_list_append (add_queue, unix_name);

    if (add_source == 0)
        add_source = g_idle_add (add_cb, NULL);
}

void playlist_insert_folder_v2 (gint playlist, gint at, const gchar * folder,
 gboolean play)
{
    playlist_insert_folder (playlist, at, folder);
    add_play = play;
}
