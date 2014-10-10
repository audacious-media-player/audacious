/*
 * ui_jumptotrack.c
 * Copyright 2007-2012 Yoshiki Yazawa and John Lindgren
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
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "list.h"
#include "ui_jumptotrack_cache.h"

static void update_cb (void * level, void *);
static void activate_cb (void * data, void *);

static JumpToTrackCache cache;
static const KeywordMatches * search_matches;
static GtkWidget * treeview, * filter_entry, * queue_button;
static gboolean watching = false;

static void destroy_cb (void)
{
    if (watching)
    {
        hook_dissociate ("playlist update", update_cb);
        hook_dissociate ("playlist activate", activate_cb);
        watching = false;
    }

    cache.clear ();

    search_matches = nullptr;
}

static int get_selected_entry (void)
{
    g_return_val_if_fail (treeview && search_matches, -1);

    GtkTreeModel * model = gtk_tree_view_get_model ((GtkTreeView *) treeview);
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *) treeview);
    GtkTreeIter iter;

    if (! gtk_tree_selection_get_selected (selection, nullptr, & iter))
        return -1;

    GtkTreePath * path = gtk_tree_model_get_path (model, & iter);
    int row = gtk_tree_path_get_indices (path)[0];
    gtk_tree_path_free (path);

    g_return_val_if_fail (row >= 0 && row < search_matches->len (), -1);
    return (* search_matches)[row].entry;
}

static void do_jump (void * unused)
{
    int entry = get_selected_entry ();
    if (entry < 0)
        return;

    int playlist = aud_playlist_get_active ();
    aud_playlist_set_position (playlist, entry);
    aud_playlist_play (playlist);

    if (aud_get_bool ("audgui", "close_jtf_dialog"))
        audgui_jump_to_track_hide();
}

static void update_queue_button (int entry)
{
    g_return_if_fail (queue_button);

    if (entry < 0)
    {
        gtk_button_set_label ((GtkButton *) queue_button, _("_Queue"));
        gtk_widget_set_sensitive (queue_button, false);
    }
    else
    {
        if (aud_playlist_queue_find_entry (aud_playlist_get_active (), entry) != -1)
            gtk_button_set_label ((GtkButton *) queue_button, _("Un_queue"));
        else
            gtk_button_set_label ((GtkButton *) queue_button, _("_Queue"));

        gtk_widget_set_sensitive (queue_button, true);
    }
}

static void do_queue (void * unused)
{
    int playlist = aud_playlist_get_active ();
    int entry = get_selected_entry ();
    if (entry < 0)
        return;

    int queued = aud_playlist_queue_find_entry (playlist, entry);
    if (queued >= 0)
        aud_playlist_queue_delete (playlist, queued, 1);
    else
        aud_playlist_queue_insert (playlist, -1, entry);

    update_queue_button (entry);
}

static void selection_changed (void)
{
    update_queue_button (get_selected_entry ());
}

static gboolean keypress_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        audgui_jump_to_track_hide();
        return true;
    }

    return false;
}

static void fill_list (void)
{
    g_return_if_fail (treeview && filter_entry);

    search_matches = cache.search (gtk_entry_get_text ((GtkEntry *) filter_entry));

    audgui_list_delete_rows (treeview, 0, audgui_list_row_count (treeview));
    audgui_list_insert_rows (treeview, 0, search_matches->len ());

    if (search_matches->len () >= 1)
    {
        GtkTreeSelection * sel = gtk_tree_view_get_selection ((GtkTreeView *) treeview);
        GtkTreePath * path = gtk_tree_path_new_from_indices (0, -1);
        gtk_tree_selection_select_path (sel, path);
        gtk_tree_path_free (path);
    }
}

static void update_cb (void * level, void *)
{
    g_return_if_fail (treeview);

    GtkTreeModel * model;
    GtkTreeIter iter;
    GtkTreePath * path = nullptr;

    if (level <= PLAYLIST_UPDATE_SELECTION)
        return;

    cache.clear ();

    /* If it's only a metadata update, save and restore the cursor position. */
    if (level <= PLAYLIST_UPDATE_METADATA &&
     gtk_tree_selection_get_selected (gtk_tree_view_get_selection
     ((GtkTreeView *) treeview), & model, & iter))
        path = gtk_tree_model_get_path (model, & iter);

    fill_list ();

    if (path != nullptr)
    {
        gtk_tree_selection_select_path (gtk_tree_view_get_selection
         ((GtkTreeView *) treeview), path);
        gtk_tree_view_scroll_to_cell ((GtkTreeView *) treeview, path, nullptr, true, 0.5, 0);
        gtk_tree_path_free (path);
    }
}

