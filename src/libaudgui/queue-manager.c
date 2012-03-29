/*
 * queue-manager.c
 * Copyright 2011 John Lindgren
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "init.h"
#include "libaudgui.h"
#include "list.h"

enum {
 COLUMN_ENTRY,
 COLUMN_TITLE};

#define RESPONSE_REMOVE 1

static GtkWidget * qm_win;
static GtkWidget * qm_list;

static void get_value (void * user, int row, int column, GValue * value)
{
    int list = aud_playlist_get_active ();
    int entry = aud_playlist_queue_get_entry (list, row);

    switch (column)
    {
    case COLUMN_ENTRY:
        g_value_set_int (value, 1 + entry);
        break;
    case COLUMN_TITLE:;
        char * title = aud_playlist_entry_get_title (list, entry, TRUE);
        g_value_set_string (value, title);
        str_unref (title);
        break;
    }
}

static bool_t get_selected (void * user, int row)
{
    int list = aud_playlist_get_active ();
    return aud_playlist_entry_get_selected (list, aud_playlist_queue_get_entry (list, row));
}

static void set_selected (void * user, int row, bool_t selected)
{
    int list = aud_playlist_get_active ();
    aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, row), selected);
}

static void select_all (void * user, bool_t selected)
{
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; i ++)
        aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, i), selected);
}

static void shift_rows (void * user, int row, int before)
{
    GArray * shift = g_array_new (FALSE, FALSE, sizeof (int));
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; i ++)
    {
        int entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            g_array_append_val (shift, entry);

            if (i < before)
                before --;
        }
    }

    aud_playlist_queue_delete_selected (list);

    for (int i = 0; i < shift->len; i ++)
        aud_playlist_queue_insert (list, before + i, g_array_index (shift, int, i));

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
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; )
    {
        int entry = aud_playlist_queue_get_entry (list, i);

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
    int oldrows = audgui_list_row_count (qm_list);
    int newrows = aud_playlist_queue_count (aud_playlist_get_active ());
    int focus = audgui_list_get_focus (qm_list);

    audgui_list_update_rows (qm_list, 0, MIN (oldrows, newrows));
    audgui_list_update_selection (qm_list, 0, MIN (oldrows, newrows));

    if (newrows > oldrows)
        audgui_list_insert_rows (qm_list, oldrows, newrows - oldrows);
    if (newrows < oldrows)
        audgui_list_delete_rows (qm_list, newrows, oldrows - newrows);

    if (focus > newrows - 1)
        audgui_list_set_focus (qm_list, newrows - 1);
}

static void destroy_cb (void)
{
    hook_dissociate ("playlist activate", update_hook);
    hook_dissociate ("playlist update", update_hook);

    qm_win = NULL;
    qm_list = NULL;
}

static bool_t keypress_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_A && (event->state & GDK_CONTROL_MASK))
        select_all (NULL, TRUE);
    else if (event->keyval == GDK_Delete)
        remove_selected ();
    else if (event->keyval == GDK_Escape)
        gtk_widget_destroy (qm_win);
    else
        return FALSE;

    return TRUE;
}

static void response_cb (GtkDialog * dialog, int response)
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

EXPORT void audgui_queue_manager_show (void)
{
    if (qm_win)
    {
        gtk_window_present ((GtkWindow *) qm_win);
        return;
    }

    qm_win = gtk_dialog_new_with_buttons (_("Queue Manager"), NULL, 0,
     GTK_STOCK_REMOVE, RESPONSE_REMOVE, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
     NULL);
    gtk_window_set_default_size ((GtkWindow *) qm_win, 400, 250);

    GtkWidget * vbox = gtk_dialog_get_content_area ((GtkDialog *) qm_win);

    GtkWidget * scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled, GTK_SHADOW_IN);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, TRUE, TRUE, 0);

    int count = aud_playlist_queue_count (aud_playlist_get_active ());
    qm_list = audgui_list_new (& callbacks, NULL, count);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) qm_list, FALSE);
    audgui_list_add_column (qm_list, NULL, 0, G_TYPE_INT, 7);
    audgui_list_add_column (qm_list, NULL, 1, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scrolled, qm_list);

    hook_associate ("playlist activate", update_hook, NULL);
    hook_associate ("playlist update", update_hook, NULL);

    g_signal_connect (qm_win, "destroy", (GCallback) destroy_cb, NULL);
    g_signal_connect (qm_win, "key-press-event", (GCallback) keypress_cb, NULL);
    g_signal_connect (qm_win, "response", (GCallback) response_cb, NULL);

    gtk_widget_show_all (qm_win);
}

void audgui_queue_manager_cleanup (void)
{
    if (qm_win)
        gtk_widget_destroy (qm_win);
}
