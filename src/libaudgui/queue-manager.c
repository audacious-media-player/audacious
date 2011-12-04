/*
 * queue-manager.c
 * Copyright 2011 John Lindgren
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "libaudgui.h"
#include "list.h"

enum {
 COLUMN_ENTRY,
 COLUMN_TITLE};

#define RESPONSE_REMOVE 1

static GtkWidget * qm_win;
static GtkWidget * qm_list;

static void get_value (void * user, gint row, gint column, GValue * value)
{
    gint list = aud_playlist_get_active ();
    gint entry = aud_playlist_queue_get_entry (list, row);

    switch (column)
    {
    case COLUMN_ENTRY:
        g_value_set_int (value, 1 + entry);
        break;
    case COLUMN_TITLE:;
        gchar * title = aud_playlist_entry_get_title (list, entry, TRUE);
        g_value_set_string (value, title);
        str_unref (title);
        break;
    }
}

static gboolean get_selected (void * user, gint row)
{
    gint list = aud_playlist_get_active ();
    return aud_playlist_entry_get_selected (list, aud_playlist_queue_get_entry (list, row));
}

static void set_selected (void * user, gint row, gboolean selected)
{
    gint list = aud_playlist_get_active ();
    aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, row), selected);
}

static void select_all (void * user, gboolean selected)
{
    gint list = aud_playlist_get_active ();
    gint count = aud_playlist_queue_count (list);

    for (gint i = 0; i < count; i ++)
        aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, i), selected);
}

static void shift_rows (void * user, gint row, gint before)
{
    GArray * shift = g_array_new (FALSE, FALSE, sizeof (gint));
    gint list = aud_playlist_get_active ();
    gint count = aud_playlist_queue_count (list);

    for (gint i = 0; i < count; i ++)
    {
        gint entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            g_array_append_val (shift, entry);

            if (i < before)
                before --;
        }
    }

    aud_playlist_queue_delete_selected (list);

    for (gint i = 0; i < shift->len; i ++)
        aud_playlist_queue_insert (list, before + i, g_array_index (shift, gint, i));

    g_array_free (shift, TRUE);
}

static const AudguiListCallbacks callbacks = {
 .get_value = get_value,
 .get_selected = get_selected,
 .set_selected = set_selected,
 .select_all = select_all,
 .shift_rows = shift_rows};

static void remove_selected (void)
{
    gint list = aud_playlist_get_active ();
    gint count = aud_playlist_queue_count (list);

    for (gint i = 0; i < count; )
    {
        gint entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            aud_playlist_queue_delete (list, i, 1);
            aud_playlist_entry_set_selected (list, entry, FALSE);
            count --;
        }
        else
            i ++;
    }
}

static void update_hook (void * data, void * user)
{
    audgui_list_delete_rows (qm_list, 0, audgui_list_row_count (qm_list));
    audgui_list_insert_rows (qm_list, 0, aud_playlist_queue_count (aud_playlist_get_active ()));
}

static void destroy_cb (void)
{
    hook_dissociate ("playlist activate", update_hook);
    hook_dissociate ("playlist update", update_hook);

    qm_win = NULL;
    qm_list = NULL;
}

static gboolean keypress_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_A && (event->state && GDK_CONTROL_MASK))
        select_all (NULL, TRUE);
    else if (event->keyval == GDK_Delete)
        remove_selected ();
    else if (event->keyval == GDK_Escape)
        gtk_widget_destroy (qm_win);
    else
        return FALSE;

    return TRUE;
}

static void response_cb (GtkDialog * dialog, gint response)
{
    switch (response)
    {
    case RESPONSE_REMOVE:;
        remove_selected ();
        break;
    case GTK_RESPONSE_CLOSE:
        gtk_widget_destroy (qm_win);
        break;
    }
}

void audgui_queue_manager_show (void)
{
    if (qm_win)
    {
        gtk_window_present ((GtkWindow *) qm_win);
        return;
    }

    qm_win = gtk_dialog_new_with_buttons (_("Queue Manager"), NULL, 0,
     GTK_STOCK_REMOVE, RESPONSE_REMOVE, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
     NULL);
    gtk_window_set_default_size ((GtkWindow *) qm_win, 250, 250);

    GtkWidget * vbox = gtk_dialog_get_content_area ((GtkDialog *) qm_win);

    GtkWidget * scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, TRUE, TRUE, 0);

    gint count = aud_playlist_queue_count (aud_playlist_get_active ());
    qm_list = audgui_list_new (& callbacks, NULL, count);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) qm_list, FALSE);
    audgui_list_add_column (qm_list, NULL, 0, G_TYPE_INT, FALSE);
    audgui_list_add_column (qm_list, NULL, 1, G_TYPE_STRING, TRUE);
    gtk_container_add ((GtkContainer *) scrolled, qm_list);

    hook_associate ("playlist activate", update_hook, NULL);
    hook_associate ("playlist update", update_hook, NULL);

    g_signal_connect (qm_win, "destroy", (GCallback) destroy_cb, NULL);
    g_signal_connect (qm_win, "key-press-event", (GCallback) keypress_cb, NULL);
    g_signal_connect (qm_win, "response", (GCallback) response_cb, NULL);

    gtk_widget_show_all (qm_win);
}
