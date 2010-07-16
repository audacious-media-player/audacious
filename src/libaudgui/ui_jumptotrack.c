/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#if defined(USE_REGEX_ONIGURUMA)
  #include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
  #include <pcreposix.h>
#else
  #include <regex.h>
#endif

#include <audacious/audconfig.h>
#include <audacious/drct.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "icons-stock.h"
#include "ui_jumptotrack_cache.h"

static void watchdog (void * hook_data, void * user_data);

static GtkWidget *jump_to_track_win = NULL;
static JumpToTrackCache* cache = NULL;
static void * storage = NULL;
static gboolean watching = FALSE;

static void change_song (guint pos)
{
    gint playlist = aud_playlist_get_active ();

    aud_playlist_set_playing (playlist);
    aud_playlist_set_position (playlist, pos);
    aud_drct_play ();
}

void
audgui_jump_to_track_hide(void)
{
    if (watching)
    {
        hook_dissociate ("playlist update", watchdog);
        watching = FALSE;
    }

    if (jump_to_track_win != NULL)
        gtk_widget_hide (jump_to_track_win);

    if (cache != NULL)
    {
        ui_jump_to_track_cache_free (cache);
        cache = NULL;
    }
}

static void
ui_jump_to_track_jump(GtkTreeView * treeview)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    change_song(pos - 1);

    if(aud_cfg->close_jtf_dialog)
        audgui_jump_to_track_hide();
}

static void
ui_jump_to_track_toggle_cb(GtkWidget * toggle)
{
    aud_cfg->close_jtf_dialog =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
}

static void
ui_jump_to_track_toggle2_cb(GtkWidget * toggle)
{
    aud_cfg->remember_jtf_entry =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
}

static void
ui_jump_to_track_jump_cb(GtkTreeView * treeview,
                             gpointer data)
{
    ui_jump_to_track_jump(treeview);
}

static void
ui_jump_to_track_set_queue_button_label(GtkButton * button,
                                      guint pos)
{
    if (aud_playlist_queue_find_entry (aud_playlist_get_active (), pos) != -1)
        gtk_button_set_label(button, _("Un_queue"));
    else
        gtk_button_set_label(button, _("_Queue"));
}

static void
ui_jump_to_track_queue_cb(GtkButton * button,
                              gpointer data)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    treeview = GTK_TREE_VIEW(data);
    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    if (aud_drct_pq_is_queued (pos - 1))
        aud_drct_pq_remove (pos - 1);
    else
        aud_drct_pq_add (pos - 1);

    ui_jump_to_track_set_queue_button_label(button, (pos - 1));
}

static void
ui_jump_to_track_selection_changed_cb(GtkTreeSelection *treesel,
                                          gpointer data)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    treeview = gtk_tree_selection_get_tree_view(treesel);
    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    ui_jump_to_track_set_queue_button_label(GTK_BUTTON(data), (pos - 1));
}

static gboolean
ui_jump_to_track_edit_keypress_cb(GtkWidget * object,
                 GdkEventKey * event,
                 gpointer data)
{
    switch (event->keyval) {
    case GDK_Return:
        if (gtk_im_context_filter_keypress (GTK_ENTRY (object)->im_context, event)) {
            GTK_ENTRY (object)->need_im_reset = TRUE;
            return TRUE;
        } else {
            ui_jump_to_track_jump(GTK_TREE_VIEW(data));
            return TRUE;
        }
    default:
        return FALSE;
    }
}

static gboolean
ui_jump_to_track_keypress_cb(GtkWidget * object,
                                 GdkEventKey * event,
                                 gpointer data)
{
    switch (event->keyval) {
    case GDK_Escape:
        audgui_jump_to_track_hide();
        return TRUE;
    case GDK_KP_Enter:
        ui_jump_to_track_queue_cb(NULL, data);
        return TRUE;
    default:
        return FALSE;
    };

    return FALSE;
}

