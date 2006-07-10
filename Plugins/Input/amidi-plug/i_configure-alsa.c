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


#include "i_configure-alsa.h"
#include "backend-alsa/b-alsa-config.h"
#include "backend-alsa/backend-alsa-icon.xpm"


enum
{
  LISTPORT_TOGGLE_COLUMN = 0,
  LISTPORT_PORTNUM_COLUMN,
  LISTPORT_CLIENTNAME_COLUMN,
  LISTPORT_PORTNAME_COLUMN,
  LISTPORT_POINTER_COLUMN,
  LISTPORT_N_COLUMNS
};

enum
{
  LISTCARD_NAME_COLUMN = 0,
  LISTCARD_ID_COLUMN,
  LISTCARD_MIXERPTR_COLUMN,
  LISTCARD_N_COLUMNS
};

enum
{
  LISTMIXER_NAME_COLUMN = 0,
  LISTMIXER_ID_COLUMN,
  LISTMIXER_N_COLUMNS
};


void i_configure_ev_portlv_changetoggle( GtkCellRendererToggle * rdtoggle ,
                                         gchar * path_str , gpointer data )
{
  GtkTreeModel *model = (GtkTreeModel*)data;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string( path_str );
  gboolean toggled;

  gtk_tree_model_get_iter( model , &iter , path );
  gtk_tree_model_get( model , &iter , LISTPORT_TOGGLE_COLUMN , &toggled , -1 );

  toggled ^= 1;
  gtk_list_store_set( GTK_LIST_STORE(model), &iter , LISTPORT_TOGGLE_COLUMN , toggled , -1 );

  gtk_tree_path_free (path);
}


gboolean i_configure_ev_mixctlcmb_inspect( GtkTreeModel * store , GtkTreePath * path,
                                           GtkTreeIter * iter , gpointer mixctl_cmb )
{
  gint ctl_id;
  gchar * ctl_name;
  amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
  gtk_tree_model_get( GTK_TREE_MODEL(store) , iter ,
                      LISTMIXER_ID_COLUMN , &ctl_id ,
                      LISTMIXER_NAME_COLUMN , &ctl_name , -1 );
  if (( !strcmp( ctl_name , alsacfg->alsa_mixer_ctl_name ) ) &&
      ( ctl_id == alsacfg->alsa_mixer_ctl_id ))
  {
    /* this is the selected control in the mixer control combobox */
    gtk_combo_box_set_active_iter( GTK_COMBO_BOX(mixctl_cmb) , iter );
    return TRUE;
  }
  g_free( ctl_name );
  return FALSE;
}


void i_configure_ev_cardcmb_changed( GtkWidget * card_cmb , gpointer mixctl_cmb )
{
  GtkTreeIter iter;
  if( gtk_combo_box_get_active_iter( GTK_COMBO_BOX(card_cmb) , &iter ) )
  {
    amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
    gpointer mixctl_store;
    gint card_id;
    GtkTreeModel * store = gtk_combo_box_get_model( GTK_COMBO_BOX(card_cmb) );
    gtk_tree_model_get( GTK_TREE_MODEL(store) , &iter ,
                        LISTCARD_ID_COLUMN , &card_id ,
                        LISTCARD_MIXERPTR_COLUMN , &mixctl_store , -1 );
    gtk_combo_box_set_model( GTK_COMBO_BOX(mixctl_cmb) , GTK_TREE_MODEL(mixctl_store) );

    /* check if the selected card is the one contained in configuration */
    if ( card_id == alsacfg->alsa_mixer_card_id )
    {
      /* search for the selected mixer control in combo box */
      gtk_tree_model_foreach( GTK_TREE_MODEL(mixctl_store) ,
                              i_configure_ev_mixctlcmb_inspect , mixctl_cmb );
    }
  }
}


gboolean i_configure_ev_portlv_inspecttoggle( GtkTreeModel * model , GtkTreePath * path ,
                                              GtkTreeIter * iter , gpointer wpp )
{
  gboolean toggled = FALSE;
  gchar * portstring;
  GString * wps = wpp;
  gtk_tree_model_get ( model , iter ,
                       LISTPORT_TOGGLE_COLUMN , &toggled ,
                       LISTPORT_PORTNUM_COLUMN , &portstring , -1 );
  if ( toggled ) /* check if the row points to an enabled port */
  {
    /* if this is not the first port added to wp, use comma as separator */
    if ( wps->str[0] != '\0' ) g_string_append_c( wps , ',' );
    g_string_append( wps , portstring );
  }
  g_free( portstring );
  return FALSE;
}


