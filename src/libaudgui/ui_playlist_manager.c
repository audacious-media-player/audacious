/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team.
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

#include <audacious/i18n.h>
#include <audacious/plugin.h>

#include "libaudgui.h"
#include "libaudgui-gtk.h"

static gint iter_to_row (GtkTreeModel * model, GtkTreeIter * iter)
{
    GtkTreePath * path = gtk_tree_model_get_path (model, iter);
    gint row = gtk_tree_path_get_indices (path)[0];
    
    gtk_tree_path_free (path);
    return row;
}

static gint get_selected_row (GtkWidget * list)
{
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *)
     list);
    GtkTreeModel * model;
    GtkTreeIter iter;

    if (! gtk_tree_selection_get_selected (selection, & model, & iter))
        return -1;
    
    return iter_to_row (model, & iter);
}

static void set_selected_row (GtkWidget * list, gint row)
{
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *)
     list);
    GtkTreePath * path = gtk_tree_path_new_from_indices (row, -1);

    gtk_tree_selection_select_path (selection, path);
    gtk_tree_path_free (path);
}

static void save_position (GtkWidget * window)
{
    gtk_window_get_position ((GtkWindow *) window,
     & aud_cfg->playlist_manager_x, & aud_cfg->playlist_manager_y);
    gtk_window_get_size ((GtkWindow *) window,
     & aud_cfg->playlist_manager_width, & aud_cfg->playlist_manager_height);
}

static gboolean hide_cb (GtkWidget * window)
{
    save_position (window);
    gtk_widget_hide (window);
    return TRUE;
}

static void activate_cb (GtkTreeView * list, GtkTreePath * path,
 GtkTreeViewColumn * column, GtkWidget * window)
{
    aud_playlist_set_active (gtk_tree_path_get_indices (path)[0]);
    
    if (aud_cfg->playlist_manager_close_on_activate)
        hide_cb (window);
}

static void new_cb (GtkButton * button, void * unused)
{
    aud_playlist_insert (-1);
}

static void delete_cb (GtkButton * button, GtkWidget * list)
{
    gint playlist = get_selected_row (list);

    if (playlist != -1)
        audgui_confirm_playlist_delete (playlist);
}

static void rename_cb (GtkButton * button, GtkWidget * lv)
{
    GtkTreeSelection *listsel = gtk_tree_view_get_selection( GTK_TREE_VIEW(lv) );
    GtkTreeModel *store;
    GtkTreeIter iter;

    if ( gtk_tree_selection_get_selected( listsel , &store , &iter ) == TRUE )
    {
        GtkTreePath *path = gtk_tree_model_get_path( GTK_TREE_MODEL(store) , &iter );
        GtkCellRenderer *rndrname = g_object_get_data( G_OBJECT(lv) , "rn" );
        /* set the name renderer to editable and start editing */
        g_object_set( G_OBJECT(rndrname) , "editable" , TRUE , NULL );
        gtk_tree_view_set_cursor_on_cell ((GtkTreeView *) lv, path,
         gtk_tree_view_get_column ((GtkTreeView *) lv,
         AUDGUI_LIBRARY_STORE_TITLE), rndrname, TRUE);
        gtk_tree_path_free( path );
    }
}

static void
playlist_manager_cb_lv_name_edited ( GtkCellRendererText *cell , gchar *path_string ,
                                     gchar *new_text , gpointer listview )
{
    /* this is currently used to change playlist names */
    GtkTreeModel *store = gtk_tree_view_get_model( GTK_TREE_VIEW(listview) );
    GtkTreeIter iter;

    if ( gtk_tree_model_get_iter_from_string( store , &iter , path_string ) == TRUE )
        aud_playlist_set_title (iter_to_row (store, & iter), new_text);

    /* set the renderer uneditable again */
    g_object_set( G_OBJECT(cell) , "editable" , FALSE , NULL );
}

static void save_config_cb (void * hook_data, void * user_data)
{
#if GTK_CHECK_VERSION (2, 18, 0)
    if (gtk_widget_get_visible ((GtkWidget *) user_data))
#else
    if (GTK_WIDGET_VISIBLE ((GtkWidget *) user_data))
#endif
        save_position ((GtkWidget *) user_data);
}

static GtkWidget * playman_win = NULL;

