/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "playlist_manager.h"
#include "ui_playlist.h"
#include "playlist.h"
#include "mainwin.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>


/* TODO: we're accessing playlist_ stuff directly here, and storing a pointer to
     each Playlist. Data consistency should be always checked, cause playlists could
     be deleted by user without using the playlist manager, while the latter is active */


enum
{
  PLLIST_COL_NAME = 0,
  PLLIST_COL_ENTRIESNUM,
  PLLIST_COL_PLPOINTER,
  PLLIST_NUMCOLS
};


static void
playlist_manager_populate ( GtkListStore * store )
{
  GList *playlists = NULL;
  GtkTreeIter iter;

  playlists = playlist_get_playlists();
  while ( playlists != NULL )
  {
    GList *entries = NULL;
    gint entriesnum = 0;
    gchar *pl_name = NULL;
    Playlist *playlist = (Playlist*)playlists->data;

    PLAYLIST_LOCK(playlist->mutex);
    /* for each playlist, pick name and number of entries */
    pl_name = (gchar*)playlist_get_current_name( playlist );
    for (entries = playlist->entries; entries; entries = g_list_next(entries))
      entriesnum++;
    PLAYLIST_UNLOCK(playlist->mutex);

    gtk_list_store_append( store , &iter );
    gtk_list_store_set( store, &iter,
                        PLLIST_COL_NAME , pl_name ,
                        PLLIST_COL_ENTRIESNUM , entriesnum ,
                        PLLIST_COL_PLPOINTER , playlist , -1 );
    playlists = g_list_next(playlists);
  }
  return;
}


static void
playlist_manager_cb_new ( gpointer listview )
{
  GList *playlists = NULL;
  Playlist *newpl = NULL;
  GtkTreeIter iter;
  GtkListStore *store;
  gchar *pl_name = NULL;

  newpl = playlist_new();
  playlists = playlist_get_playlists();
  playlist_add_playlist( newpl );
  pl_name = (gchar*)playlist_get_current_name( newpl );

  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(listview) );
  gtk_list_store_append( store , &iter );
  gtk_list_store_set( store, &iter,
                      PLLIST_COL_NAME , pl_name ,
                      PLLIST_COL_ENTRIESNUM , 0 ,
                      PLLIST_COL_PLPOINTER , newpl , -1 );
  return;
}


static void
playlist_manager_cb_del ( gpointer listview )
{
  GtkTreeSelection *listsel = gtk_tree_view_get_selection( GTK_TREE_VIEW(listview) );
  GtkTreeModel *store;
  GtkTreeIter iter;

  if ( gtk_tree_selection_get_selected( listsel , &store , &iter ) == TRUE )
  {
    Playlist *playlist = NULL;
    gtk_tree_model_get( store, &iter, PLLIST_COL_PLPOINTER , &playlist , -1 );

    if ( gtk_tree_model_iter_n_children( store , NULL ) < 2 )
      return; /* do not delete the last playlist available */

    gtk_list_store_remove( (GtkListStore*)store , &iter );

    /* if the playlist removed is the active one, switch to the next */
    if ( playlist == playlist_get_active() )
      playlist_select_next();

    /* TODO: check that playlist has not been freed already!! */
    playlist_remove_playlist( playlist );
  }

  return;
}


void
playlist_manager_cb_lv_dclick ( GtkTreeView * lv , GtkTreePath * path ,
                                GtkTreeViewColumn * col , gpointer userdata )
{
  GtkTreeModel *store;
  GtkTreeIter iter;

  store = gtk_tree_view_get_model( GTK_TREE_VIEW(lv) );
  if ( gtk_tree_model_get_iter( store , &iter , path ) == TRUE )
  {
    Playlist *playlist = NULL;
    gtk_tree_model_get( store , &iter , PLLIST_COL_PLPOINTER , &playlist , -1 );
    /* TODO: check that playlist has not been freed already!! */
    playlist_select_playlist( playlist );
  }

  return;
}