void i_configure_ev_portlv_commit( gpointer port_lv )
{
  amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
  GtkTreeModel * store;
  GString *wp = g_string_new( "" );
  /* get the model of the port listview */
  store = gtk_tree_view_get_model( GTK_TREE_VIEW(port_lv) );
  /* after going through this foreach, wp contains a comma-separated list of selected ports */
  gtk_tree_model_foreach( store , i_configure_ev_portlv_inspecttoggle , wp );
  g_free( alsacfg->alsa_seq_wports ); /* free previous */
  alsacfg->alsa_seq_wports = g_strdup( wp->str ); /* set with new */
  g_string_free( wp , TRUE ); /* not needed anymore */
  return;
}

void i_configure_ev_cardcmb_commit( gpointer card_cmb )
{
  GtkTreeModel * store;
  GtkTreeIter iter;
  store = gtk_combo_box_get_model( GTK_COMBO_BOX(card_cmb) );
  /* get the selected item */
  if ( gtk_combo_box_get_active_iter( GTK_COMBO_BOX(card_cmb) , &iter ) )
  {
    amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
    /* update amidiplug_cfg.alsa_mixer_card_id */
    gtk_tree_model_get( GTK_TREE_MODEL(store) , &iter ,
                        LISTCARD_ID_COLUMN ,
                        &alsacfg->alsa_mixer_card_id , -1 );
  }
  return;
}

void i_configure_ev_mixctlcmb_commit( gpointer mixctl_cmb )
{
  GtkTreeModel * store;
  GtkTreeIter iter;
  store = gtk_combo_box_get_model( GTK_COMBO_BOX(mixctl_cmb) );
  /* get the selected item */
  if ( gtk_combo_box_get_active_iter( GTK_COMBO_BOX(mixctl_cmb) , &iter ) )
  {
    amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
    g_free( alsacfg->alsa_mixer_ctl_name ); /* free previous */
    /* update amidiplug_cfg.alsa_mixer_card_id */
    gtk_tree_model_get( GTK_TREE_MODEL(store) , &iter ,
                        LISTMIXER_NAME_COLUMN ,
                        &alsacfg->alsa_mixer_ctl_name ,
                        LISTMIXER_ID_COLUMN ,
                        &alsacfg->alsa_mixer_ctl_id , -1 );
  }
  return;
}


void i_configure_gui_ctlcmb_datafunc( GtkCellLayout *cell_layout , GtkCellRenderer *cr ,
                                      GtkTreeModel *store , GtkTreeIter *iter , gpointer p )
{
  gchar *ctl_display, *ctl_name;
  gint ctl_id;
  gtk_tree_model_get( store , iter ,
                      LISTMIXER_NAME_COLUMN , &ctl_name ,
                      LISTMIXER_ID_COLUMN , &ctl_id , -1 );
  if ( ctl_id == 0 )
    ctl_display = g_strdup_printf( "%s" , ctl_name );
  else
    ctl_display = g_strdup_printf( "%s (%i)" , ctl_name , ctl_id );
  g_object_set( G_OBJECT(cr) , "text" , ctl_display , NULL );
  g_free( ctl_display );
  g_free( ctl_name );
}


