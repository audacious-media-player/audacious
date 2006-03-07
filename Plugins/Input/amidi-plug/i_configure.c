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
* 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/


#include "i_configure.h"

/* internals */
void i_configure_upd_portlist( void );
void i_configure_upd_mixercardlist( void );
gchar * i_configure_read_seq_ports_default( void );
gboolean i_configure_ev_checktoggle( GtkTreeModel * , GtkTreePath * , GtkTreeIter * , gpointer );
void i_configure_ev_changetoggle( GtkCellRendererToggle * , gchar * , gpointer );


void i_configure_gui( GSList * wports , GSList * scards )
{
  GtkWidget *configwin_vbox;
  GtkWidget *port_lv, *port_lv_sw, *port_frame;
  GtkWidget *mixer_card_cmb_evbox, *mixer_card_cmb, *mixer_vbox, *mixer_frame;
  GtkWidget *advanced_precalc_checkbt, *advanced_vbox, *advanced_frame;
  GtkWidget *hseparator, *hbuttonbox, *button_ok, *button_cancel;
  GtkListStore *port_liststore, *mixer_card_liststore;
  GtkTreeIter iter;
  GtkTreeSelection *port_lv_sel;
  GtkCellRenderer *port_lv_toggle_rndr, *port_lv_text_rndr, *mixer_card_cmb_text_rndr;
  GtkTreeViewColumn *port_lv_toggle_col, *port_lv_portnum_col;
  GtkTreeViewColumn *port_lv_clientname_col, *port_lv_portname_col;
  gchar **portstring_from_cfg = NULL;

  if ( amidiplug_gui_prefs.config_win )
    return;

  if ( strlen( amidiplug_cfg.seq_writable_ports ) > 0 )
    portstring_from_cfg = g_strsplit( amidiplug_cfg.seq_writable_ports , "," , 0 );

  amidiplug_gui_prefs.config_win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(amidiplug_gui_prefs.config_win), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_title( GTK_WINDOW(amidiplug_gui_prefs.config_win), "AMIDI-Plug - configuration" );
  gtk_container_set_border_width( GTK_CONTAINER(amidiplug_gui_prefs.config_win), 10 );
  g_signal_connect( G_OBJECT(amidiplug_gui_prefs.config_win) ,
                    "destroy" , G_CALLBACK(i_configure_ev_destroy) , NULL );

  configwin_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_add( GTK_CONTAINER(amidiplug_gui_prefs.config_win) , configwin_vbox );

  port_liststore = gtk_list_store_new( LISTPORT_N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING ,
                                       G_TYPE_STRING , G_TYPE_STRING , G_TYPE_POINTER );

  /* append ALSA MIDI ports */
  while ( wports != NULL )
  {
    gint i = 0;
    gboolean toggled = FALSE;
    data_bucket_t * portinfo = (data_bucket_t *)wports->data;
    GString * portstring = g_string_new("");
    G_STRING_PRINTF( portstring , "%i:%i" ,
                     portinfo->bint[0] , portinfo->bint[1] );
    gtk_list_store_append( port_liststore , &iter );

    /* in the existing configuration there may be previously selected ports */
    if ( portstring_from_cfg )
    {
      /* check if current row contains a port selected by user */
      for ( i = 0 ; portstring_from_cfg[i] != NULL ; i++ )
      {
        /* if it's one of the selected ports, toggle its checkbox */
        if ( !strcmp( portstring->str, portstring_from_cfg[i] ) )
          toggled = TRUE;
      }
    }

    gtk_list_store_set( port_liststore , &iter ,
                        LISTPORT_TOGGLE_COLUMN , toggled ,
                        LISTPORT_PORTNUM_COLUMN , portstring->str ,
                        LISTPORT_CLIENTNAME_COLUMN , portinfo->bcharp[0] ,
                        LISTPORT_PORTNAME_COLUMN , portinfo->bcharp[1] ,
                        LISTPORT_POINTER_COLUMN , portinfo , -1 );

    g_string_free( portstring , TRUE );
    /* on with next */
    wports = wports->next;
  }

  /* this string array is not needed anymore */
  g_strfreev( portstring_from_cfg );

  port_lv = gtk_tree_view_new_with_model( GTK_TREE_MODEL(port_liststore) );
  amidiplug_gui_prefs.port_treeview = port_lv;
  g_object_unref( port_liststore );
  port_lv_toggle_rndr = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE(port_lv_toggle_rndr) , FALSE );
  gtk_cell_renderer_toggle_set_active( GTK_CELL_RENDERER_TOGGLE(port_lv_toggle_rndr) , TRUE );
  g_signal_connect( port_lv_toggle_rndr , "toggled" ,
                    G_CALLBACK(i_configure_ev_changetoggle) , port_liststore );
  port_lv_text_rndr = gtk_cell_renderer_text_new();
  port_lv_toggle_col = gtk_tree_view_column_new_with_attributes( "", port_lv_toggle_rndr,
                                                                 "active", LISTPORT_TOGGLE_COLUMN, NULL );
  port_lv_portnum_col = gtk_tree_view_column_new_with_attributes( "Port", port_lv_text_rndr,
                                                                  "text", LISTPORT_PORTNUM_COLUMN, NULL );
  port_lv_clientname_col = gtk_tree_view_column_new_with_attributes( "Client name", port_lv_text_rndr,
                                                                     "text", LISTPORT_CLIENTNAME_COLUMN, NULL );
  port_lv_portname_col = gtk_tree_view_column_new_with_attributes( "Port name", port_lv_text_rndr,
                                                                   "text", LISTPORT_PORTNAME_COLUMN, NULL );
  gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_toggle_col );
  gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_portnum_col );
  gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_clientname_col );
  gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_portname_col );

  port_lv_sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(port_lv) );
  gtk_tree_selection_set_mode( GTK_TREE_SELECTION(port_lv_sel) , GTK_SELECTION_SINGLE );

  port_lv_sw = gtk_scrolled_window_new( NULL , NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(port_lv_sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER );
  port_frame = gtk_frame_new( "ALSA output ports" );

  gtk_container_add( GTK_CONTAINER(port_lv_sw) , port_lv );
  gtk_container_set_border_width( GTK_CONTAINER(port_lv_sw) , 5 );
  gtk_container_add( GTK_CONTAINER(port_frame) , port_lv_sw );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , port_frame , TRUE , TRUE , 2 );

  /**********************/
  /*** MIXER SETTINGS ***/
  mixer_card_liststore = gtk_list_store_new( LISTMIXER_N_COLUMNS , G_TYPE_STRING , G_TYPE_INT ,
                                             G_TYPE_INT , G_TYPE_STRING );
  mixer_card_cmb = gtk_combo_box_new_with_model( GTK_TREE_MODEL(mixer_card_liststore) );
  amidiplug_gui_prefs.mixercard_combo = mixer_card_cmb;
  /* fill models with sound cards and relative mixer controls */
  while ( scards != NULL )
  {
    data_bucket_t * cardinfo = (data_bucket_t *)scards->data;
    GString * desc_cardmix_string = g_string_new( "" );
    GSList * mixctl_list = cardinfo->bpointer[0];
    while ( mixctl_list != NULL )
    {
      data_bucket_t * mixctlinfo = (data_bucket_t *)mixctl_list->data;
      G_STRING_PRINTF( desc_cardmix_string , "%s - ctl: %s (ID %i)" , cardinfo->bcharp[0] ,
                       mixctlinfo->bcharp[0] , mixctlinfo->bint[0] );
      gtk_list_store_append( mixer_card_liststore , &iter );
      gtk_list_store_set( mixer_card_liststore , &iter ,
                          LISTMIXER_DESC_COLUMN , desc_cardmix_string->str ,
                          LISTMIXER_CARDID_COLUMN , cardinfo->bint[0] ,
                          LISTMIXER_MIXCTLID_COLUMN , mixctlinfo->bint[0] ,
                          LISTMIXER_MIXCTLNAME_COLUMN , mixctlinfo->bcharp[0] , -1 );
      /* check if this corresponds to the value previously selected by user */
      if ( ( cardinfo->bint[0] == amidiplug_cfg.mixer_card_id ) &&
           ( !strcasecmp( mixctlinfo->bcharp[0] , amidiplug_cfg.mixer_control_name ) ) &&
           ( mixctlinfo->bint[0] == amidiplug_cfg.mixer_control_id ) )
        gtk_combo_box_set_active_iter( GTK_COMBO_BOX(mixer_card_cmb) , &iter );
      mixctl_list = mixctl_list->next;
    }
    g_string_free( desc_cardmix_string , TRUE );
    /* on with next */
    scards = scards->next;
  }
  /* free the instance of liststore, we already have one in the combo box */
  g_object_unref( mixer_card_liststore );
  /* create renderer to display text in the mixer combo box */
  mixer_card_cmb_text_rndr = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(mixer_card_cmb) , mixer_card_cmb_text_rndr , TRUE );
  gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT(mixer_card_cmb) , mixer_card_cmb_text_rndr , "text" , 0 );

  /* the event box is needed to display a tooltip for the mixer combo box */
  mixer_card_cmb_evbox = gtk_event_box_new();
  gtk_container_add( GTK_CONTAINER(mixer_card_cmb_evbox) , mixer_card_cmb );
  mixer_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_set_border_width( GTK_CONTAINER(mixer_vbox) , 5 );
  gtk_box_pack_start( GTK_BOX(mixer_vbox) , mixer_card_cmb_evbox , TRUE , TRUE , 0 );

  mixer_frame = gtk_frame_new( "Mixer settings" );
  gtk_container_add( GTK_CONTAINER(mixer_frame) , mixer_vbox );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , mixer_frame , TRUE , TRUE , 2 );

  /*************************/
  /*** ADVANCED SETTINGS ***/
  advanced_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_set_border_width( GTK_CONTAINER(advanced_vbox) , 5 );
  
  advanced_precalc_checkbt = gtk_check_button_new_with_label( "pre-calculate length of MIDI files in playlist" );
  amidiplug_gui_prefs.precalc_checkbt = advanced_precalc_checkbt;
  if ( amidiplug_cfg.length_precalc_enable )
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(advanced_precalc_checkbt) , TRUE );
  gtk_box_pack_start( GTK_BOX(advanced_vbox) , advanced_precalc_checkbt , TRUE , TRUE , 0 );

  advanced_frame = gtk_frame_new( "Advanced settings" );
  gtk_container_add( GTK_CONTAINER(advanced_frame) , advanced_vbox );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , advanced_frame , TRUE , TRUE , 2 );


  /* horizontal separator and buttons */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , hseparator , FALSE , FALSE , 4 );
  hbuttonbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(hbuttonbox) , GTK_BUTTONBOX_END );
  button_cancel = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
  g_signal_connect( G_OBJECT(button_cancel) , "clicked" , G_CALLBACK(i_configure_ev_bcancel) , NULL );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_cancel );
  button_ok = gtk_button_new_from_stock( GTK_STOCK_OK );
  g_signal_connect( G_OBJECT(button_ok) , "clicked" , G_CALLBACK(i_configure_ev_bok) , NULL );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

  /* tooltips */
  amidiplug_gui_prefs.config_tips = gtk_tooltips_new();
  gtk_tooltips_set_tip( GTK_TOOLTIPS(amidiplug_gui_prefs.config_tips) , port_lv ,
                        "* Select ALSA output ports *\n"
                        "MIDI events will be sent to the ports selected here. At least one "
                        "port must be selected in order to play MIDI with AMIDI-Plug. Unless "
                        "you know what you're doing, you'll probably want to use the "
                        "wavetable synthesizer ports." , "" );
  gtk_tooltips_set_tip( GTK_TOOLTIPS(amidiplug_gui_prefs.config_tips) , mixer_card_cmb_evbox ,
                        "* Select ALSA mixer control *\n"
                        "AMIDI-Plug outputs directly through ALSA, it doesn't use effect "
                        "and ouput plugins from the player. While playing with AMIDI-Plug, "
                        "the player volume will manipulate the control you select here. "
                        "Unless you know what you're doing, you'll probably want to select "
                        "the Synth control here." , "" );

  gtk_widget_show_all( amidiplug_gui_prefs.config_win );
}


