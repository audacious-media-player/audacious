/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/


#include "i_configure-ap.h"
#include "amidi-plug-icon.xpm"


enum
{
  LISTBACKEND_NAME_COLUMN = 0,
  LISTBACKEND_LONGNAME_COLUMN,
  LISTBACKEND_DESC_COLUMN,
  LISTBACKEND_FILENAME_COLUMN,
  LISTBACKEND_PPOS_COLUMN,
  LISTBACKEND_N_COLUMNS
};


void i_configure_ev_backendlv_info( gpointer backend_lv )
{
  GtkTreeModel * store;
  GtkTreeIter iter;
  GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(backend_lv) );
  /* get the selected item */
  if ( gtk_tree_selection_get_selected( sel , &store , &iter ) )
  {
    GtkWidget *bidialog;
    GdkGeometry bi_hints;
    GtkWidget *title_label, *title_frame;
    GtkWidget *filename_entry, *filename_frame;
    GtkWidget *description_label, *description_frame;
    GtkWidget *parent_window = gtk_widget_get_toplevel( backend_lv );
    gchar *longname_title, *longname, *filename, *description;
    gtk_tree_model_get( GTK_TREE_MODEL(store) , &iter ,
                        LISTBACKEND_LONGNAME_COLUMN , &longname ,
                        LISTBACKEND_DESC_COLUMN , &description ,
                        LISTBACKEND_FILENAME_COLUMN , &filename , -1 );
    bidialog = gtk_dialog_new_with_buttons( _("AMIDI-Plug - backend information") ,
                                            GTK_WINDOW(parent_window) ,
                                            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL ,
                                            GTK_STOCK_OK , GTK_RESPONSE_NONE , NULL );
    bi_hints.min_width = 360; bi_hints.min_height = -1;
    gtk_window_set_geometry_hints( GTK_WINDOW(bidialog) , GTK_WIDGET(bidialog) ,
                                   &bi_hints , GDK_HINT_MIN_SIZE );

    longname_title = g_markup_printf_escaped( "<span size=\"larger\" weight=\"bold\" >%s</span>" , longname );
    title_frame = gtk_frame_new( NULL );
    title_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(title_label) , longname_title );
    g_free( longname_title ); g_free( longname );
    gtk_container_add( GTK_CONTAINER(title_frame) , title_label );
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(bidialog)->vbox) , title_frame , FALSE , FALSE , 0 );

    filename_frame = gtk_frame_new( NULL );
    filename_entry = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY(filename_entry) , filename );
    gtk_entry_set_alignment( GTK_ENTRY(filename_entry) , 0.5 );
    gtk_editable_set_editable( GTK_EDITABLE(filename_entry) , FALSE );
    gtk_entry_set_has_frame( GTK_ENTRY(filename_entry) , FALSE );
    g_free( filename );
    gtk_container_add( GTK_CONTAINER(filename_frame) , filename_entry );
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(bidialog)->vbox) , filename_frame , FALSE , FALSE , 0 );

    description_frame = gtk_frame_new( NULL );
    description_label = gtk_label_new( description );
    gtk_misc_set_padding( GTK_MISC(description_label) , 4 , 4 );
    gtk_label_set_line_wrap( GTK_LABEL(description_label) , TRUE );
    g_free( description );
    gtk_container_add( GTK_CONTAINER(description_frame) , description_label );
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(bidialog)->vbox) , description_frame , TRUE , TRUE , 0 );

    gtk_widget_show_all( bidialog );
    gtk_window_set_focus( GTK_WINDOW(bidialog) , NULL );
    gtk_dialog_run( GTK_DIALOG(bidialog) );
    gtk_widget_destroy( bidialog );
  }
  return;
}


void i_configure_ev_backendlv_commit( gpointer backend_lv )
{
  GtkTreeModel * store;
  GtkTreeIter iter;
  GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(backend_lv) );
  /* get the selected item */
  if ( gtk_tree_selection_get_selected( sel , &store , &iter ) )
  {
    g_free( amidiplug_cfg_ap.ap_seq_backend ); /* free previous */
    /* update amidiplug_cfg_ap.ap_seq_backend */
    gtk_tree_model_get( GTK_TREE_MODEL(store) , &iter ,
                        LISTBACKEND_NAME_COLUMN , &amidiplug_cfg_ap.ap_seq_backend , -1 );
  }
  return;
}


void i_configure_ev_settings_commit( gpointer settings_vbox )
{
  GtkWidget * settings_precalc_checkbt = g_object_get_data( G_OBJECT(settings_vbox) ,
                                                            "ap_opts_length_precalc" );
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(settings_precalc_checkbt) ) )
    amidiplug_cfg_ap.ap_opts_length_precalc = 1;
  else
    amidiplug_cfg_ap.ap_opts_length_precalc = 0;
  return;
}


