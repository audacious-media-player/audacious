/*
 * ui_jumptotrack.c
 * Copyright 1998-2003 XMMS development team
 * Copyright 2003-2004 BMP development team
 * Copyright 2007-2011 Yoshiki Yazawa and John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <audacious/drct.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "libaudgui.h"
#include "list.h"
#include "ui_jumptotrack_cache.h"

static void update_cb (void * data, void * user);
static void activate_cb (void * data, void * user);

static GtkWidget *jump_to_track_win = NULL;
static JumpToTrackCache* cache = NULL;
static const GArray * search_matches;
static GtkWidget * treeview, * filter_entry, * queue_button;
static bool_t watching = FALSE;

EXPORT void
audgui_jump_to_track_hide(void)
{
    if (watching)
    {
        hook_dissociate ("playlist update", update_cb);
        hook_dissociate ("playlist activate", activate_cb);
        watching = FALSE;
    }

    if (jump_to_track_win != NULL)
        gtk_widget_hide (jump_to_track_win);

    if (cache != NULL)
    {
        ui_jump_to_track_cache_free (cache);
        cache = NULL;
    }

    search_matches = NULL;
}

static int get_selected_entry (void)
{
    g_return_val_if_fail (treeview && search_matches, -1);

    GtkTreeModel * model = gtk_tree_view_get_model ((GtkTreeView *) treeview);
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *) treeview);
    GtkTreeIter iter;

    if (! gtk_tree_selection_get_selected (selection, NULL, & iter))
        return -1;

    GtkTreePath * path = gtk_tree_model_get_path (model, & iter);
    int row = gtk_tree_path_get_indices (path)[0];
    gtk_tree_path_free (path);

    g_return_val_if_fail (row >= 0 && row < search_matches->len, -1);
    return g_array_index (search_matches, int, row);
}

static void do_jump (void)
{
    int entry = get_selected_entry ();
    if (entry < 0)
        return;

    int playlist = aud_playlist_get_active ();
    aud_playlist_set_playing (playlist);
    aud_playlist_set_position (playlist, entry);
    aud_drct_play ();

    if (aud_get_bool ("audgui", "close_jtf_dialog"))
        audgui_jump_to_track_hide();
}

static void update_queue_button (int entry)
{
    g_return_if_fail (queue_button);

    if (entry < 0)
    {
        gtk_button_set_label ((GtkButton *) queue_button, _("_Queue"));
        gtk_widget_set_sensitive (queue_button, FALSE);
    }
    else
    {
        if (aud_playlist_queue_find_entry (aud_playlist_get_active (), entry) != -1)
            gtk_button_set_label ((GtkButton *) queue_button, _("Un_queue"));
        else
            gtk_button_set_label ((GtkButton *) queue_button, _("_Queue"));

        gtk_widget_set_sensitive (queue_button, TRUE);
    }
}

static void do_queue (void)
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

static bool_t keypress_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_Escape)
    {
        audgui_jump_to_track_hide();
        return TRUE;
    }

    return FALSE;
}

static void fill_list (void)
{
    g_return_if_fail (treeview && filter_entry);

    if (! cache)
        cache = ui_jump_to_track_cache_new();

    search_matches = ui_jump_to_track_cache_search (cache, gtk_entry_get_text
     ((GtkEntry *) filter_entry));

    audgui_list_delete_rows (treeview, 0, audgui_list_row_count (treeview));
    audgui_list_insert_rows (treeview, 0, search_matches->len);
}

static void clear_cb (GtkWidget * widget)
{
    g_return_if_fail (filter_entry);
    gtk_entry_set_text ((GtkEntry *) filter_entry, "");
    gtk_widget_grab_focus (filter_entry);
}

static void update_cb (void * data, void * user)
{
    g_return_if_fail (treeview);

    GtkTreeModel * model;
    GtkTreeIter iter;
    GtkTreePath * path = NULL;

    if (GPOINTER_TO_INT (data) <= PLAYLIST_UPDATE_SELECTION)
        return;

    if (cache != NULL)
    {
        ui_jump_to_track_cache_free (cache);
        cache = NULL;
    }

    /* If it's only a metadata update, save and restore the cursor position. */
    if (GPOINTER_TO_INT (data) <= PLAYLIST_UPDATE_METADATA &&
     gtk_tree_selection_get_selected (gtk_tree_view_get_selection
     ((GtkTreeView *) treeview), & model, & iter))
        path = gtk_tree_model_get_path (model, & iter);

    fill_list ();

    if (path != NULL)
    {
        gtk_tree_selection_select_path (gtk_tree_view_get_selection
         ((GtkTreeView *) treeview), path);
        gtk_tree_view_scroll_to_cell ((GtkTreeView *) treeview, path, NULL, TRUE, 0.5, 0);
        gtk_tree_path_free (path);
    }
}

static void activate_cb (void * data, void * user)
{
    update_cb (GINT_TO_POINTER (PLAYLIST_UPDATE_STRUCTURE), NULL);
}

static void toggle_button_cb (GtkToggleButton * toggle, const char * setting)
{
    aud_set_bool ("audgui", setting, gtk_toggle_button_get_active (toggle));
}

static bool_t delete_cb (void)
{
    audgui_jump_to_track_hide ();
    return TRUE;
}

