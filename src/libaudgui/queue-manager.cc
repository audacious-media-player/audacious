/*
 * queue-manager.c
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "list.h"

enum {
    COLUMN_ENTRY,
    COLUMN_TITLE
};

static void get_value (void * user, int row, int column, GValue * value)
{
    int list = aud_playlist_get_active ();
    int entry = aud_playlist_queue_get_entry (list, row);

    switch (column)
    {
    case COLUMN_ENTRY:
        g_value_set_int (value, 1 + entry);
        break;
    case COLUMN_TITLE:
        Tuple tuple = aud_playlist_entry_get_tuple (list, entry, Playlist::NoWait);
        g_value_set_string (value, tuple.get_str (Tuple::FormattedTitle));
        break;
    }
}

static bool get_selected (void * user, int row)
{
    int list = aud_playlist_get_active ();
    return aud_playlist_entry_get_selected (list, aud_playlist_queue_get_entry (list, row));
}

static void set_selected (void * user, int row, bool selected)
{
    int list = aud_playlist_get_active ();
    aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, row), selected);
}

static void select_all (void * user, bool selected)
{
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; i ++)
        aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, i), selected);
}

static void shift_rows (void * user, int row, int before)
{
    Index<int> shift;
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; i ++)
    {
        int entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            shift.append (entry);

            if (i < before)
                before --;
        }
    }

    aud_playlist_queue_delete_selected (list);

    for (int i = 0; i < shift.len (); i ++)
        aud_playlist_queue_insert (list, before + i, shift[i]);
}

static const AudguiListCallbacks callbacks = {
    get_value,
    get_selected,
    set_selected,
    select_all,
    0,  // activate_row
    0,  // right_click
    shift_rows
};

static void remove_selected (void *)
{
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; )
    {
        int entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            aud_playlist_queue_delete (list, i, 1);
            aud_playlist_entry_set_selected (list, entry, false);
            count --;
        }
        else
            i ++;
    }
}

static void update_hook (void * data, void * user)
{
    GtkWidget * qm_list = (GtkWidget *) user;

    int oldrows = audgui_list_row_count (qm_list);
    int newrows = aud_playlist_queue_count (aud_playlist_get_active ());
    int focus = audgui_list_get_focus (qm_list);

    audgui_list_update_rows (qm_list, 0, aud::min (oldrows, newrows));
    audgui_list_update_selection (qm_list, 0, aud::min (oldrows, newrows));

    if (newrows > oldrows)
        audgui_list_insert_rows (qm_list, oldrows, newrows - oldrows);
    if (newrows < oldrows)
        audgui_list_delete_rows (qm_list, newrows, oldrows - newrows);

    if (focus > newrows - 1)
        audgui_list_set_focus (qm_list, newrows - 1);
}

static void destroy_cb ()
{
    hook_dissociate ("playlist activate", update_hook);
    hook_dissociate ("playlist update", update_hook);
}

static gboolean keypress_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_KEY_A && (event->state & GDK_CONTROL_MASK))
        select_all (nullptr, true);
    else if (event->keyval == GDK_KEY_Delete)
        remove_selected (nullptr);
    else if (event->keyval == GDK_KEY_Escape)
        gtk_widget_destroy (widget);
    else
        return false;

    return true;
}

static GtkWidget * create_queue_manager ()
{
    int dpi = audgui_get_dpi ();

    GtkWidget * qm_win = gtk_dialog_new ();
    gtk_window_set_title ((GtkWindow *) qm_win, _("Queue Manager"));
    gtk_window_set_default_size ((GtkWindow *) qm_win, 3 * dpi, 2 * dpi);

    GtkWidget * vbox = gtk_dialog_get_content_area ((GtkDialog *) qm_win);

    GtkWidget * scrolled = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled, GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, true, true, 0);

    int count = aud_playlist_queue_count (aud_playlist_get_active ());
    GtkWidget * qm_list = audgui_list_new (& callbacks, nullptr, count);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) qm_list, false);
    audgui_list_add_column (qm_list, nullptr, 0, G_TYPE_INT, 7);
    audgui_list_add_column (qm_list, nullptr, 1, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scrolled, qm_list);

    GtkWidget * button1 = audgui_button_new (_("_Unqueue"), "list-remove", remove_selected, nullptr);
    GtkWidget * button2 = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) gtk_widget_destroy, qm_win);

    gtk_dialog_add_action_widget ((GtkDialog *) qm_win, button1, GTK_RESPONSE_NONE);
    gtk_dialog_add_action_widget ((GtkDialog *) qm_win, button2, GTK_RESPONSE_NONE);

    hook_associate ("playlist activate", update_hook, qm_list);
    hook_associate ("playlist update", update_hook, qm_list);

    g_signal_connect (qm_win, "destroy", (GCallback) destroy_cb, nullptr);
    g_signal_connect (qm_win, "key-press-event", (GCallback) keypress_cb, nullptr);

    return qm_win;
}

EXPORT void audgui_queue_manager_show ()
{
    if (! audgui_reshow_unique_window (AUDGUI_QUEUE_MANAGER_WINDOW))
        audgui_show_unique_window (AUDGUI_QUEUE_MANAGER_WINDOW, create_queue_manager ());
}