void i_configure_gui_tab_alsa( GtkWidget * alsa_page_alignment ,
                               gpointer backend_list_p ,
                               gpointer commit_button )
{
  GtkWidget *alsa_page_vbox;
  GtkWidget *title_widget;
  GtkWidget *content_vbox; /* this vbox will contain two items of equal space (50%/50%) */
  GSList * backend_list = backend_list_p;
  gboolean alsa_module_ok = FALSE;
  gchar * alsa_module_pathfilename;

  alsa_page_vbox = gtk_vbox_new( FALSE , 0 );

  title_widget = i_configure_gui_draw_title( _("ALSA BACKEND CONFIGURATION") );
  gtk_box_pack_start( GTK_BOX(alsa_page_vbox) , title_widget , FALSE , FALSE , 2 );

  content_vbox = gtk_vbox_new( TRUE , 2 );

  /* check if the ALSA module is available */
  while ( backend_list != NULL )
  {
    amidiplug_sequencer_backend_name_t * mn = backend_list->data;
    if ( !strcmp( mn->name , "alsa" ) )
    {
      alsa_module_ok = TRUE;
      alsa_module_pathfilename = mn->filename;
      break;
    }
    backend_list = backend_list->next;
  }

  if ( alsa_module_ok )
  {
    GtkListStore *port_store, *mixer_card_store;
    GtkWidget *port_lv, *port_lv_sw, *port_lv_frame;
    GtkCellRenderer *port_lv_toggle_rndr, *port_lv_text_rndr;
    GtkTreeViewColumn *port_lv_toggle_col, *port_lv_portnum_col;
    GtkTreeViewColumn *port_lv_clientname_col, *port_lv_portname_col;
    GtkTreeSelection *port_lv_sel;
    GtkTreeIter iter;
    GtkWidget *mixer_table, *mixer_frame;
    GtkCellRenderer *mixer_card_cmb_text_rndr, *mixer_ctl_cmb_text_rndr;
    GtkWidget *mixer_card_cmb_evbox, *mixer_card_cmb, *mixer_card_label;
    GtkWidget *mixer_ctl_cmb_evbox, *mixer_ctl_cmb, *mixer_ctl_label;
    GtkTooltips *tips;

    amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;

    gchar **portstring_from_cfg = NULL;

    GModule * alsa_module;
    GSList *wports = NULL, *wports_h = NULL, *scards = NULL, *scards_h = NULL;
    GSList * (*get_port_list)( void );
    void (*free_port_list)( GSList * );
    GSList * (*get_card_list)( void );
    void (*free_card_list)( GSList * );

    if ( strlen( alsacfg->alsa_seq_wports ) > 0 )
      portstring_from_cfg = g_strsplit( alsacfg->alsa_seq_wports , "," , 0 );

    tips = gtk_tooltips_new();
    g_object_set_data_full( G_OBJECT(alsa_page_alignment) , "tt" , tips , g_object_unref );

    /* it's legit to assume that this can't fail,
       since the module is present in the backend_list */
    alsa_module = g_module_open( alsa_module_pathfilename , 0 );
    g_module_symbol( alsa_module , "sequencer_port_get_list" , (gpointer*)&get_port_list );
    g_module_symbol( alsa_module , "sequencer_port_free_list" , (gpointer*)&free_port_list );
    g_module_symbol( alsa_module , "alsa_card_get_list" , (gpointer*)&get_card_list );
    g_module_symbol( alsa_module , "alsa_card_free_list" , (gpointer*)&free_card_list );
    /* get an updated list of writable ALSA MIDI ports and ALSA-enabled sound cards*/
    wports = get_port_list(); wports_h = wports;
    scards = get_card_list(); scards_h = scards;

    /* ALSA MIDI PORT LISTVIEW */
    port_store = gtk_list_store_new( LISTPORT_N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING ,
                                     G_TYPE_STRING , G_TYPE_STRING , G_TYPE_POINTER );
    while ( wports != NULL )
    {
      gboolean toggled = FALSE;
      data_bucket_t * portinfo = (data_bucket_t *)wports->data;
      GString * portstring = g_string_new("");
      G_STRING_PRINTF( portstring , "%i:%i" , portinfo->bint[0] , portinfo->bint[1] );
      gtk_list_store_append( port_store , &iter );
      /* in the existing configuration there may be previously selected ports */
      if ( portstring_from_cfg != NULL )
      {
        gint i = 0;
        /* check if current row contains a port selected by user */
        for ( i = 0 ; portstring_from_cfg[i] != NULL ; i++ )
        {
          /* if it's one of the selected ports, toggle its checkbox */
          if ( !strcmp( portstring->str, portstring_from_cfg[i] ) )
            toggled = TRUE;
        }
      }
      gtk_list_store_set( port_store , &iter ,
                          LISTPORT_TOGGLE_COLUMN , toggled ,
                          LISTPORT_PORTNUM_COLUMN , portstring->str ,
                          LISTPORT_CLIENTNAME_COLUMN , portinfo->bcharp[0] ,
                          LISTPORT_PORTNAME_COLUMN , portinfo->bcharp[1] ,
                          LISTPORT_POINTER_COLUMN , portinfo , -1 );
      g_string_free( portstring , TRUE );
      /* on with next */
      wports = wports->next;
    }
    g_strfreev( portstring_from_cfg ); /* not needed anymore */
    port_lv = gtk_tree_view_new_with_model( GTK_TREE_MODEL(port_store) );
    gtk_tree_view_set_rules_hint( GTK_TREE_VIEW(port_lv), TRUE );
    g_object_unref( port_store );
    port_lv_toggle_rndr = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE(port_lv_toggle_rndr) , FALSE );
    gtk_cell_renderer_toggle_set_active( GTK_CELL_RENDERER_TOGGLE(port_lv_toggle_rndr) , TRUE );
    g_signal_connect( port_lv_toggle_rndr , "toggled" ,
                      G_CALLBACK(i_configure_ev_portlv_changetoggle) , port_store );
    port_lv_text_rndr = gtk_cell_renderer_text_new();
    port_lv_toggle_col = gtk_tree_view_column_new_with_attributes( "", port_lv_toggle_rndr,
                                                                   "active", LISTPORT_TOGGLE_COLUMN, NULL );
    port_lv_portnum_col = gtk_tree_view_column_new_with_attributes( _("Port"), port_lv_text_rndr,
                                                                    "text", LISTPORT_PORTNUM_COLUMN, NULL );
    port_lv_clientname_col = gtk_tree_view_column_new_with_attributes( _("Client name"), port_lv_text_rndr,
                                                                       "text", LISTPORT_CLIENTNAME_COLUMN, NULL );
    port_lv_portname_col = gtk_tree_view_column_new_with_attributes( _("Port name"), port_lv_text_rndr,
                                                                     "text", LISTPORT_PORTNAME_COLUMN, NULL );
    gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_toggle_col );
    gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_portnum_col );
    gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_clientname_col );
    gtk_tree_view_append_column( GTK_TREE_VIEW(port_lv), port_lv_portname_col );
    port_lv_sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(port_lv) );
    gtk_tree_selection_set_mode( GTK_TREE_SELECTION(port_lv_sel) , GTK_SELECTION_NONE );
    port_lv_sw = gtk_scrolled_window_new( NULL , NULL );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(port_lv_sw),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );
    port_lv_frame = gtk_frame_new( _("ALSA output ports") );
    gtk_container_add( GTK_CONTAINER(port_lv_sw) , port_lv );
    gtk_container_add( GTK_CONTAINER(port_lv_frame) , port_lv_sw );
    gtk_box_pack_start( GTK_BOX(content_vbox) , port_lv_frame , TRUE , TRUE , 0 );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_portlv_commit) , port_lv );

    /* MIXER CARD/CONTROL COMBOBOXES */
    mixer_card_store = gtk_list_store_new( LISTCARD_N_COLUMNS , G_TYPE_STRING , G_TYPE_INT , G_TYPE_POINTER );
    mixer_card_cmb = gtk_combo_box_new_with_model( GTK_TREE_MODEL(mixer_card_store) ); /* soundcard combo box */
    mixer_ctl_cmb = gtk_combo_box_new(); /* mixer control combo box */
    g_signal_connect( mixer_card_cmb , "changed" , G_CALLBACK(i_configure_ev_cardcmb_changed) , mixer_ctl_cmb );
    while ( scards != NULL )
    {
      GtkListStore *mixer_ctl_store;
      GtkTreeIter itermix;
      data_bucket_t * cardinfo = (data_bucket_t *)scards->data;
      GSList *mixctl_list = cardinfo->bpointer[0];
      mixer_ctl_store = gtk_list_store_new( LISTMIXER_N_COLUMNS , G_TYPE_STRING , G_TYPE_INT );
      while ( mixctl_list != NULL )
      {
        data_bucket_t * mixctlinfo = (data_bucket_t *)mixctl_list->data;
        gtk_list_store_append( mixer_ctl_store , &itermix );
        gtk_list_store_set( mixer_ctl_store , &itermix ,
                            LISTMIXER_NAME_COLUMN , mixctlinfo->bcharp[0] ,
                            LISTMIXER_ID_COLUMN , mixctlinfo->bint[0] , -1 );
        mixctl_list = mixctl_list->next; /* on with next mixer control */
      }
      gtk_list_store_append( mixer_card_store , &iter );
      gtk_list_store_set( mixer_card_store , &iter ,
                          LISTCARD_NAME_COLUMN , cardinfo->bcharp[0] ,
                          LISTCARD_ID_COLUMN , cardinfo->bint[0] ,
                          LISTCARD_MIXERPTR_COLUMN , mixer_ctl_store , -1 );
      /* check if this corresponds to the value previously selected by user */
      if ( cardinfo->bint[0] == alsacfg->alsa_mixer_card_id )
        gtk_combo_box_set_active_iter( GTK_COMBO_BOX(mixer_card_cmb) , &iter );
      scards = scards->next; /* on with next card */
    }
    g_object_unref( mixer_card_store ); /* free a reference, no longer needed */
    /* create renderer to display text in the mixer combo box */
    mixer_card_cmb_text_rndr = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(mixer_card_cmb) , mixer_card_cmb_text_rndr , TRUE );
    gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT(mixer_card_cmb) , mixer_card_cmb_text_rndr ,
                                   "text" , LISTCARD_NAME_COLUMN );
    mixer_ctl_cmb_text_rndr = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT(mixer_ctl_cmb) , mixer_ctl_cmb_text_rndr , TRUE );
    gtk_cell_layout_set_cell_data_func( GTK_CELL_LAYOUT(mixer_ctl_cmb) , mixer_ctl_cmb_text_rndr ,
                                        i_configure_gui_ctlcmb_datafunc , NULL , NULL );
    /* the event box is needed to display a tooltip for the mixer combo box */
    mixer_card_cmb_evbox = gtk_event_box_new();
    gtk_container_add( GTK_CONTAINER(mixer_card_cmb_evbox) , mixer_card_cmb );
    mixer_ctl_cmb_evbox = gtk_event_box_new();
    gtk_container_add( GTK_CONTAINER(mixer_ctl_cmb_evbox) , mixer_ctl_cmb );
    mixer_card_label = gtk_label_new( _("Soundcard: ") );
    gtk_misc_set_alignment( GTK_MISC(mixer_card_label) , 0 , 0.5 );
    mixer_ctl_label = gtk_label_new( _("Mixer control: ") );
    gtk_misc_set_alignment( GTK_MISC(mixer_ctl_label) , 0 , 0.5 );
    mixer_table = gtk_table_new( 3 , 2 , FALSE );
    gtk_container_set_border_width( GTK_CONTAINER(mixer_table), 4 );
    gtk_table_attach( GTK_TABLE(mixer_table) , mixer_card_label , 0 , 1 , 0 , 1 ,
                      GTK_FILL , 0 , 1 , 2 );
    gtk_table_attach( GTK_TABLE(mixer_table) , mixer_card_cmb_evbox , 1 , 2 , 0 , 1 ,
                      GTK_EXPAND | GTK_FILL , 0 , 1 , 2 );
    gtk_table_attach( GTK_TABLE(mixer_table) , mixer_ctl_label , 0 , 1 , 1 , 2 ,
                      GTK_FILL , 0 , 1 , 2 );
    gtk_table_attach( GTK_TABLE(mixer_table) , mixer_ctl_cmb_evbox , 1 , 2 , 1 , 2 ,
                      GTK_EXPAND | GTK_FILL , 0 , 1 , 2 );
    mixer_frame = gtk_frame_new( _("Mixer settings") );
    gtk_container_add( GTK_CONTAINER(mixer_frame) , mixer_table );
    gtk_box_pack_start( GTK_BOX(content_vbox) , mixer_frame , TRUE , TRUE , 0 );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_cardcmb_commit) , mixer_card_cmb );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_mixctlcmb_commit) , mixer_ctl_cmb );

    free_card_list( scards_h );
    free_port_list( wports_h );
    g_module_close( alsa_module );

    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , port_lv ,
                        _("* Select ALSA output ports *\n"
                        "MIDI events will be sent to the ports selected here. In example, if your "
                        "audio card provides a hardware synth and you want to play MIDI with it, "
                        "you'll probably want to select the wavetable synthesizer ports.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , mixer_card_cmb_evbox ,
                        _("* Select ALSA mixer card *\n"
                        "The ALSA backend outputs directly through ALSA, it doesn't use effect "
                        "and ouput plugins from the player. During playback, the player volume"
                        "slider will manipulate the mixer control you select here. "
                        "If you're using wavetable synthesizer ports, you'll probably want to "
                        "select the Synth control here.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , mixer_ctl_cmb_evbox ,
                        _("* Select ALSA mixer control *\n"
                        "The ALSA backend outputs directly through ALSA, it doesn't use effect "
                        "and ouput plugins from the player. During playback, the player volume "
                        "slider will manipulate the mixer control you select here. "
                        "If you're using wavetable synthesizer ports, you'll probably want to "
                        "select the Synth control here.") , "" );
  }
  else
  {
    /* display "not available" information */
    GtkWidget * info_label;
    info_label = gtk_label_new( _("ALSA Backend not loaded or not available") );
    gtk_box_pack_start( GTK_BOX(alsa_page_vbox) , info_label , FALSE , FALSE , 2 );
  }

  gtk_box_pack_start( GTK_BOX(alsa_page_vbox) , content_vbox , TRUE , TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(alsa_page_alignment) , alsa_page_vbox );
}