static void list_get_value (void * user, int row, int column, GValue * value)
{
    g_return_if_fail (search_matches);
    g_return_if_fail (column >= 0 && column < 2);
    g_return_if_fail (row >= 0 && row < search_matches->len);

    int playlist = aud_playlist_get_active ();
    int entry = g_array_index (search_matches, int, row);

    switch (column)
    {
    case 0:
        g_value_set_int (value, 1 + entry);
        break;
    case 1:;
        char * title = aud_playlist_entry_get_title (playlist, entry, TRUE);
        g_return_if_fail (title);
        g_value_set_string (value, title);
        str_unref (title);
        break;
    }
}

static const AudguiListCallbacks callbacks = {
 .get_value = list_get_value};

static void create_window (void)
{
    jump_to_track_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(jump_to_track_win),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(jump_to_track_win), _("Jump to Song"));

    g_signal_connect (jump_to_track_win, "key_press_event", (GCallback) keypress_cb, NULL);
    g_signal_connect (jump_to_track_win, "delete-event", (GCallback) delete_cb, NULL);

    gtk_container_set_border_width(GTK_CONTAINER(jump_to_track_win), 10);
    gtk_window_set_default_size(GTK_WINDOW(jump_to_track_win), 600, 500);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_add(GTK_CONTAINER(jump_to_track_win), vbox);

    treeview = audgui_list_new (& callbacks, NULL, 0);
    audgui_list_add_column (treeview, NULL, 0, G_TYPE_INT, 7);
    audgui_list_add_column (treeview, NULL, 1, G_TYPE_STRING, -1);

    g_signal_connect (gtk_tree_view_get_selection ((GtkTreeView *) treeview),
     "changed", (GCallback) selection_changed, NULL);
    g_signal_connect (treeview, "row-activated", (GCallback) do_jump, NULL);

    GtkWidget * hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    /* filter box */
    GtkWidget * search_label = gtk_label_new (_("Filter: "));
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(search_label), _("_Filter:"));
    gtk_box_pack_start(GTK_BOX(hbox), search_label, FALSE, FALSE, 0);

    filter_entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget ((GtkLabel *) search_label, filter_entry);
    g_signal_connect (filter_entry, "changed", (GCallback) fill_list, NULL);
    gtk_entry_set_activates_default ((GtkEntry *) filter_entry, TRUE);
    gtk_box_pack_start ((GtkBox *) hbox, filter_entry, TRUE, TRUE, 3);

    /* remember text entry */
    GtkWidget * toggle2 = gtk_check_button_new_with_label (_("Remember"));
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle2, aud_get_bool
     ("audgui", "remember_jtf_entry"));
    gtk_box_pack_start(GTK_BOX(hbox), toggle2, FALSE, FALSE, 0);
    g_signal_connect (toggle2, "clicked", (GCallback) toggle_button_cb, "remember_jtf_entry");

    /* clear button */
    GtkWidget * rescan = gtk_button_new_with_mnemonic (_("Clea_r"));
    gtk_button_set_image ((GtkButton *) rescan, gtk_image_new_from_stock
     (GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(hbox), rescan, FALSE, FALSE, 0);
    g_signal_connect (rescan, "clicked", (GCallback) clear_cb, NULL);

    GtkWidget * scrollwin = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);

    GtkWidget * bbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 4);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    /* close dialog toggle */
    GtkWidget * toggle = gtk_check_button_new_with_label(_("Close on Jump"));
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle, aud_get_bool
     ("audgui", "close_jtf_dialog"));
    gtk_box_pack_start(GTK_BOX(bbox), toggle, FALSE, FALSE, 0);
    g_signal_connect (toggle, "clicked", (GCallback) toggle_button_cb, "close_jtf_dialog");

    /* queue button */
    queue_button = gtk_button_new_with_mnemonic(_("_Queue"));
    gtk_button_set_image ((GtkButton *) queue_button, gtk_image_new_from_stock
     (AUD_STOCK_QUEUETOGGLE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start ((GtkBox *) bbox, queue_button, FALSE, FALSE, 0);

    g_signal_connect (queue_button, "clicked", (GCallback) do_queue, NULL);

    /* jump button */
    GtkWidget * jump = gtk_button_new_from_stock (GTK_STOCK_JUMP_TO);
    gtk_box_pack_start(GTK_BOX(bbox), jump, FALSE, FALSE, 0);

    g_signal_connect (jump, "clicked", (GCallback) do_jump, NULL);

    gtk_widget_set_can_default(jump, TRUE);
    gtk_widget_grab_default(jump);

    /* close button */
    GtkWidget * close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(bbox), close, FALSE, FALSE, 0);
    g_signal_connect (close, "clicked", (GCallback) audgui_jump_to_track_hide,
     NULL);
    gtk_widget_set_can_default(close, TRUE);
}

EXPORT void audgui_jump_to_track (void)
{
    bool_t create = (! jump_to_track_win);
    if (create)
        create_window ();

    g_return_if_fail (filter_entry);

    if (aud_get_bool ("audgui", "remember_jtf_entry"))
        gtk_editable_select_region ((GtkEditable *) filter_entry, 0, -1);
    else
        gtk_entry_set_text ((GtkEntry *) filter_entry, "");

    if (! watching)
    {
        fill_list ();
        hook_associate ("playlist update", update_cb, NULL);
        hook_associate ("playlist activate", activate_cb, NULL);
        watching = TRUE;
    }

    if (create)
        gtk_widget_show_all (jump_to_track_win);
    else
        gtk_window_present ((GtkWindow *) jump_to_track_win);

    gtk_widget_grab_focus (filter_entry);
}