static void activate_cb (void * data, void *)
{
    update_cb (PLAYLIST_UPDATE_STRUCTURE, nullptr);
}

static void toggle_button_cb (GtkToggleButton * toggle)
{
    aud_set_bool ("audgui", "close_jtf_dialog", gtk_toggle_button_get_active (toggle));
}

static void list_get_value (void * user, int row, int column, GValue * value)
{
    g_return_if_fail (search_matches);
    g_return_if_fail (column >= 0 && column < 2);
    g_return_if_fail (row >= 0 && row < search_matches->len ());

    int playlist = aud_playlist_get_active ();
    int entry = (* search_matches)[row].entry;

    switch (column)
    {
    case 0:
        g_value_set_int (value, 1 + entry);
        break;
    case 1:;
        g_value_set_string (value, aud_playlist_entry_get_title (playlist, entry, true));
        break;
    }
}

static const AudguiListCallbacks callbacks = {
    list_get_value
};

static GtkWidget * create_window (void)
{
    GtkWidget * jump_to_track_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(jump_to_track_win),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(jump_to_track_win), _("Jump to Song"));

    g_signal_connect (jump_to_track_win, "key_press_event", (GCallback) keypress_cb, nullptr);
    g_signal_connect (jump_to_track_win, "destroy", (GCallback) destroy_cb, nullptr);

    gtk_container_set_border_width(GTK_CONTAINER(jump_to_track_win), 10);
    gtk_window_set_default_size(GTK_WINDOW(jump_to_track_win), 600, 500);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_add(GTK_CONTAINER(jump_to_track_win), vbox);

    treeview = audgui_list_new (& callbacks, nullptr, 0);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) treeview, false);

    audgui_list_add_column (treeview, nullptr, 0, G_TYPE_INT, 7);
    audgui_list_add_column (treeview, nullptr, 1, G_TYPE_STRING, -1);

    g_signal_connect (gtk_tree_view_get_selection ((GtkTreeView *) treeview),
     "changed", (GCallback) selection_changed, nullptr);
    g_signal_connect (treeview, "row-activated", (GCallback) do_jump, nullptr);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 3);

    /* filter box */
    GtkWidget * search_label = gtk_label_new (_("Filter: "));
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(search_label), _("_Filter:"));
    gtk_box_pack_start(GTK_BOX(hbox), search_label, false, false, 0);

    filter_entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget ((GtkLabel *) search_label, filter_entry);
    g_signal_connect (filter_entry, "changed", (GCallback) fill_list, nullptr);
    gtk_entry_set_activates_default ((GtkEntry *) filter_entry, true);
    gtk_box_pack_start ((GtkBox *) hbox, filter_entry, true, true, 0);

    GtkWidget * scrollwin = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), scrollwin, true, true, 0);

    GtkWidget * bbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 4);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, false, false, 0);

    /* close dialog toggle */
    GtkWidget * toggle = gtk_check_button_new_with_mnemonic (_("C_lose on jump"));
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle, aud_get_bool
     ("audgui", "close_jtf_dialog"));
    gtk_box_pack_start(GTK_BOX(bbox), toggle, false, false, 0);
    g_signal_connect (toggle, "clicked", (GCallback) toggle_button_cb, nullptr);

    /* queue button */
    queue_button = audgui_button_new (_("_Queue"), nullptr, do_queue, nullptr);
    gtk_box_pack_start ((GtkBox *) bbox, queue_button, false, false, 0);

    /* jump button */
    GtkWidget * jump = audgui_button_new (_("_Jump"), "go-jump", do_jump, nullptr);
    gtk_box_pack_start(GTK_BOX(bbox), jump, false, false, 0);

    gtk_widget_set_can_default(jump, true);
    gtk_widget_grab_default(jump);

    /* close button */
    GtkWidget * close = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) audgui_jump_to_track_hide, nullptr);
    gtk_box_pack_start(GTK_BOX(bbox), close, false, false, 0);

    return jump_to_track_win;
}

EXPORT void audgui_jump_to_track (void)
{
    if (audgui_reshow_unique_window (AUDGUI_JUMP_TO_TRACK_WINDOW))
        return;

    GtkWidget * jump_to_track_win = create_window ();

    if (! watching)
    {
        fill_list ();
        hook_associate ("playlist update", update_cb, nullptr);
        hook_associate ("playlist activate", activate_cb, nullptr);
        watching = true;
    }

    gtk_widget_grab_focus (filter_entry);

    audgui_show_unique_window (AUDGUI_JUMP_TO_TRACK_WINDOW, jump_to_track_win);
}

EXPORT void audgui_jump_to_track_hide (void)
{
    audgui_hide_unique_window (AUDGUI_JUMP_TO_TRACK_WINDOW);
}