static void
ui_jump_to_track_edit_cb(GtkEntry * entry, gpointer user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeIter iter;
    gint playlist;
    GtkListStore *store;

    const GArray *search_matches;
    int i;

    if (cache == NULL) {
        cache = ui_jump_to_track_cache_new();
    }

    /* FIXME: Remove the connected signals before clearing
     * (row-selected will still eventually arrive once) */
    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

    /* detach model from treeview */
    g_object_ref( store );
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview) , NULL );

    gtk_list_store_clear(store);

    search_matches = ui_jump_to_track_cache_search(cache,
                                                   gtk_entry_get_text(entry));

    playlist = aud_playlist_get_active ();

    for (i = 0; i < search_matches->len; i++)
    {
        gint entry = g_array_index (search_matches, gint, i);
        const gchar * title = aud_playlist_entry_get_title (playlist, entry,
         TRUE);

        if (title == NULL)
            continue;

        gtk_list_store_append(store, &iter);
        gtk_list_store_set (store, & iter, 0, 1 + entry, 1, title, -1);
    }

    /* attach the model again to the treeview */
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview) , GTK_TREE_MODEL(store) );
    g_object_unref( store );

    if (gtk_tree_model_get_iter_first ((GtkTreeModel *) store, & iter))
        gtk_tree_selection_select_iter (gtk_tree_view_get_selection (treeview),
         & iter);
}

static gboolean
ui_jump_to_track_fill(gpointer treeview)
{
    gint playlist, entries, entry;
    GtkTreeIter iter;
    GtkListStore *jtf_store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );

    /* detach model from treeview before fill */
    g_object_ref(jtf_store);
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview), NULL );

    gtk_list_store_clear(jtf_store);

    playlist = aud_playlist_get_active ();
    entries = aud_playlist_entry_count (playlist);

    for (entry = 0; entry < entries; entry ++)
    {
        gtk_list_store_append(GTK_LIST_STORE(jtf_store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(jtf_store), &iter,
         0, 1 + entry, 1, aud_playlist_entry_get_title (playlist, entry, TRUE),
          -1);
    }

    /* attach liststore to treeview */
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(jtf_store));
    g_object_unref(jtf_store);
    return FALSE;
}

static void clear_cb (GtkWidget * widget, void * data)
{
    GtkWidget * entry = g_object_get_data (data, "edit");

    gtk_entry_set_text ((GtkEntry *) entry, "");
    gtk_widget_grab_focus (entry);
}

static void watchdog (void * hook_data, void * user_data)
{
    GtkTreeView * tree;
    GtkTreeModel * model;
    GtkTreeIter iter;
    GtkTreePath * path = NULL;

    if (GPOINTER_TO_INT (hook_data) <= PLAYLIST_UPDATE_SELECTION || storage ==
     NULL)
        return;

    if (cache != NULL)
    {
        ui_jump_to_track_cache_free (cache);
        cache = NULL;
    }

    tree = g_object_get_data (storage, "treeview");

    /* If it's only a metadata update, save and restore the cursor position. */
    if (GPOINTER_TO_INT (hook_data) <= PLAYLIST_UPDATE_METADATA &&
     gtk_tree_selection_get_selected (gtk_tree_view_get_selection (tree),
     & model, & iter))
        path = gtk_tree_model_get_path (model, & iter);

    /* Redo the search. */
    ui_jump_to_track_edit_cb (g_object_get_data (storage, "edit"), tree);

    if (path != NULL)
    {
        gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree), path);
        gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0);
        gtk_tree_path_free (path);
    }
}

static gboolean delete_cb (GtkWidget * widget, GdkEvent * event, void * unused)
{
    audgui_jump_to_track_hide ();
    return TRUE;
}