void i_configure_upd_portlist( void )
{
  GtkTreeModel * liststore;
  GString *wp = g_string_new( "" );
  /* get the model of the port list control */
  liststore = gtk_tree_view_get_model( GTK_TREE_VIEW(amidiplug_gui_prefs.port_treeview) );
  /* after the following foreach, wp contains a comma-separated list of selected ports */
  gtk_tree_model_foreach( liststore , i_configure_ev_checktoggle , wp );
  /* free previous */
  g_free( amidiplug_cfg.seq_writable_ports );
  /* update point amidiplug_cfg.seq_writable_ports
     so it points to the new list of ports */
  amidiplug_cfg.seq_writable_ports = g_strdup( wp->str );
  /* not needed anymore */
  g_string_free( wp , TRUE );
  return;
}


void i_configure_upd_mixercardlist( void )
{
  GtkTreeModel * liststore;
  GtkTreeIter iter;
  /* get the model of the card-mixer list control */
  liststore = gtk_combo_box_get_model( GTK_COMBO_BOX(amidiplug_gui_prefs.mixercard_combo) );
  /* get the selected item */
  if ( gtk_combo_box_get_active_iter( GTK_COMBO_BOX(amidiplug_gui_prefs.mixercard_combo) , &iter ) )
  {
    /* free previous */
    g_free( amidiplug_cfg.mixer_control_name );
    /* update amidiplug_cfg.mixer_card_id and amidiplug_cfg.mixer_control_name */
    gtk_tree_model_get( GTK_TREE_MODEL(liststore) , &iter ,
                        LISTMIXER_CARDID_COLUMN , &amidiplug_cfg.mixer_card_id ,
                        LISTMIXER_MIXCTLID_COLUMN , &amidiplug_cfg.mixer_control_id ,
                        LISTMIXER_MIXCTLNAME_COLUMN , &amidiplug_cfg.mixer_control_name , -1 );
  }
  return;
}


