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

#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <audacious/i18n.h>
#include <audacious/plugin.h>

#include "ui_playlist_manager.h"

#define DISABLE_MANAGER_UPDATE() g_object_set_data(G_OBJECT(listview),"opt1",GINT_TO_POINTER(1))
#define ENABLE_MANAGER_UPDATE() g_object_set_data(G_OBJECT(listview),"opt1",GINT_TO_POINTER(0))

GtkWidget * playman_win = NULL;

/* in this enum, place the columns according to visualization order
   (information not displayed in columns should be placed right before PLLIST_NUMCOLS) */
enum
{
    PLLIST_COL_NAME = 0,
    PLLIST_COL_ENTRIESNUM,
    PLLIST_NUMBER,
    PLLIST_TEXT_WEIGHT,
    PLLIST_NUMCOLS
};

static gboolean escape_cb (GtkWidget * widget, GdkEventKey * event, void * data)
{
    if (event->keyval == GDK_Escape)
    {
        gtk_widget_destroy (widget);
        return TRUE;
    }

    return FALSE;
}

void widget_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect ((GObject *) widget, "key-press-event", (GCallback)
     escape_cb, NULL);
}

void check_button_toggled (GtkToggleButton * button, void * data)
{
    * (gboolean *) data = gtk_toggle_button_get_active (button);
}

static void confirm_delete_cb (GtkButton * button, void * data)
{
    if (GPOINTER_TO_INT (data) < aud_playlist_count ())
        aud_playlist_delete (GPOINTER_TO_INT (data));
}

void audgui_confirm_playlist_delete (gint playlist)
{
    GtkWidget * window, * vbox, * hbox, * label, * button;
    gchar * message;

    if (aud_cfg->no_confirm_playlist_delete)
    {
        aud_playlist_delete (playlist);
        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) window,
     GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_resizable ((GtkWindow *) window, FALSE);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    widget_destroy_on_escape (window);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) window, vbox);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    gtk_box_pack_start ((GtkBox *) hbox, gtk_image_new_from_stock
     (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG), FALSE, FALSE, 0);

    message = g_strdup_printf (_("Are you sure you want to close %s?  If you "
     "do, any changes made since the playlist was exported will be lost."),
     aud_playlist_get_title (playlist));
    label = gtk_label_new (message);
    g_free (message);
    gtk_label_set_line_wrap ((GtkLabel *) label, TRUE);
    gtk_widget_set_size_request (label, 320, -1);
    gtk_box_pack_start ((GtkBox *) hbox, label, TRUE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    button = gtk_check_button_new_with_mnemonic (_("_Don't show this message "
     "again"));
    gtk_box_pack_start ((GtkBox *) hbox, button, FALSE, FALSE, 0);
    g_signal_connect ((GObject *) button, "toggled", (GCallback)
     check_button_toggled, & aud_cfg->no_confirm_playlist_delete);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    button = gtk_button_new_from_stock (GTK_STOCK_NO);
    gtk_box_pack_end ((GtkBox *) hbox, button, FALSE, FALSE, 0);
    g_signal_connect_swapped (button, "clicked", (GCallback)
     gtk_widget_destroy, window);

    button = gtk_button_new_from_stock (GTK_STOCK_YES);
    gtk_box_pack_end ((GtkBox *) hbox, button, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION (2, 18, 0)
    gtk_widget_set_can_default (button, TRUE);
#endif
    gtk_widget_grab_default (button);
    gtk_widget_grab_focus (button);
    g_signal_connect ((GObject *) button, "clicked", (GCallback)
     confirm_delete_cb, GINT_TO_POINTER (playlist));
    g_signal_connect_swapped ((GObject *) button, "clicked", (GCallback)
     gtk_widget_destroy, window);

    gtk_widget_show_all (window);
}

static GtkTreeIter
playlist_manager_populate ( GtkListStore * store )
{
    gint playlists, active, playlist;
    GtkTreeIter iter, active_iter;
    gboolean have_active_iter = FALSE;

    playlists = aud_playlist_count ();
    active = aud_playlist_get_active ();

    while (gtk_tree_model_get_iter_first ((GtkTreeModel *) store, & iter))
        gtk_list_store_remove (store, & iter);

    for (playlist = 0; playlist < playlists; playlist ++)
    {
        gint entriesnum = aud_playlist_entry_count (playlist);
        const gchar * pl_name = aud_playlist_get_title (playlist);

        gtk_list_store_append (store, & iter);

        gtk_list_store_set (store, & iter, PLLIST_COL_NAME, pl_name,
         PLLIST_COL_ENTRIESNUM, entriesnum, PLLIST_NUMBER, playlist,
                            PLLIST_TEXT_WEIGHT , playlist == active ?
                                PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL ,
                            -1 );
        if ( !have_active_iter && playlist == active )
        {
            active_iter = iter;
            have_active_iter = TRUE;
        }
    }

    if ( !have_active_iter )
        gtk_tree_model_get_iter_first( GTK_TREE_MODEL(store) , &active_iter );

    return active_iter;
}


static void
playlist_manager_cb_new ( gpointer listview )
{
    gint playlists = aud_playlist_count ();
    GtkTreeIter iter;
    GtkListStore *store;

    /* this ensures that playlist_manager_update() will
       not perform update, since we're already doing it here */
    DISABLE_MANAGER_UPDATE();

    aud_playlist_insert (playlists);

    store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(listview) );
    gtk_list_store_append( store , &iter );
    gtk_list_store_set (store, & iter, PLLIST_COL_NAME, aud_playlist_get_title
     (playlists), PLLIST_COL_ENTRIESNUM, 0, PLLIST_NUMBER, playlists,
                        PLLIST_TEXT_WEIGHT , PANGO_WEIGHT_NORMAL ,
                        -1 );

    ENABLE_MANAGER_UPDATE();

    return;
}