void
audgui_jump_to_track(void)
{
    GtkWidget *scrollwin;
    GtkWidget *vbox, *bbox, *sep;
    GtkWidget *toggle, *toggle2;
    GtkWidget *jump, *queue, *close;
    GtkWidget *rescan;
    GtkWidget *search_label, *hbox;
    static GtkWidget *edit;

    GtkWidget *treeview = NULL;
    GtkListStore *jtf_store;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    if (cache == NULL)
        cache = ui_jump_to_track_cache_new ();

    if (! watching)
    {
        hook_associate ("playlist update", watchdog, NULL);
        watching = TRUE;
    }

    if (jump_to_track_win) {
        gtk_window_present(GTK_WINDOW(jump_to_track_win));

        if(!aud_cfg->remember_jtf_entry)
            gtk_entry_set_text(GTK_ENTRY(edit), "");

        gtk_widget_grab_focus(edit);
        gtk_editable_select_region(GTK_EDITABLE(edit), 0, -1);
        return;
    }

    #if defined(USE_REGEX_ONIGURUMA)
    /* set encoding for Oniguruma regex to UTF-8 */
    reg_set_encoding( REG_POSIX_ENCODING_UTF8 );
    onig_set_default_syntax( ONIG_SYNTAX_POSIX_BASIC );
    #endif

    jump_to_track_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(jump_to_track_win),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(jump_to_track_win), _("Jump to Track"));

    gtk_window_set_position(GTK_WINDOW(jump_to_track_win), GTK_WIN_POS_CENTER);
    g_signal_connect (jump_to_track_win, "delete-event", (GCallback) delete_cb,
     NULL);

    gtk_container_set_border_width(GTK_CONTAINER(jump_to_track_win), 10);
    gtk_window_set_default_size(GTK_WINDOW(jump_to_track_win), 600, 500);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(jump_to_track_win), vbox);

    jtf_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(jtf_store));
    g_object_unref(jtf_store);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 0, NULL);
    gtk_tree_view_column_set_spacing(column, 4);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), 1);

    g_signal_connect(treeview, "row-activated",
                     G_CALLBACK(ui_jump_to_track_jump), NULL);

    hbox = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);


    /* filter box */
    search_label = gtk_label_new(_("Filter: "));
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(search_label), _("_Filter:"));
    gtk_box_pack_start(GTK_BOX(hbox), search_label, FALSE, FALSE, 0);

    edit = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(edit), TRUE);
    gtk_label_set_mnemonic_widget(GTK_LABEL(search_label), edit);
    g_signal_connect(edit, "changed",
                     G_CALLBACK(ui_jump_to_track_edit_cb), treeview);

    g_signal_connect(edit, "key_press_event",
                     G_CALLBACK(ui_jump_to_track_edit_keypress_cb), treeview);

    g_signal_connect(jump_to_track_win, "key_press_event",
                     G_CALLBACK(ui_jump_to_track_keypress_cb), treeview);

    gtk_box_pack_start(GTK_BOX(hbox), edit, TRUE, TRUE, 3);

    /* remember text entry */
    toggle2 = gtk_check_button_new_with_label(_("Remember"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle2),
                                 aud_cfg->remember_jtf_entry ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), toggle2, FALSE, FALSE, 0);
    g_signal_connect(toggle2, "clicked",
                     G_CALLBACK(ui_jump_to_track_toggle2_cb),
                     toggle2);

    /* clear button */
    rescan = gtk_button_new_with_mnemonic (_("Clea_r"));
    gtk_button_set_image ((GtkButton *) rescan, gtk_image_new_from_stock
     (GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(hbox), rescan, FALSE, FALSE, 0);


    /* pack to container */
    storage = g_object_new(G_TYPE_OBJECT, NULL);
    g_object_set_data(storage, "widget", rescan);
    g_object_set_data(storage, "treeview", treeview);
    g_object_set_data(storage, "edit", edit);

    g_signal_connect (rescan, "clicked", (GCallback) clear_cb, storage);

    GTK_WIDGET_SET_FLAGS(rescan, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(rescan);

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);

    sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 4);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    /* close dialog toggle */
    toggle = gtk_check_button_new_with_label(_("Close on Jump"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 aud_cfg->close_jtf_dialog ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(bbox), toggle, FALSE, FALSE, 0);
    g_signal_connect(toggle, "clicked",
                     G_CALLBACK(ui_jump_to_track_toggle_cb),
                     toggle);

    queue = gtk_button_new_with_mnemonic(_("_Queue"));
    gtk_button_set_image(GTK_BUTTON(queue),
                     gtk_image_new_from_stock(AUD_STOCK_QUEUETOGGLE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(bbox), queue, FALSE, FALSE, 0);
    GTK_WIDGET_SET_FLAGS(queue, GTK_CAN_DEFAULT);
    g_signal_connect(queue, "clicked",
                     G_CALLBACK(ui_jump_to_track_queue_cb),
                     treeview);
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed",
                     G_CALLBACK(ui_jump_to_track_selection_changed_cb),
                     queue);

    jump = gtk_button_new_from_stock(GTK_STOCK_JUMP_TO);
    gtk_box_pack_start(GTK_BOX(bbox), jump, FALSE, FALSE, 0);

    g_signal_connect_swapped(jump, "clicked",
                             G_CALLBACK(ui_jump_to_track_jump_cb),
                             treeview);

    GTK_WIDGET_SET_FLAGS(jump, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(jump);

    close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(bbox), close, FALSE, FALSE, 0);
    g_signal_connect (close, "clicked", (GCallback) audgui_jump_to_track_hide,
     NULL);
    GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);

    g_timeout_add(100, (GSourceFunc)ui_jump_to_track_fill, treeview);

    gtk_widget_show_all(jump_to_track_win);
    gtk_widget_grab_focus(edit);
}