gboolean i_configure_ev_checktoggle( GtkTreeModel * model , GtkTreePath * path ,
                                     GtkTreeIter * iter , gpointer wpp )
{
  gboolean toggled = FALSE;
  gchar * portstring;
  GString * wps = wpp;
  gtk_tree_model_get ( model , iter ,
                       LISTPORT_TOGGLE_COLUMN , &toggled ,
                       LISTPORT_PORTNUM_COLUMN , &portstring , -1 );
  /* check if the row points to an enabled port */
  if ( toggled )
  {
    /* if this is not the first port added to wp, use comma as separator */
    if ( wps->str[0] != '\0' ) g_string_append_c( wps , ',' );
    g_string_append( wps , portstring );
  }
  g_free( portstring );
  return FALSE;
}


void i_configure_ev_changetoggle( GtkCellRendererToggle * rdtoggle ,
                                  gchar * path_str , gpointer data )
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string( path_str );
  gboolean toggled;

  gtk_tree_model_get_iter( model , &iter , path );
  gtk_tree_model_get( model , &iter , LISTPORT_TOGGLE_COLUMN , &toggled , -1);

  toggled ^= 1;
  gtk_list_store_set( GTK_LIST_STORE(model), &iter , LISTPORT_TOGGLE_COLUMN , toggled , -1);

  gtk_tree_path_free (path);
}