static gint get_selected_row (GtkWidget * list)
{
    gint row = -1;
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *)
     list);
    GtkTreeModel * model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, & model, & iter))
    {
        GtkTreePath * path = gtk_tree_model_get_path (model, & iter);

        row = gtk_tree_path_get_indices (path)[0];
        gtk_tree_path_free (path);
    }

    return row;
}

static void set_selected_row (GtkWidget * list, gint row)
{
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *)
     list);
    GtkTreePath * path = gtk_tree_path_new_from_indices (row, -1);

    gtk_tree_selection_select_path (selection, path);
    gtk_tree_path_free (path);
}

static void reorder (GtkWidget * list, gint offset)
{
    gint from = get_selected_row (list);
    gint to;

    if (from == -1)
        return;

    to = from + GPOINTER_TO_INT (offset);

    if (to < 0 || to >= aud_playlist_count ())
        return;

    aud_playlist_reorder (from, to, 1);
    set_selected_row (list, to);
}

static void up_cb (GtkButton * button, GtkWidget * list)
{
    reorder (list, -1);
}

static void down_cb (GtkButton * button, GtkWidget * list)
{
    reorder (list, 1);
}

static void
playlist_manager_cb_del ( gpointer listview )
{
    GtkTreeSelection *listsel = gtk_tree_view_get_selection( GTK_TREE_VIEW(listview) );
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint active;
    gboolean was_active;

    if ( gtk_tree_selection_get_selected( listsel , &store , &iter ) == TRUE )
    {
        gint playlist;

        gtk_tree_model_get (store, & iter, PLLIST_NUMBER, & playlist, -1);

        active = aud_playlist_get_active();
        was_active = ( playlist == active );

        if ( gtk_tree_model_iter_n_children( store , NULL ) < 2 )
        {
            /* let playlist_manager_update() handle the deletion of the last playlist */
            audgui_confirm_playlist_delete (playlist);
        }
        else
        {
            gtk_list_store_remove( (GtkListStore*)store , &iter );
            /* this ensures that playlist_manager_update() will
               not perform update, since we're already doing it here */
            DISABLE_MANAGER_UPDATE();
            audgui_confirm_playlist_delete (playlist);
            ENABLE_MANAGER_UPDATE();
        }

        if ( was_active && gtk_tree_model_get_iter_first( store , &iter ) )
        {
            /* update bolded playlist */
            active = aud_playlist_get_active();
            do {
                gtk_tree_model_get (store, & iter, PLLIST_NUMBER, & playlist, -1);
                gtk_list_store_set( GTK_LIST_STORE(store) , &iter ,
                        PLLIST_TEXT_WEIGHT , playlist == active ?
                            PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL ,
                       -1 );
            } while ( gtk_tree_model_iter_next( store , &iter ) );
        }
    }

    return;
}


