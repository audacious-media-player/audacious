/*
 * ui_playlist_manager.c
 * Copyright 2006-2012 Giacomo Lozito, John Lindgren, and Thomas Lange
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

#include <string.h>
#include <gtk/gtk.h>

#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/drct.h>
#include <audacious/playlist.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "list.h"

static void activate_row (void * user, int row);

static void play_cb (void * unused)
{
    activate_row (NULL, aud_playlist_get_active ());
}

static void rename_cb (void * unused)
{
    audgui_show_playlist_rename (aud_playlist_get_active ());
}

static void new_cb (void * unused)
{
    aud_playlist_insert (aud_playlist_get_active () + 1);
    aud_playlist_set_active (aud_playlist_get_active () + 1);
}

static void delete_cb (void * unused)
{
    audgui_confirm_playlist_delete (aud_playlist_get_active ());
}

static void get_value (void * user, int row, int column, GValue * value)
{
    switch (column)
    {
    case 0:;
        char * title = aud_playlist_get_title (row);
        g_value_set_string (value, title);
        str_unref (title);
        break;
    case 1:
        g_value_set_int (value, aud_playlist_entry_count (row));
        break;
    }
}

static bool_t get_selected (void * user, int row)
{
    return (row == aud_playlist_get_active ());
}

static void set_selected (void * user, int row, bool_t selected)
{
    if (selected)
        aud_playlist_set_active (row);
}

static void select_all (void * user, bool_t selected)
{
}

static void activate_row (void * user, int row)
{
    aud_playlist_set_active (row);
    aud_drct_play_playlist (row);

    if (aud_get_bool ("audgui", "playlist_manager_close_on_activate"))
        audgui_hide_unique_window (AUDGUI_PLAYLIST_MANAGER_WINDOW);
}

static void shift_rows (void * user, int row, int before)
{
    if (before < row)
        aud_playlist_reorder (row, before, 1);
    else if (before - 1 > row)
        aud_playlist_reorder (row, before - 1, 1);
}

static const AudguiListCallbacks callbacks = {
 .get_value = get_value,
 .get_selected = get_selected,
 .set_selected = set_selected,
 .select_all = select_all,
 .activate_row = activate_row,
 .right_click = NULL,
 .shift_rows = shift_rows,
 .data_type = NULL,
 .get_data = NULL,
 .receive_data = NULL};

static bool_t search_cb (GtkTreeModel * model, int column, const char * key,
 GtkTreeIter * iter, void * user)
{
    GtkTreePath * path = gtk_tree_model_get_path (model, iter);
    g_return_val_if_fail (path, TRUE);
    int row = gtk_tree_path_get_indices (path)[0];
    gtk_tree_path_free (path);

    char * title = aud_playlist_get_title (row);
    g_return_val_if_fail (title, TRUE);

    Index * keys = str_list_to_index (key, " ");
    int count = index_count (keys);

    bool_t match = FALSE;

    for (int i = 0; i < count; i ++)
    {
        if (strstr_nocase_utf8 (title, index_get (keys, i)))
            match = TRUE;
        else
        {
            match = FALSE;
            break;
        }
    }

    index_free_full (keys, (IndexFreeFunc) str_unref);
    str_unref (title);

    return ! match; /* TRUE == not matched, FALSE == matched */
}

static bool_t position_changed = FALSE;
static bool_t playlist_activated = FALSE;

static void update_hook (void * data, void * list)
{
    int rows = aud_playlist_count ();

    if (GPOINTER_TO_INT (data) == PLAYLIST_UPDATE_STRUCTURE)
    {
        int old_rows = audgui_list_row_count (list);

        if (rows < old_rows)
            audgui_list_delete_rows (list, rows, old_rows - rows);
        else if (rows > old_rows)
            audgui_list_insert_rows (list, old_rows, rows - old_rows);

        position_changed = TRUE;
        playlist_activated = TRUE;
    }

    if (GPOINTER_TO_INT (data) >= PLAYLIST_UPDATE_METADATA)
        audgui_list_update_rows (list, 0, rows);

    if (playlist_activated)
    {
        audgui_list_set_focus (list, aud_playlist_get_active ());
        audgui_list_update_selection (list, 0, rows);
        playlist_activated = FALSE;
    }

    if (position_changed)
    {
        audgui_list_set_highlight (list, aud_playlist_get_playing ());
        position_changed = FALSE;
    }
}