gint i_configure_backendlist_sortf( GtkTreeModel * model , GtkTreeIter * a ,
                                    GtkTreeIter * b , gpointer data )
{
  gint v_a = 0, v_b = 0;
  gtk_tree_model_get( model , a , LISTBACKEND_PPOS_COLUMN , &v_a , -1 );
  gtk_tree_model_get( model , b , LISTBACKEND_PPOS_COLUMN , &v_b , -1 );
  return (v_a - v_b);
}


void i_configure_gui_tab_ap( GtkWidget * ap_page_alignment ,
                               gpointer backend_list_p ,
                               gpointer commit_button )
{
  GtkWidget *ap_page_vbox;
  GtkWidget *title_widget;
  GtkWidget *content_vbox; /* this vbox will contain two items of equal space (50%/50%) */
  GtkWidget *settings_frame, *settings_vbox, *settings_precalc_checkbt;
  GtkWidget *backend_lv_frame, *backend_lv, *backend_lv_sw;
  GtkWidget *backend_lv_hbox, *backend_lv_vbbox, *backend_lv_infobt;
  GtkListStore *backend_store;
  GtkCellRenderer *backend_lv_text_rndr;
  GtkTreeViewColumn *backend_lv_name_col;
  GtkTreeIter backend_lv_iter_selected;
  GtkTreeSelection *backend_lv_sel;
  GtkTreeIter iter;
  GtkTooltips *tips;
  GSList * backend_list = backend_list_p;

  tips = gtk_tooltips_new();
  g_object_set_data_full( G_OBJECT(ap_page_alignment) , "tt" , tips , g_object_unref );

  ap_page_vbox = gtk_vbox_new( FALSE , 0 );

  title_widget = i_configure_gui_draw_title( _("AMIDI-PLUG PREFERENCES") );
  gtk_box_pack_start( GTK_BOX(ap_page_vbox) , title_widget , FALSE , FALSE , 2 );

  content_vbox = gtk_vbox_new( TRUE , 2 );

  backend_store = gtk_list_store_new( LISTBACKEND_N_COLUMNS , G_TYPE_STRING , G_TYPE_STRING ,
                                      G_TYPE_STRING , G_TYPE_STRING , G_TYPE_INT );
  gtk_tree_sortable_set_default_sort_func( GTK_TREE_SORTABLE(backend_store) ,
                                           i_configure_backendlist_sortf , NULL , NULL );
  gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE(backend_store) ,
                                        GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID ,
                                        GTK_SORT_ASCENDING );
  while ( backend_list != NULL )
  {
    amidiplug_sequencer_backend_name_t * mn = backend_list->data;
    gtk_list_store_append( backend_store , &iter );
    gtk_list_store_set ( backend_store, &iter,
                         LISTBACKEND_NAME_COLUMN , mn->name ,
                         LISTBACKEND_LONGNAME_COLUMN , mn->longname ,
                         LISTBACKEND_DESC_COLUMN , mn->desc ,
                         LISTBACKEND_FILENAME_COLUMN , mn->filename ,
                         LISTBACKEND_PPOS_COLUMN , mn->ppos , -1 );
    if ( !strcmp( mn->name , amidiplug_cfg_ap.ap_seq_backend ) )
      backend_lv_iter_selected = iter;
    backend_list = backend_list->next;
  }

  backend_lv_frame = gtk_frame_new( _("Backend selection") );
  backend_lv = gtk_tree_view_new_with_model( GTK_TREE_MODEL(backend_store) );
  g_object_unref( backend_store );
  backend_lv_text_rndr = gtk_cell_renderer_text_new();
  backend_lv_name_col = gtk_tree_view_column_new_with_attributes( _("Available backends") ,
                                                                  backend_lv_text_rndr ,
                                                                  "text" , 1 , NULL );
  gtk_tree_view_append_column( GTK_TREE_VIEW(backend_lv), backend_lv_name_col );

  backend_lv_sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(backend_lv) );
  gtk_tree_selection_set_mode( GTK_TREE_SELECTION(backend_lv_sel) , GTK_SELECTION_BROWSE );
  gtk_tree_selection_select_iter( GTK_TREE_SELECTION(backend_lv_sel) , &backend_lv_iter_selected );

  backend_lv_sw = gtk_scrolled_window_new( NULL , NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(backend_lv_sw) ,
                                  GTK_POLICY_NEVER , GTK_POLICY_ALWAYS );
  gtk_container_add( GTK_CONTAINER(backend_lv_sw) , backend_lv );
  g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                            G_CALLBACK(i_configure_ev_backendlv_commit) , backend_lv );

  backend_lv_hbox = gtk_hbox_new( FALSE , 0 );
  gtk_box_pack_start( GTK_BOX(backend_lv_hbox) , backend_lv_sw , TRUE , TRUE , 0 );
  backend_lv_vbbox = gtk_vbox_new( FALSE , 2 );
  gtk_box_pack_start( GTK_BOX(backend_lv_hbox) , backend_lv_vbbox , FALSE , FALSE , 3 );
  backend_lv_infobt = gtk_button_new();
  gtk_button_set_image( GTK_BUTTON(backend_lv_infobt) ,
                          gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO , GTK_ICON_SIZE_BUTTON ) );
  g_signal_connect_swapped( G_OBJECT(backend_lv_infobt) , "clicked" ,
                            G_CALLBACK(i_configure_ev_backendlv_info) , backend_lv );
  gtk_box_pack_start( GTK_BOX(backend_lv_vbbox) , backend_lv_infobt , FALSE , FALSE , 0 );
  gtk_container_add( GTK_CONTAINER(backend_lv_frame) , backend_lv_hbox );

  settings_frame = gtk_frame_new( _("Advanced settings") );
  settings_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_set_border_width( GTK_CONTAINER(settings_vbox), 4 );
  settings_precalc_checkbt = gtk_check_button_new_with_label( _("pre-calculate length of MIDI files in playlist") );
  if ( amidiplug_cfg_ap.ap_opts_length_precalc )
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(settings_precalc_checkbt) , TRUE );
  gtk_box_pack_start( GTK_BOX(settings_vbox) , settings_precalc_checkbt , FALSE , FALSE , 2 );
  gtk_container_add( GTK_CONTAINER(settings_frame) , settings_vbox );
  /* attach pointers of options to settings_vbox so we can handle all of them in a single callback */
  g_object_set_data( G_OBJECT(settings_vbox) , "ap_opts_length_precalc" , settings_precalc_checkbt );
  g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                            G_CALLBACK(i_configure_ev_settings_commit) , settings_vbox );

  gtk_box_pack_start( GTK_BOX(content_vbox) , backend_lv_frame , TRUE , TRUE , 0 );
  gtk_box_pack_start( GTK_BOX(content_vbox) , settings_frame , TRUE , TRUE , 0 );
  gtk_box_pack_start( GTK_BOX(ap_page_vbox) , content_vbox , TRUE , TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(ap_page_alignment) , ap_page_vbox );

  gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , backend_lv ,
                        _("* Backend selection *\n"
                        "AMIDI-Plug works with backends, in a modular fashion; here you should "
                        "select your backend; that is, the way MIDI events are going to be handled "
                        "and played.\nIf you have a hardware synthesizer on your audio card, and ALSA "
                        "supports it, you'll want to use the ALSA backend. It can also be "
                        "used with anything that provides an interface to the ALSA sequencer, including "
                        "software synths or external devices.\nIf you want to rely on a software "
                        "synthesizer and/or want to pipe audio into effect and output plugins of the "
                        "player you'll want to use the good FluidSynth backend.\nPress the info "
                        "button to read specific information about each backend.") , "" );
  gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , settings_precalc_checkbt ,
                        _("* Pre-calculate MIDI length *\n"
                        "If this option is enabled, AMIDI-Plug will calculate the MIDI file "
                        "length as soon as the player requests it, instead of doing that only "
                        "when the MIDI file is being played. In example, MIDI length "
                        "will be calculated straight after adding MIDI files in a playlist. "
                        "Disable this option if you want faster playlist loading (when a lot "
                        "of MIDI files are added), enable it to display more information "
                        "in the playlist straight after loading.") , "" );
}


void i_configure_gui_tablabel_ap( GtkWidget * ap_page_alignment ,
                                  gpointer backend_list_p ,
                                  gpointer commit_button )
{
  GtkWidget *pagelabel_vbox, *pagelabel_image, *pagelabel_label;
  GdkPixbuf *pagelabel_image_pix;
  pagelabel_vbox = gtk_vbox_new( FALSE , 1 );
  pagelabel_image_pix = gdk_pixbuf_new_from_xpm_data( (const gchar **)amidi_plug_icon_xpm );
  pagelabel_image = gtk_image_new_from_pixbuf( pagelabel_image_pix ); g_object_unref( pagelabel_image_pix );
  pagelabel_label = gtk_label_new( "" );
  gtk_label_set_markup( GTK_LABEL(pagelabel_label) , "<span size=\"smaller\">AMIDI\nPlug</span>" );
  gtk_label_set_justify( GTK_LABEL(pagelabel_label) , GTK_JUSTIFY_CENTER );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_image , FALSE , FALSE , 1 );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_label , FALSE , FALSE , 1 );
  gtk_container_add( GTK_CONTAINER(ap_page_alignment) , pagelabel_vbox );
  gtk_widget_show_all( ap_page_alignment );
  return;
}