static void
playlist_manager_cb_lv_dclick ( GtkTreeView * listview , GtkTreePath * path ,
                                GtkTreeViewColumn * col , gpointer userdata )
{
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint playlist, active;

    store = gtk_tree_view_get_model( GTK_TREE_VIEW(listview) );
    if ( gtk_tree_model_get_iter( store , &iter , path ) == TRUE )
    {
        gtk_tree_model_get (store, & iter, PLLIST_NUMBER, & playlist, -1);
        DISABLE_MANAGER_UPDATE();
        aud_playlist_set_active (playlist);
        ENABLE_MANAGER_UPDATE();
    }

    if ( gtk_tree_model_get_iter_first( store , &iter ) )
    {
        /* update bolded playlist */
        active = aud_playlist_get_active();
        do {
            gtk_tree_model_get (store, & iter, PLLIST_NUMBER, & playlist, -1);
            gtk_list_store_set( GTK_LIST_STORE(store) , &iter ,
                    PLLIST_TEXT_WEIGHT , playlist == active ?
                        PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL ,
                   -1 );
        } while ( gtk_tree_model_iter_next( store , &iter ) );
    }

    return;
}


static void
playlist_manager_cb_lv_pmenu_rename ( GtkMenuItem *menuitem , gpointer lv )
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
        gtk_tree_view_set_cursor_on_cell( GTK_TREE_VIEW(lv) , path ,
                                          gtk_tree_view_get_column( GTK_TREE_VIEW(lv) , PLLIST_COL_NAME ) , rndrname , TRUE );
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
    {
        gint playlist;

        gtk_tree_model_get (store, & iter, PLLIST_NUMBER, & playlist, -1);
        DISABLE_MANAGER_UPDATE();
        aud_playlist_set_title (playlist, new_text);
        ENABLE_MANAGER_UPDATE();
        gtk_list_store_set( GTK_LIST_STORE(store), &iter, PLLIST_COL_NAME , new_text , -1 );
    }
    /* set the renderer uneditable again */
    g_object_set( G_OBJECT(cell) , "editable" , FALSE , NULL );
}


static gboolean
playlist_manager_cb_lv_btpress ( GtkWidget *lv , GdkEventButton *event )
{
    if (( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ))
    {
        GtkWidget *pmenu = (GtkWidget*)g_object_get_data( G_OBJECT(lv) , "menu" );
        gtk_menu_popup( GTK_MENU(pmenu) , NULL , NULL , NULL , NULL ,
                        (event != NULL) ? event->button : 0,
                        event->time);
        return TRUE;
    }

    return FALSE;
}


static gboolean
playlist_manager_cb_keypress ( GtkWidget *win , GdkEventKey *event )
{
    switch (event->keyval)
    {
        case GDK_Escape:
            gtk_widget_destroy( playman_win );
            return TRUE;
        default:
            return FALSE;
    }
}

void playlist_manager_catch_changes (void) {
   aud_hook_associate ("playlist update", (HookFunction) audgui_playlist_manager_update,
    0);
}

void playlist_manager_uncatch_changes (void) {
   aud_hook_dissociate ("playlist update",
    (HookFunction) audgui_playlist_manager_update);
}

