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

#include <libaudcore/audstrings.h>

#include "audconfig.h"
#include "config.h"
#include "i18n.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"

static GList * add_queue = NULL;
static gint add_source = 0;
static gint add_playlist, add_at;
static gboolean add_play;
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

typedef struct {
    gchar * filename;
    PluginHandle * decoder;
} AddPair;

static gint pair_compare (const AddPair * a, const AddPair * b)
{
    return string_compare_encoded (a->filename, b->filename);
}

static gboolean add_cb (void * unused)
{
    static GList * stack = NULL;
    static GList * list = NULL;

    if (! stack)
        stack = g_list_prepend (NULL, add_queue->data);

    show_progress ((gchar *) stack->data, g_list_length (list));

    for (gint count = 0; count < 30; count ++)
    {
        struct stat info;

        /* top of stack is a filename */

        if (! stat (stack->data, & info))
        {
            if (S_ISREG (info.st_mode))
            {
                gchar * filename = g_filename_to_uri (stack->data, NULL, NULL);
                PluginHandle * decoder = (filename == NULL) ? NULL :
                 file_find_decoder (filename, TRUE);

                if (decoder)
                {
                    AddPair * pair = g_slice_new (AddPair);
                    pair->filename = filename;
                    pair->decoder = decoder;
                    list = g_list_prepend (list, pair);
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

        /* top of stack is a (DIR *) */

        struct dirent * entry = readdir (stack->data);

        if (entry)
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

    if (! stack)
    {
        list = g_list_sort (list, (GCompareFunc) pair_compare);

        struct index * filenames = index_new ();
        struct index * decoders = index_new ();

        for (GList * node = list; node; node = node->next)
        {
            AddPair * pair = node->data;
            index_append (filenames, pair->filename);
            index_append (decoders, pair->decoder);
            g_slice_free (AddPair, pair);
        }

        g_list_free (list);
        list = NULL;

        if (add_playlist > playlist_count () - 1)
            add_playlist = playlist_count () - 1;

        gint count = playlist_entry_count (add_playlist);

        if (add_at < 0 || add_at > count)
            add_at = count;

        playlist_entry_insert_batch_with_decoders (add_playlist, add_at,
         filenames, decoders, NULL);

        if (add_play && playlist_entry_count (add_playlist) > count)
        {
            playlist_set_playing (add_playlist);
            if (! cfg.shuffle)
                playlist_set_position (add_playlist, add_at);
            playback_play (0, FALSE);
            add_play = FALSE;
        }

        add_at += playlist_entry_count (add_playlist) - count;

        add_queue = g_list_delete_link (add_queue, add_queue);

        if (add_queue == NULL)
        {
            show_done ();
            add_source = 0;
            return FALSE;
        }
    }

    return TRUE;
}

void playlist_insert_folder (gint playlist, gint at, const gchar * folder,
 gboolean play)
{
    gchar * unix_name = g_filename_from_uri (folder, NULL, NULL);

    add_playlist = playlist;
    add_at = at;
    add_play |= play;

    if (unix_name == NULL)
        return;

    if (unix_name[strlen (unix_name) - 1] == '/')
        unix_name[strlen (unix_name) - 1] = 0;

    add_queue = g_list_append (add_queue, unix_name);

    if (add_source == 0)
        add_source = g_idle_add (add_cb, NULL);
}