void i_configure_ev_destroy( void )
{
  g_object_unref( amidiplug_gui_prefs.config_tips );
  amidiplug_gui_prefs.config_tips = NULL;
  amidiplug_gui_prefs.config_win = NULL;
}


void i_configure_ev_bcancel( void )
{
  gtk_widget_destroy(amidiplug_gui_prefs.config_win);
}


void i_configure_ev_bok( void )
{
  /* update amidiplug_cfg.seq_writable_ports
     using the selected values from the port list */
  i_configure_upd_portlist();

  /* update amidiplug_cfg.mixer_card_id and amidiplug_cfg.mixer_control_name
     using the selected values from the card-mixer combo list */
  i_configure_upd_mixercardlist();
  
  /* update amidiplug_cfg.length_precalc_enable using
     the check control in the advanced settings frame */
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(amidiplug_gui_prefs.precalc_checkbt) ) )
    amidiplug_cfg.length_precalc_enable = 1;
  else
    amidiplug_cfg.length_precalc_enable = 0;

  /* save settings */
  i_configure_cfg_save();

  gtk_widget_destroy(amidiplug_gui_prefs.config_win);
}


gchar * i_configure_read_seq_ports_default( void )
{
  FILE * fp = NULL;
  /* first try, get seq ports from proc on card0 */
  fp = fopen( "/proc/asound/card0/wavetableD1" , "rb" );
  if ( fp )
  {
    gchar buffer[100];
    while ( !feof( fp ) )
    {
      fgets( buffer , 100 , fp );
      if (( strlen( buffer ) > 11 ) && ( !strncasecmp( buffer , "addresses: " , 11 ) ))
      {
        /* change spaces between ports (65:0 65:1 65:2 ...)
           into commas (65:0,65:1,65:2,...) */
        g_strdelimit( &buffer[11] , " " , ',' );
        /* remove lf and cr from the end of the string */
        g_strdelimit( &buffer[11] , "\r\n" , '\0' );
        /* ready to go */
        DEBUGMSG( "init, default values for seq ports detected: %s\n" , &buffer[11] );
        fclose( fp );
        return g_strdup( &buffer[11] );
      }
    }
    fclose( fp );
  }

  /* second option: do not set ports at all, let the user
     select the right ones in the nice preferences dialog :) */
  return g_strdup( "" );
}