static void activate_hook (void * data, void * list)
{
    if (aud_playlist_update_pending ())
        playlist_activated = TRUE;
    else
    {
        audgui_list_set_focus (list, aud_playlist_get_active ());
        audgui_list_update_selection (list, 0, aud_playlist_count ());
    }
}

static void position_hook (void * data, void * list)
{
    if (aud_playlist_update_pending ())
        position_changed = TRUE;
    else
        audgui_list_set_highlight (list, aud_playlist_get_playing ());
}

static void close_on_activate_cb (GtkToggleButton * toggle)
{
    aud_set_bool ("audgui", "playlist_manager_close_on_activate",
     gtk_toggle_button_get_active (toggle));
}

static void destroy_cb (GtkWidget * window)
{
    hook_dissociate ("playlist update", update_hook);
    hook_dissociate ("playlist activate", activate_hook);
    hook_dissociate ("playlist set playing", position_hook);
}

static GtkWidget * create_playlist_manager (void)
{
    GtkWidget * playman_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) playman_win, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title ((GtkWindow *) playman_win, _("Playlist Manager"));
    gtk_container_set_border_width ((GtkContainer *) playman_win, 6);
    gtk_widget_set_size_request (playman_win, 400, 250);

    g_signal_connect (playman_win, "destroy", (GCallback) destroy_cb, NULL);
    audgui_destroy_on_escape (playman_win);

    GtkWidget * playman_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add ((GtkContainer *) playman_win, playman_vbox);

    /* ListView */
    GtkWidget * playman_pl_lv = audgui_list_new (& callbacks, NULL, aud_playlist_count ());
    audgui_list_add_column (playman_pl_lv, _("Title"), 0, G_TYPE_STRING, -1);
    audgui_list_add_column (playman_pl_lv, _("Entries"), 1, G_TYPE_INT, 7);
    audgui_list_set_highlight (playman_pl_lv, aud_playlist_get_playing ());
    gtk_tree_view_set_search_equal_func ((GtkTreeView *) playman_pl_lv,
     search_cb, NULL, NULL);
    hook_associate ("playlist update", update_hook, playman_pl_lv);
    hook_associate ("playlist activate", activate_hook, playman_pl_lv);
    hook_associate ("playlist set playing", position_hook, playman_pl_lv);

    GtkWidget * playman_pl_lv_sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add ((GtkContainer *) playman_pl_lv_sw, playman_pl_lv);
    gtk_box_pack_start ((GtkBox *) playman_vbox, playman_pl_lv_sw, TRUE, TRUE, 0);

    /* ButtonBox */
    GtkWidget * playman_button_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget * new_button = audgui_button_new (_("_New"), "document-new", new_cb, NULL);
    GtkWidget * delete_button = audgui_button_new (_("_Remove"), "edit-delete", delete_cb, NULL);
    GtkWidget * rename_button = audgui_button_new (_("Ren_ame"), "insert-text", rename_cb, NULL);
    GtkWidget * play_button = audgui_button_new (_("_Play"), "media-playback-start", play_cb, NULL);

    gtk_container_add ((GtkContainer *) playman_button_hbox, new_button);
    gtk_container_add ((GtkContainer *) playman_button_hbox, delete_button);
    gtk_box_pack_end ((GtkBox *) playman_button_hbox, play_button, FALSE, FALSE, 0);
    gtk_box_pack_end ((GtkBox *) playman_button_hbox, rename_button, FALSE, FALSE, 0);
    gtk_container_add ((GtkContainer *) playman_vbox, playman_button_hbox);

    /* CheckButton */
    GtkWidget * hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start ((GtkBox *) playman_vbox, hbox, FALSE, FALSE, 0);
    GtkWidget * check_button = gtk_check_button_new_with_mnemonic
     (_("_Close dialog on activating playlist"));
    gtk_box_pack_start ((GtkBox *) hbox, check_button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active ((GtkToggleButton *) check_button, aud_get_bool
     ("audgui", "playlist_manager_close_on_activate"));
    g_signal_connect (check_button, "toggled", (GCallback) close_on_activate_cb, NULL);

    return playman_win;
}

EXPORT void audgui_playlist_manager (void)
{
    if (! audgui_reshow_unique_window (AUDGUI_PLAYLIST_MANAGER_WINDOW))
        audgui_show_unique_window (AUDGUI_PLAYLIST_MANAGER_WINDOW, create_playlist_manager ());
}
