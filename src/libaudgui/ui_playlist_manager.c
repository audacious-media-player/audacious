/*
 * ui_playlist_manager.c
 * Copyright 2006-2011 Giacomo Lozito and John Lindgren
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
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "list.h"

static GtkWidget * playman_win = NULL;
static void activate_row (void * user, int row);

static void save_position (GtkWidget * window)
{
    int x, y, w, h;
    gtk_window_get_position ((GtkWindow *) window, & x, & y);
    gtk_window_get_size ((GtkWindow *) window, & w, & h);

    aud_set_int ("audgui", "playlist_manager_x", x);
    aud_set_int ("audgui", "playlist_manager_y", y);
    aud_set_int ("audgui", "playlist_manager_w", w);
    aud_set_int ("audgui", "playlist_manager_h", h);
}

static bool_t hide_cb (GtkWidget * window)
{
    save_position (window);
    gtk_widget_hide (window);
    return TRUE;
}

static void play_cb (void)
{
    activate_row (NULL, aud_playlist_get_active ());
}

static void rename_cb (void)
{
    audgui_show_playlist_rename (aud_playlist_get_active ());
}

static void new_cb (void)
{
    aud_playlist_insert (aud_playlist_get_active () + 1);
    aud_playlist_set_active (aud_playlist_get_active () + 1);
}

static void delete_cb (void)
{
    audgui_confirm_playlist_delete (aud_playlist_get_active ());
}

static void save_config_cb (void * hook_data, void * user_data)
{
    if (gtk_widget_get_visible ((GtkWidget *) user_data))
        save_position ((GtkWidget *) user_data);
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
        hide_cb (playman_win);
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

    char * temp = aud_playlist_get_title (row);
    g_return_val_if_fail (temp, TRUE);
    char * title = g_utf8_strdown (temp, -1);
    str_unref (temp);

    temp = g_utf8_strdown (key, -1);
    char * * keys = g_strsplit (temp, " ", 0);
    g_free (temp);

    bool_t match = FALSE;

    for (int i = 0; keys[i]; i ++)
    {
        if (! keys[i][0])
            continue;

        if (strstr (title, keys[i]))
            match = TRUE;
        else
        {
            match = FALSE;
            break;
        }
    }

    g_free (title);
    g_strfreev (keys);

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

EXPORT void audgui_playlist_manager (void)
{
    GtkWidget * playman_vbox;
    GtkWidget * playman_pl_lv, * playman_pl_lv_sw;
    GtkWidget * playman_button_hbox;
    GtkWidget * new_button, * delete_button, * rename_button, * play_button;
    GtkWidget * hbox, * check_button;
    GdkGeometry playman_win_hints;

    if (playman_win)
    {
        gtk_window_present ((GtkWindow *) playman_win);
        return;
    }

    playman_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) playman_win, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title ((GtkWindow *) playman_win, _("Playlist Manager"));
    gtk_container_set_border_width ((GtkContainer *) playman_win, 6);
    playman_win_hints.min_width = 400;
    playman_win_hints.min_height = 250;
    gtk_window_set_geometry_hints ((GtkWindow *) playman_win, playman_win,
                                   &playman_win_hints , GDK_HINT_MIN_SIZE);

    int x = aud_get_int ("audgui", "playlist_manager_x");
    int y = aud_get_int ("audgui", "playlist_manager_y");
    int w = aud_get_int ("audgui", "playlist_manager_w");
    int h = aud_get_int ("audgui", "playlist_manager_h");

    if (w && h)
    {
        gtk_window_move ((GtkWindow *) playman_win, x, y);
        gtk_window_set_default_size ((GtkWindow *) playman_win, w, h);
    }

    g_signal_connect (playman_win, "delete-event", (GCallback) hide_cb, NULL);
    audgui_hide_on_escape (playman_win);

    playman_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add ((GtkContainer *) playman_win, playman_vbox);

    /* ListView */
    playman_pl_lv = audgui_list_new (& callbacks, NULL, aud_playlist_count ());
    audgui_list_add_column (playman_pl_lv, _("Title"), 0, G_TYPE_STRING, -1);
    audgui_list_add_column (playman_pl_lv, _("Entries"), 1, G_TYPE_INT, 7);
    audgui_list_set_highlight (playman_pl_lv, aud_playlist_get_playing ());
    gtk_tree_view_set_search_equal_func ((GtkTreeView *) playman_pl_lv,
     search_cb, NULL, NULL);
    hook_associate ("playlist update", update_hook, playman_pl_lv);
    hook_associate ("playlist activate", activate_hook, playman_pl_lv);
    hook_associate ("playlist set playing", position_hook, playman_pl_lv);

    playman_pl_lv_sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add ((GtkContainer *) playman_pl_lv_sw, playman_pl_lv);
    gtk_box_pack_start ((GtkBox *) playman_vbox, playman_pl_lv_sw, TRUE, TRUE, 0);

    /* ButtonBox */
    playman_button_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    new_button = gtk_button_new_from_stock (GTK_STOCK_NEW);
    delete_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
    rename_button = gtk_button_new_with_mnemonic (_("_Rename"));
    gtk_button_set_image ((GtkButton *) rename_button, gtk_image_new_from_stock
     (GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON));
    play_button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);

    gtk_container_add ((GtkContainer *) playman_button_hbox, new_button);
    gtk_container_add ((GtkContainer *) playman_button_hbox, delete_button);
    gtk_box_pack_end ((GtkBox *) playman_button_hbox, play_button, FALSE, FALSE, 0);
    gtk_box_pack_end ((GtkBox *) playman_button_hbox, rename_button, FALSE, FALSE, 0);
    gtk_container_add ((GtkContainer *) playman_vbox, playman_button_hbox);

    g_signal_connect (new_button, "clicked", (GCallback) new_cb, NULL);
    g_signal_connect (delete_button, "clicked", (GCallback) delete_cb, NULL);
    g_signal_connect (rename_button, "clicked", (GCallback) rename_cb, NULL);
    g_signal_connect (play_button, "clicked", (GCallback) play_cb, NULL);

    /* CheckButton */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start ((GtkBox *) playman_vbox, hbox, FALSE, FALSE, 0);
    check_button = gtk_check_button_new_with_mnemonic
     (_("_Close dialog on activating playlist"));
    gtk_box_pack_start ((GtkBox *) hbox, check_button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active ((GtkToggleButton *) check_button, aud_get_bool
     ("audgui", "playlist_manager_close_on_activate"));
    g_signal_connect (check_button, "toggled", (GCallback) close_on_activate_cb, NULL);

    gtk_widget_show_all (playman_win);

    hook_associate ("config save", save_config_cb, playman_win);
}

void audgui_playlist_manager_cleanup (void)
{
    if (! playman_win)
        return;

    hook_dissociate ("playlist update", update_hook);
    hook_dissociate ("playlist activate", activate_hook);
    hook_dissociate ("playlist set playing", position_hook);
    hook_dissociate ("config save", save_config_cb);

    gtk_widget_destroy (playman_win);
    playman_win = NULL;
}