void i_configure_gui_tablabel_alsa( GtkWidget * alsa_page_alignment ,
                                    gpointer backend_list_p ,
                                    gpointer commit_button )
{
  GtkWidget *pagelabel_vbox, *pagelabel_image, *pagelabel_label;
  GdkPixbuf *pagelabel_image_pix;
  pagelabel_vbox = gtk_vbox_new( FALSE , 1 );
  pagelabel_image_pix = gdk_pixbuf_new_from_xpm_data( (const gchar **)backend_alsa_icon_xpm );
  pagelabel_image = gtk_image_new_from_pixbuf( pagelabel_image_pix ); g_object_unref( pagelabel_image_pix );
  pagelabel_label = gtk_label_new( "" );
  gtk_label_set_markup( GTK_LABEL(pagelabel_label) , "<span size=\"smaller\">ALSA\nbackend</span>" );
  gtk_label_set_justify( GTK_LABEL(pagelabel_label) , GTK_JUSTIFY_CENTER );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_image , FALSE , FALSE , 1 );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_label , FALSE , FALSE , 1 );
  gtk_container_add( GTK_CONTAINER(alsa_page_alignment) , pagelabel_vbox );
  gtk_widget_show_all( alsa_page_alignment );
  return;
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
     select the right ones in the nice config window :) */
  return g_strdup( "" );
}



