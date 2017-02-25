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
    auto list = Playlist::active_playlist ();
    int entry = list.queue_get_entry (row);

    switch (column)
    {
    case COLUMN_ENTRY:
        g_value_set_int (value, 1 + entry);
        break;
    case COLUMN_TITLE:
        Tuple tuple = list.entry_tuple (entry, Playlist::NoWait);
        g_value_set_string (value, tuple.get_str (Tuple::FormattedTitle));
        break;
    }
}

static bool get_selected (void * user, int row)
{
    auto list = Playlist::active_playlist ();
    return list.entry_selected (list.queue_get_entry (row));
}

static void set_selected (void * user, int row, bool selected)
{
    auto list = Playlist::active_playlist ();
    list.select_entry (list.queue_get_entry (row), selected);
}

static void select_all (void * user, bool selected)
{
    auto list = Playlist::active_playlist ();
    int count = list.n_queued ();

    for (int i = 0; i < count; i ++)
        list.select_entry (list.queue_get_entry (i), selected);
}

static void shift_rows (void * user, int row, int before)
{
    Index<int> shift;
    auto list = Playlist::active_playlist ();
    int count = list.n_queued ();

    for (int i = 0; i < count; i ++)
    {
        int entry = list.queue_get_entry (i);

        if (list.entry_selected (entry))
        {
            shift.append (entry);

            if (i < before)
                before --;
        }
    }

    list.queue_remove_selected ();

    for (int i = 0; i < shift.len (); i ++)
        list.queue_insert (before + i, shift[i]);
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
    auto list = Playlist::active_playlist ();
    int count = list.n_queued ();

    for (int i = 0; i < count; )
    {
        int entry = list.queue_get_entry (i);

        if (list.entry_selected (entry))
        {
            list.queue_remove (i);
            list.select_entry (entry, false);
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
    int newrows = Playlist::active_playlist ().n_queued ();
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

    int count = Playlist::active_playlist ().n_queued ();
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