void
audgui_playlist_manager_ui_show (GtkWidget *mainwin)
{
    GtkWidget *playman_vbox;
    GtkWidget *playman_pl_lv, *playman_pl_lv_frame, *playman_pl_lv_sw;
    GtkCellRenderer *playman_pl_lv_textrndr_name, *playman_pl_lv_textrndr_entriesnum;
    GtkTreeViewColumn *playman_pl_lv_col_name, *playman_pl_lv_col_entriesnum;
    GtkListStore *pl_store;
    GtkWidget *playman_pl_lv_pmenu, *playman_pl_lv_pmenu_rename;
    GtkWidget *playman_bbar_hbbox;
    GtkWidget *playman_bbar_bt_new, *playman_bbar_bt_del, *playman_bbar_bt_close;
    GtkWidget * up_button, * down_button;
    GdkGeometry playman_win_hints;
    GtkTreeIter active_iter;
    GtkTreePath *active_path;

    if ( playman_win != NULL )
    {
        gtk_window_present( GTK_WINDOW(playman_win) );
        return;
    }

    playman_win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint( GTK_WINDOW(playman_win), GDK_WINDOW_TYPE_HINT_DIALOG );
    gtk_window_set_transient_for( GTK_WINDOW(playman_win) , GTK_WINDOW(mainwin) );
    gtk_window_set_position( GTK_WINDOW(playman_win), GTK_WIN_POS_CENTER );
    gtk_window_set_title( GTK_WINDOW(playman_win), _("Playlist Manager") );
    gtk_container_set_border_width( GTK_CONTAINER(playman_win), 10 );
    g_signal_connect( G_OBJECT(playman_win) , "destroy" ,
                      G_CALLBACK(gtk_widget_destroyed) , &playman_win );
    g_signal_connect( G_OBJECT(playman_win) , "key-press-event" ,
                      G_CALLBACK(playlist_manager_cb_keypress) , NULL );
    playman_win_hints.min_width = 400;
    playman_win_hints.min_height = 250;
    gtk_window_set_geometry_hints( GTK_WINDOW(playman_win) , GTK_WIDGET(playman_win) ,
                                   &playman_win_hints , GDK_HINT_MIN_SIZE );

    playman_vbox = gtk_vbox_new( FALSE , 10 );
    gtk_container_add( GTK_CONTAINER(playman_win) , playman_vbox );

    /* current liststore model
       ----------------------------------------------
       G_TYPE_STRING -> playlist name
       G_TYPE_UINT -> number of entries in playlist
       G_TYPE_INT -> playlist number (gint)
       PANGO_TYPE_WEIGHT -> font weight
       ----------------------------------------------
       */
    pl_store = gtk_list_store_new (PLLIST_NUMCOLS, G_TYPE_STRING, G_TYPE_UINT,
     G_TYPE_INT, PANGO_TYPE_WEIGHT);
    active_iter = playlist_manager_populate( pl_store );

    playman_pl_lv_frame = gtk_frame_new( NULL );
    playman_pl_lv = gtk_tree_view_new_with_model( GTK_TREE_MODEL(pl_store) );

    g_object_set_data( G_OBJECT(playman_win) , "lv" , playman_pl_lv );
    g_object_set_data( G_OBJECT(playman_pl_lv) , "opt1" , GINT_TO_POINTER(0) );
    playman_pl_lv_textrndr_entriesnum = gtk_cell_renderer_text_new(); /* uneditable */
    playman_pl_lv_textrndr_name = gtk_cell_renderer_text_new(); /* can become editable */
    g_object_set( G_OBJECT(playman_pl_lv_textrndr_entriesnum) , "weight-set" , TRUE , NULL );
    g_object_set( G_OBJECT(playman_pl_lv_textrndr_name) , "weight-set" , TRUE , NULL );
    g_signal_connect( G_OBJECT(playman_pl_lv_textrndr_name) , "edited" ,
                      G_CALLBACK(playlist_manager_cb_lv_name_edited) , playman_pl_lv );
    g_object_set_data( G_OBJECT(playman_pl_lv) , "rn" , playman_pl_lv_textrndr_name );
    playman_pl_lv_col_name = gtk_tree_view_column_new_with_attributes(
            _("Playlist") , playman_pl_lv_textrndr_name ,
            "text" , PLLIST_COL_NAME ,
            "weight", PLLIST_TEXT_WEIGHT ,
            NULL );
    gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_name) , TRUE );
    gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_name );
    gtk_tree_view_column_set_sort_column_id( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_name), PLLIST_COL_NAME );

    playman_pl_lv_col_entriesnum = gtk_tree_view_column_new_with_attributes(
            _("Entries") , playman_pl_lv_textrndr_entriesnum ,
            "text" , PLLIST_COL_ENTRIESNUM ,
            "weight", PLLIST_TEXT_WEIGHT ,
            NULL );
    gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_entriesnum) , FALSE );
    gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_entriesnum );
    playman_pl_lv_sw = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(playman_pl_lv_sw) ,
                                    GTK_POLICY_NEVER , GTK_POLICY_ALWAYS );
    gtk_container_add( GTK_CONTAINER(playman_pl_lv_sw) , playman_pl_lv );
    gtk_container_add( GTK_CONTAINER(playman_pl_lv_frame) , playman_pl_lv_sw );
    gtk_box_pack_start( GTK_BOX(playman_vbox) , playman_pl_lv_frame , TRUE , TRUE , 0 );

    /* listview popup menu */
    playman_pl_lv_pmenu = gtk_menu_new();
    playman_pl_lv_pmenu_rename = gtk_menu_item_new_with_mnemonic( _( "_Rename" ) );
    g_signal_connect( G_OBJECT(playman_pl_lv_pmenu_rename) , "activate" ,
                      G_CALLBACK(playlist_manager_cb_lv_pmenu_rename) , playman_pl_lv );
    gtk_menu_shell_append( GTK_MENU_SHELL(playman_pl_lv_pmenu) , playman_pl_lv_pmenu_rename );
    gtk_widget_show_all( playman_pl_lv_pmenu );
    g_object_set_data( G_OBJECT(playman_pl_lv) , "menu" , playman_pl_lv_pmenu );
    g_signal_connect_swapped( G_OBJECT(playman_win) , "destroy" ,
                              G_CALLBACK(gtk_widget_destroy) , playman_pl_lv_pmenu );

    /* button bar */
    playman_bbar_hbbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout( GTK_BUTTON_BOX(playman_bbar_hbbox) , GTK_BUTTONBOX_END );
    gtk_box_set_spacing(GTK_BOX(playman_bbar_hbbox), 5);

    playman_bbar_bt_close = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
    playman_bbar_bt_del = gtk_button_new_from_stock( GTK_STOCK_DELETE );
    playman_bbar_bt_new = gtk_button_new_from_stock( GTK_STOCK_NEW );
    up_button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
    down_button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);

    gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_close );
    gtk_container_add ((GtkContainer *) playman_bbar_hbbox, up_button);
    gtk_container_add ((GtkContainer *) playman_bbar_hbbox, down_button);
    gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_del );
    gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_new );
    gtk_button_box_set_child_secondary( GTK_BUTTON_BOX(playman_bbar_hbbox) ,
                                        playman_bbar_bt_close , TRUE );

    gtk_box_pack_start( GTK_BOX(playman_vbox) , playman_bbar_hbbox , FALSE , FALSE , 0 );

    g_signal_connect( G_OBJECT(playman_pl_lv) , "button-press-event" ,
                      G_CALLBACK(playlist_manager_cb_lv_btpress) , NULL );
    g_signal_connect( G_OBJECT(playman_pl_lv) , "row-activated" ,
                      G_CALLBACK(playlist_manager_cb_lv_dclick) , NULL );

    g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_new) , "clicked" ,
                              G_CALLBACK(playlist_manager_cb_new) , playman_pl_lv );
    g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_del) , "clicked" ,
                              G_CALLBACK(playlist_manager_cb_del) , playman_pl_lv );
    g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_close) , "clicked" ,
                              G_CALLBACK(gtk_widget_destroy) , playman_win );
    g_signal_connect ((GObject *) up_button, "clicked", (GCallback) up_cb,
     playman_pl_lv);
    g_signal_connect ((GObject *) down_button, "clicked", (GCallback) down_cb,
     playman_pl_lv);

    /* have active playlist selected and scrolled to */
    active_path = gtk_tree_model_get_path( GTK_TREE_MODEL(pl_store) ,
            &active_iter );
    gtk_tree_view_set_cursor( GTK_TREE_VIEW(playman_pl_lv) ,
            active_path , NULL , FALSE );
    gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW(playman_pl_lv) ,
            active_path , NULL , TRUE , 0.5 , 0.0 );
    gtk_tree_path_free( active_path );

    g_object_unref( pl_store );

    playlist_manager_catch_changes ();
    g_signal_connect (G_OBJECT (playman_win), "destroy",
     G_CALLBACK (playlist_manager_uncatch_changes), 0);

    gtk_widget_show_all( playman_win );
}


void
audgui_playlist_manager_update ( void )
{
    /* this function is called whenever there is a change in playlist, such as
       playlist created/deleted or entry added/deleted in a playlist; if the playlist
       manager is active, it should be updated to keep consistency of information */

    if ( playman_win != NULL )
    {
        GtkWidget *lv = (GtkWidget*)g_object_get_data( G_OBJECT(playman_win) , "lv" );
        if ( GPOINTER_TO_INT(g_object_get_data(G_OBJECT(lv),"opt1")) == 0 )
        {
            gint row = get_selected_row (lv);
            GtkListStore *store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(lv) );
            playlist_manager_populate( store );

            if (row >= aud_playlist_count ())
                row = aud_playlist_count () - 1;

            if (row != -1)
                set_selected_row (lv, row);
        }
    }
}

void audgui_playlist_manager_destroy(void)
{
    if (playman_win)
        gtk_widget_destroy(playman_win);
}