void i_configure_cfg_alsa_alloc( void )
{
  amidiplug_cfg_alsa_t * alsacfg = g_malloc(sizeof(amidiplug_cfg_alsa_t));
  amidiplug_cfg_backend->alsa = alsacfg;
}


void i_configure_cfg_alsa_free( void )
{
  amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;
  g_free( alsacfg->alsa_seq_wports );
  g_free( alsacfg->alsa_mixer_ctl_name );
  g_free( amidiplug_cfg_backend->alsa );
}


void i_configure_cfg_alsa_read( pcfg_t * cfgfile )
{
  amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;

  if (!cfgfile)
  {
    /* alsa backend defaults */
    alsacfg->alsa_seq_wports = i_configure_read_seq_ports_default();
    alsacfg->alsa_mixer_card_id = 0;
    alsacfg->alsa_mixer_ctl_name = g_strdup( "Synth" );
    alsacfg->alsa_mixer_ctl_id = 0;
  }
  else
  {
    i_pcfg_read_string( cfgfile , "alsa" , "alsa_seq_wports" , &alsacfg->alsa_seq_wports , NULL );
    if ( alsacfg->alsa_seq_wports == NULL )
      alsacfg->alsa_seq_wports = i_configure_read_seq_ports_default(); /* pick default values */
    i_pcfg_read_integer( cfgfile , "alsa" , "alsa_mixer_card_id" , &alsacfg->alsa_mixer_card_id , 0 );
    i_pcfg_read_string( cfgfile , "alsa" , "alsa_mixer_ctl_name" , &alsacfg->alsa_mixer_ctl_name , "Synth" );
    i_pcfg_read_integer( cfgfile , "alsa" , "alsa_mixer_ctl_id" , &alsacfg->alsa_mixer_ctl_id , 0 );
  }
}


void i_configure_cfg_alsa_save( pcfg_t * cfgfile )
{
  amidiplug_cfg_alsa_t * alsacfg = amidiplug_cfg_backend->alsa;

  i_pcfg_write_string( cfgfile , "alsa" , "alsa_seq_wports" , alsacfg->alsa_seq_wports );
  i_pcfg_write_integer( cfgfile , "alsa" , "alsa_mixer_card_id" , alsacfg->alsa_mixer_card_id );
  i_pcfg_write_string( cfgfile , "alsa" , "alsa_mixer_ctl_name" , alsacfg->alsa_mixer_ctl_name );
  i_pcfg_write_integer( cfgfile , "alsa" , "alsa_mixer_ctl_id" , alsacfg->alsa_mixer_ctl_id );
}