void
audgui_playlist_manager_ui_show (GtkWidget *mainwin)
{
    GtkWidget *playman_vbox;
    GtkWidget * playman_pl_lv, * playman_pl_lv_sw;
    GtkCellRenderer *playman_pl_lv_textrndr_name, *playman_pl_lv_textrndr_entriesnum;
    GtkTreeViewColumn *playman_pl_lv_col_name, *playman_pl_lv_col_entriesnum;
    GtkWidget *playman_bbar_hbbox;
    GtkWidget * rename_button, * new_button, * delete_button;
    GtkWidget * hbox, * button;
    GdkGeometry playman_win_hints;

    if ( playman_win != NULL )
    {
        gtk_window_present( GTK_WINDOW(playman_win) );
        return;
    }

    playman_win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint( GTK_WINDOW(playman_win), GDK_WINDOW_TYPE_HINT_DIALOG );
    gtk_window_set_transient_for( GTK_WINDOW(playman_win) , GTK_WINDOW(mainwin) );
    gtk_window_set_title( GTK_WINDOW(playman_win), _("Playlist Manager") );
    gtk_container_set_border_width ((GtkContainer *) playman_win, 6);
    playman_win_hints.min_width = 400;
    playman_win_hints.min_height = 250;
    gtk_window_set_geometry_hints( GTK_WINDOW(playman_win) , GTK_WIDGET(playman_win) ,
                                   &playman_win_hints , GDK_HINT_MIN_SIZE );

    if (aud_cfg->playlist_manager_width)
    {
        gtk_window_move ((GtkWindow *) playman_win, aud_cfg->playlist_manager_x,
         aud_cfg->playlist_manager_y);
        gtk_window_set_default_size ((GtkWindow *) playman_win,
         aud_cfg->playlist_manager_width, aud_cfg->playlist_manager_height);
    }

    g_signal_connect ((GObject *) playman_win, "delete-event", (GCallback)
     hide_cb, NULL);

    playman_vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add( GTK_CONTAINER(playman_win) , playman_vbox );

    playman_pl_lv = gtk_tree_view_new_with_model (audgui_get_library_store ());
    gtk_tree_view_set_reorderable ((GtkTreeView *) playman_pl_lv, TRUE);

    playman_pl_lv_textrndr_entriesnum = gtk_cell_renderer_text_new(); /* uneditable */
    playman_pl_lv_textrndr_name = gtk_cell_renderer_text_new(); /* can become editable */
    g_object_set( G_OBJECT(playman_pl_lv_textrndr_entriesnum) , "weight-set" , TRUE , NULL );
    g_object_set( G_OBJECT(playman_pl_lv_textrndr_name) , "weight-set" , TRUE , NULL );
    g_signal_connect( G_OBJECT(playman_pl_lv_textrndr_name) , "edited" ,
                      G_CALLBACK(playlist_manager_cb_lv_name_edited) , playman_pl_lv );
    g_object_set_data( G_OBJECT(playman_pl_lv) , "rn" , playman_pl_lv_textrndr_name );

    playman_pl_lv_col_name = gtk_tree_view_column_new_with_attributes
     (_("Playlist"), playman_pl_lv_textrndr_name, "text",
     AUDGUI_LIBRARY_STORE_TITLE, "weight", AUDGUI_LIBRARY_STORE_FONT_WEIGHT,
     NULL);
    gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_name) , TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_name );

    playman_pl_lv_col_entriesnum = gtk_tree_view_column_new_with_attributes
     (_("Entries"), playman_pl_lv_textrndr_entriesnum, "text",
     AUDGUI_LIBRARY_STORE_ENTRY_COUNT, "weight",
     AUDGUI_LIBRARY_STORE_FONT_WEIGHT, NULL);
    gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_entriesnum) , FALSE );
    gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_entriesnum );

    playman_pl_lv_sw = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) playman_pl_lv_sw,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add( GTK_CONTAINER(playman_pl_lv_sw) , playman_pl_lv );
    gtk_box_pack_start ((GtkBox *) playman_vbox, playman_pl_lv_sw, TRUE, TRUE, 0);

    /* button bar */
    playman_bbar_hbbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout( GTK_BUTTON_BOX(playman_bbar_hbbox) , GTK_BUTTONBOX_END );
    gtk_box_set_spacing ((GtkBox *) playman_bbar_hbbox, 6);

    rename_button = gtk_button_new_with_mnemonic (_("_Rename"));
    gtk_button_set_image ((GtkButton *) rename_button, gtk_image_new_from_stock
     (GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON));
    new_button = gtk_button_new_from_stock (GTK_STOCK_NEW);
    delete_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);

    gtk_container_add ((GtkContainer *) playman_bbar_hbbox, rename_button);
    gtk_button_box_set_child_secondary ((GtkButtonBox *) playman_bbar_hbbox,
     rename_button, TRUE);
    gtk_container_add ((GtkContainer *) playman_bbar_hbbox, new_button);
    gtk_container_add ((GtkContainer *) playman_bbar_hbbox, delete_button);

    gtk_box_pack_start( GTK_BOX(playman_vbox) , playman_bbar_hbbox , FALSE , FALSE , 0 );

    g_signal_connect ((GObject *) playman_pl_lv, "row-activated", (GCallback)
     activate_cb, playman_win);
    g_signal_connect ((GObject *) rename_button, "clicked", (GCallback)
     rename_cb, playman_pl_lv);
    g_signal_connect ((GObject *) new_button, "clicked", (GCallback) new_cb,
     playman_pl_lv);
    g_signal_connect ((GObject *) delete_button, "clicked", (GCallback)
     delete_cb, playman_pl_lv);

    set_selected_row (playman_pl_lv, aud_playlist_get_active ());

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) playman_vbox, hbox, FALSE, FALSE, 0);
    button = gtk_check_button_new_with_mnemonic
     (_("_Close dialog on activating playlist"));
    gtk_box_pack_start ((GtkBox *) hbox, button, FALSE, FALSE, 0);
    audgui_connect_check_box (button,
     & aud_cfg->playlist_manager_close_on_activate);

    gtk_widget_show_all( playman_win );
    
    aud_hook_associate ("config save", save_config_cb, playman_win);
}

void audgui_playlist_manager_destroy(void)
{
    if (playman_win)
        hide_cb (playman_win);
}