void
playlist_manager_ui_show ( void )
{
  static GtkWidget *playman_win = NULL;
  GtkWidget *playman_vbox;
  GtkWidget *playman_pl_lv, *playman_pl_lv_frame, *playman_pl_lv_sw;
  GtkCellRenderer *playman_pl_lv_textrndr;
  GtkTreeViewColumn *playman_pl_lv_col_name, *playman_pl_lv_col_entriesnum;
  GtkListStore *pl_store;
  GtkWidget *playman_bbar_hbbox;
  GtkWidget *playman_bbar_bt_new, *playman_bbar_bt_del, *playman_bbar_bt_close;
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
  gtk_container_set_border_width( GTK_CONTAINER(playman_win), 10 );
  g_signal_connect( G_OBJECT(playman_win) , "destroy" ,
                    G_CALLBACK(gtk_widget_destroyed) , &playman_win );
  playman_win_hints.min_width = 400;
  playman_win_hints.min_height = 250;
  gtk_window_set_geometry_hints( GTK_WINDOW(playman_win) , GTK_WIDGET(playman_win) ,
                                 &playman_win_hints , GDK_HINT_MIN_SIZE );

  playman_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_add( GTK_CONTAINER(playman_win) , playman_vbox );

  /* current liststore model
     ----------------------------------------------
     G_TYPE_STRING -> playlist name
     G_TYPE_UINT -> number of entries in playlist
     G_TYPE_POINTER -> playlist pointer (Playlist*)
     ----------------------------------------------
  */
  pl_store = gtk_list_store_new( PLLIST_NUMCOLS , G_TYPE_STRING , G_TYPE_UINT , G_TYPE_POINTER );
  playlist_manager_populate( pl_store );

  playman_pl_lv_frame = gtk_frame_new( NULL );
  playman_pl_lv = gtk_tree_view_new_with_model( GTK_TREE_MODEL(pl_store) );
  g_object_unref( pl_store );
  playman_pl_lv_textrndr = gtk_cell_renderer_text_new();
  playman_pl_lv_col_name = gtk_tree_view_column_new_with_attributes(
    _("Playlist") , playman_pl_lv_textrndr , "text" , PLLIST_COL_NAME , NULL );
  gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_name) , TRUE );
  gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_name );
  playman_pl_lv_col_entriesnum = gtk_tree_view_column_new_with_attributes(
    _("Entries") , playman_pl_lv_textrndr , "text" , PLLIST_COL_ENTRIESNUM , NULL );
  gtk_tree_view_column_set_expand( GTK_TREE_VIEW_COLUMN(playman_pl_lv_col_entriesnum) , FALSE );
  gtk_tree_view_append_column( GTK_TREE_VIEW(playman_pl_lv), playman_pl_lv_col_entriesnum );
  playman_pl_lv_sw = gtk_scrolled_window_new( NULL , NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(playman_pl_lv_sw) ,
                                  GTK_POLICY_NEVER , GTK_POLICY_ALWAYS );
  gtk_container_add( GTK_CONTAINER(playman_pl_lv_sw) , playman_pl_lv );
  gtk_container_add( GTK_CONTAINER(playman_pl_lv_frame) , playman_pl_lv_sw );
  gtk_box_pack_start( GTK_BOX(playman_vbox) , playman_pl_lv_frame , TRUE , TRUE , 0 );

  gtk_box_pack_start( GTK_BOX(playman_vbox) , gtk_hseparator_new() , FALSE , FALSE , 4 );

  /* button bar */
  playman_bbar_hbbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(playman_bbar_hbbox) , GTK_BUTTONBOX_START );
  playman_bbar_bt_new = gtk_button_new_from_stock( GTK_STOCK_NEW );
  playman_bbar_bt_del = gtk_button_new_from_stock( GTK_STOCK_DELETE );
  playman_bbar_bt_close = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
  gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_new );
  gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_del );
  gtk_container_add( GTK_CONTAINER(playman_bbar_hbbox) , playman_bbar_bt_close );
  gtk_button_box_set_child_secondary( GTK_BUTTON_BOX(playman_bbar_hbbox) ,
                                      playman_bbar_bt_close , TRUE );
  gtk_box_pack_start( GTK_BOX(playman_vbox) , playman_bbar_hbbox , FALSE , FALSE , 0 );

  g_signal_connect( G_OBJECT(playman_pl_lv) , "row-activated" ,
                    G_CALLBACK(playlist_manager_cb_lv_dclick) , NULL );
  g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_new) , "clicked" ,
                            G_CALLBACK(playlist_manager_cb_new) , playman_pl_lv );
  g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_del) , "clicked" ,
                            G_CALLBACK(playlist_manager_cb_del) , playman_pl_lv );
  g_signal_connect_swapped( G_OBJECT(playman_bbar_bt_close) , "clicked" ,
                            G_CALLBACK(gtk_widget_destroy) , playman_win );

  gtk_widget_show_all( playman_win );
}