void i_configure_cfg_read( void )
{
  ConfigFile *cfgfile;
  cfgfile = xmms_cfg_open_default_file();

  if (!cfgfile)
  {
    /* use defaults */
    amidiplug_cfg.seq_writable_ports = i_configure_read_seq_ports_default();
    amidiplug_cfg.mixer_card_id = 0;
    amidiplug_cfg.mixer_control_name = g_strdup( "Synth" );
    amidiplug_cfg.mixer_control_id = 0;
    amidiplug_cfg.length_precalc_enable = 0;
  }
  else
  {
    if ( !xmms_cfg_read_string( cfgfile , "amidi-plug" , "writable_ports" , &amidiplug_cfg.seq_writable_ports ) )
      amidiplug_cfg.seq_writable_ports = i_configure_read_seq_ports_default(); /* default value */

    if ( !xmms_cfg_read_int( cfgfile , "amidi-plug" , "mixer_card_id" , &amidiplug_cfg.mixer_card_id ) )
      amidiplug_cfg.mixer_card_id = 0; /* default value */

    if ( !xmms_cfg_read_string( cfgfile , "amidi-plug" , "mixer_control_name" , &amidiplug_cfg.mixer_control_name ) )
      amidiplug_cfg.mixer_control_name = g_strdup( "Synth" ); /* default value */

    if ( !xmms_cfg_read_int( cfgfile , "amidi-plug" , "mixer_control_id" , &amidiplug_cfg.mixer_control_id ) )
      amidiplug_cfg.mixer_control_id = 0; /* default value */

    if ( !xmms_cfg_read_int( cfgfile , "amidi-plug" , "length_precalc_enable" , &amidiplug_cfg.length_precalc_enable ) )
      amidiplug_cfg.length_precalc_enable = 0; /* default value */

    xmms_cfg_free(cfgfile);
  }
}


void i_configure_cfg_save( void )
{
  ConfigFile *cfgfile;
  cfgfile = xmms_cfg_open_default_file();

  if (!cfgfile)
    cfgfile = xmms_cfg_new();

  xmms_cfg_write_string( cfgfile , "amidi-plug" , "writable_ports" , amidiplug_cfg.seq_writable_ports );
  xmms_cfg_write_int( cfgfile , "amidi-plug" , "mixer_card_id" , amidiplug_cfg.mixer_card_id );
  xmms_cfg_write_string( cfgfile , "amidi-plug" , "mixer_control_name" , amidiplug_cfg.mixer_control_name );
  xmms_cfg_write_int( cfgfile , "amidi-plug" , "mixer_control_id" , amidiplug_cfg.mixer_control_id );
  xmms_cfg_write_int( cfgfile , "amidi-plug" , "length_precalc_enable" , amidiplug_cfg.length_precalc_enable );

  xmms_cfg_write_default_file(cfgfile);
  xmms_cfg_free(cfgfile);
}
