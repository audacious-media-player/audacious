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


#include "i_configure.h"
#include "i_configure_private.h"
#include "i_backend.h"
#include "i_configure-ap.h"
#include "i_configure-alsa.h"
#include "i_configure-fluidsynth.h"
#include "i_configure-dummy.h"
#include "i_utils.h"
#include "libaudacious/beepctrl.h"


amidiplug_cfg_backend_t * amidiplug_cfg_backend;


void i_configure_ev_bcancel( gpointer );
void i_configure_ev_bapply( GtkWidget * , gpointer );
void i_configure_ev_bokcheck( GtkWidget * , gpointer );
void i_configure_ev_bok( GtkWidget * , gpointer );
void i_configure_cfg_backend_alloc( void );
void i_configure_cfg_backend_free( void );
void i_configure_cfg_backend_save( void );
void i_configure_cfg_backend_read( void );
void i_configure_cfg_ap_save( void );
void i_configure_cfg_ap_read( void );


GtkWidget * i_configure_gui_draw_title( gchar * title_string )
{
  GtkWidget *title_label, *title_evbox, *title_frame;
  GtkStyle * style = gtk_widget_get_default_style();
  GdkColor title_fgcol = style->fg[GTK_STATE_SELECTED];
  GdkColor title_bgcol = style->bg[GTK_STATE_SELECTED];
  title_label = gtk_label_new( title_string );
  title_evbox = gtk_event_box_new();
  title_frame = gtk_frame_new( NULL );
  gtk_frame_set_shadow_type( GTK_FRAME(title_frame) , GTK_SHADOW_OUT );
  gtk_container_add( GTK_CONTAINER(title_evbox) , title_label );
  gtk_container_set_border_width( GTK_CONTAINER(title_evbox) , 5 );
  gtk_container_add( GTK_CONTAINER(title_frame) , title_evbox );
  gtk_widget_modify_fg( GTK_WIDGET(title_label) , GTK_STATE_NORMAL , &title_fgcol );
  gtk_widget_modify_bg( GTK_WIDGET(title_evbox) , GTK_STATE_NORMAL , &title_bgcol );
  return title_frame;
}


void i_configure_ev_browse_for_entry( GtkWidget * target_entry )
{
  GtkWidget *parent_window = gtk_widget_get_toplevel( target_entry );
  GtkFileChooserAction act = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(target_entry),"fc-act"));
  if ( GTK_WIDGET_TOPLEVEL(parent_window) )
  {
    GtkWidget *browse_dialog = gtk_file_chooser_dialog_new( _("AMIDI-Plug - select file") ,
                                                            GTK_WINDOW(parent_window) , act ,
                                                            GTK_STOCK_CANCEL , GTK_RESPONSE_CANCEL ,
                                                            GTK_STOCK_OPEN , GTK_RESPONSE_ACCEPT , NULL );
    if ( strcmp( gtk_entry_get_text(GTK_ENTRY(target_entry)) , "" ) )
      gtk_file_chooser_set_filename( GTK_FILE_CHOOSER(browse_dialog) ,
                                     gtk_entry_get_text(GTK_ENTRY(target_entry)) );
    if ( gtk_dialog_run(GTK_DIALOG(browse_dialog)) == GTK_RESPONSE_ACCEPT )
    {
      gchar *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(browse_dialog) );
      gtk_entry_set_text( GTK_ENTRY(target_entry) , filename );
      DEBUGMSG( "selected file: %s\n" , filename );
      g_free( filename );
    }
    gtk_widget_destroy( browse_dialog );
  }
}


void i_configure_gui( void )
{
  static GtkWidget *configwin = NULL;
  GdkGeometry cw_hints;
  GtkWidget *configwin_vbox;
  GtkWidget *hseparator, *hbuttonbox, *button_ok, *button_cancel, *button_apply;

  GtkWidget *configwin_notebook;

  GtkWidget *ap_page_alignment, *ap_pagelabel_alignment; /* amidi-plug */
  GtkWidget *alsa_page_alignment, *alsa_pagelabel_alignment; /* alsa */
  GtkWidget *dumm_page_alignment, *dumm_pagelabel_alignment; /* dummy */
  GtkWidget *fsyn_page_alignment, *fsyn_pagelabel_alignment; /* fluidsynth */

  GSList *backend_list = NULL, *backend_list_h = NULL;

  if ( configwin != NULL )
  {
    DEBUGMSG( "config window is already open!\n" );
    return;
  }

  /* get configuration information for backends */
  i_configure_cfg_backend_alloc();
  i_configure_cfg_backend_read();

  configwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(configwin), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_title( GTK_WINDOW(configwin), _("AMIDI-Plug - configuration") );
  gtk_container_set_border_width( GTK_CONTAINER(configwin), 10 );
  g_signal_connect( G_OBJECT(configwin) , "destroy" ,
                    G_CALLBACK(gtk_widget_destroyed) , &configwin );
  button_ok = gtk_button_new_from_stock( GTK_STOCK_OK );
  if ( g_signal_lookup( "ap-commit" , GTK_WIDGET_TYPE(button_ok) ) == 0 )
  {
    g_signal_new( "ap-commit" , GTK_WIDGET_TYPE(button_ok) ,
                  G_SIGNAL_ACTION , 0 , NULL , NULL ,
                  g_cclosure_marshal_VOID__VOID , G_TYPE_NONE , 0 );
  }
  g_signal_connect( G_OBJECT(button_ok) , "clicked" ,
                    G_CALLBACK(i_configure_ev_bokcheck) , configwin );
  cw_hints.min_width = 480; cw_hints.min_height = -1;
  gtk_window_set_geometry_hints( GTK_WINDOW(configwin) , GTK_WIDGET(configwin) ,
                                 &cw_hints , GDK_HINT_MIN_SIZE );

  configwin_vbox = gtk_vbox_new( FALSE , 0 );
  gtk_container_add( GTK_CONTAINER(configwin) , configwin_vbox );

  configwin_notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos( GTK_NOTEBOOK(configwin_notebook) , GTK_POS_LEFT );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , configwin_notebook , TRUE , TRUE , 2 );

  /* GET A LIST OF BACKENDS */
  backend_list = i_backend_list_lookup(); /* get a list of available backends */;
  backend_list_h = backend_list;

  /* AMIDI-PLUG PREFERENCES TAB */
  ap_pagelabel_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  ap_page_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  gtk_alignment_set_padding( GTK_ALIGNMENT(ap_page_alignment) , 3 , 3 , 8 , 3 );
  i_configure_gui_tab_ap( ap_page_alignment , backend_list , button_ok );
  i_configure_gui_tablabel_ap( ap_pagelabel_alignment , backend_list , button_ok );
  gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
                            ap_page_alignment , ap_pagelabel_alignment );

  /* ALSA BACKEND CONFIGURATION TAB */
  alsa_pagelabel_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  alsa_page_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  gtk_alignment_set_padding( GTK_ALIGNMENT(alsa_page_alignment) , 3 , 3 , 8 , 3 );
  i_configure_gui_tab_alsa( alsa_page_alignment , backend_list , button_ok );
  i_configure_gui_tablabel_alsa( alsa_pagelabel_alignment , backend_list , button_ok );
  gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
                            alsa_page_alignment , alsa_pagelabel_alignment );

  /* FLUIDSYNTH BACKEND CONFIGURATION TAB */
  fsyn_pagelabel_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  fsyn_page_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  gtk_alignment_set_padding( GTK_ALIGNMENT(fsyn_page_alignment) , 3 , 3 , 8 , 3 );
  i_configure_gui_tab_fsyn( fsyn_page_alignment , backend_list , button_ok );
  i_configure_gui_tablabel_fsyn( fsyn_pagelabel_alignment , backend_list , button_ok );
  gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
                            fsyn_page_alignment , fsyn_pagelabel_alignment );

  /* DUMMY BACKEND CONFIGURATION TAB */
  dumm_pagelabel_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  dumm_page_alignment = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
  gtk_alignment_set_padding( GTK_ALIGNMENT(dumm_page_alignment) , 3 , 3 , 8 , 3 );
  i_configure_gui_tab_dumm( dumm_page_alignment , backend_list , button_ok );
  i_configure_gui_tablabel_dumm( dumm_pagelabel_alignment , backend_list , button_ok );
  gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
                            dumm_page_alignment , dumm_pagelabel_alignment );

  i_backend_list_free( backend_list_h ); /* done, free the list of available backends */

  /* horizontal separator and buttons */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , hseparator , FALSE , FALSE , 4 );
  hbuttonbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(hbuttonbox) , GTK_BUTTONBOX_END );
  button_apply = gtk_button_new_from_stock( GTK_STOCK_APPLY );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_apply );
  button_cancel = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
  g_signal_connect_swapped( G_OBJECT(button_cancel) , "clicked" ,
                            G_CALLBACK(i_configure_ev_bcancel) , configwin );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_cancel );
  /* button_ok = gtk_button_new_from_stock( GTK_STOCK_OK ); created above */
  g_object_set_data( G_OBJECT(button_ok) , "bapply_pressed" , GUINT_TO_POINTER(0) );
  g_object_set_data( G_OBJECT(button_apply) , "bok" , button_ok );
  g_signal_connect( G_OBJECT(button_ok) , "ap-commit" ,
                    G_CALLBACK(i_configure_ev_bok) , configwin );
  g_signal_connect( G_OBJECT(button_apply) , "clicked" ,
                    G_CALLBACK(i_configure_ev_bapply) , configwin );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

  gtk_widget_show_all( configwin );
}


void i_configure_ev_bcancel( gpointer configwin )
{
  i_configure_cfg_backend_free(); /* free backend settings */
  gtk_widget_destroy(GTK_WIDGET(configwin));
}


void i_configure_ev_bapply( GtkWidget * button_apply , gpointer configwin )
{
  GtkWidget *button_ok = g_object_get_data( G_OBJECT(button_apply) , "bok" );
  g_object_set_data( G_OBJECT(button_ok) , "bapply_pressed" , GUINT_TO_POINTER(1) );
  i_configure_ev_bokcheck( button_ok , configwin );
}


void i_configure_ev_bokcheck( GtkWidget * button_ok , gpointer configwin )
{
  if ( xmms_remote_is_playing(0) || xmms_remote_is_paused(0) )
  {
    /* we can't change settings while a song is being played */
    static GtkWidget * configwin_warnmsg = NULL;
    g_object_set_data( G_OBJECT(button_ok) , "bapply_pressed" , GUINT_TO_POINTER(0) );
    if ( configwin_warnmsg != NULL )
    {
      gdk_window_raise( configwin_warnmsg->window );
    }
    else
    {
      configwin_warnmsg = (GtkWidget*)i_message_gui( _("AMIDI-Plug message") ,
                                        _("Please stop the player before changing AMIDI-Plug settings.") ,
                                        AMIDIPLUG_MESSAGE_WARN , configwin , FALSE );
      g_signal_connect( G_OBJECT(configwin_warnmsg) , "destroy" ,
                        G_CALLBACK(gtk_widget_destroyed) , &configwin_warnmsg );
      gtk_widget_show_all( configwin_warnmsg );
    }
  }
  else
  {
    g_signal_emit_by_name( G_OBJECT(button_ok) , "ap-commit" ); /* call commit actions */
  }
}


void i_configure_ev_bok( GtkWidget * button_ok , gpointer configwin )
{
  DEBUGMSG( "saving configuration...\n" );
  i_configure_cfg_ap_save(); /* save amidiplug settings */
  i_configure_cfg_backend_save(); /* save backend settings */
  DEBUGMSG( "configuration saved\n" );

  /* check if a different backend has been selected */
  if (( backend.name == NULL ) || ( strcmp( amidiplug_cfg_ap.ap_seq_backend , backend.name ) ))
  {
    DEBUGMSG( "a new backend has been selected, unloading previous and loading the new one\n" );
    i_backend_unload(); /* unload previous backend */
    i_backend_load( amidiplug_cfg_ap.ap_seq_backend ); /* load new backend */
  }
  else /* same backend, just reload updated configuration */
  {
    if ( backend.gmodule != NULL )
    {
      DEBUGMSG( "the selected backend is already loaded, so just perform backend cleanup and reinit\n" );
      backend.cleanup();
      backend.init();
    }
  }

  if ( GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button_ok),"bapply_pressed")) == 1 )
  {
    g_object_set_data( G_OBJECT(button_ok) , "bapply_pressed" , GUINT_TO_POINTER(0) );
  }
  else
  {
    i_configure_cfg_backend_free(); /* free backend settings */
    gtk_widget_destroy(GTK_WIDGET(configwin));
  }
}


void i_configure_cfg_backend_alloc( void )
{
  amidiplug_cfg_backend = g_malloc(sizeof(amidiplug_cfg_backend));

  i_configure_cfg_alsa_alloc(); /* alloc alsa backend configuration */
  i_configure_cfg_fsyn_alloc(); /* alloc fluidsynth backend configuration */
  i_configure_cfg_dumm_alloc(); /* alloc dummy backend configuration */
}


void i_configure_cfg_backend_free( void )
{
  i_configure_cfg_alsa_free(); /* free alsa backend configuration */
  i_configure_cfg_fsyn_free(); /* free fluidsynth backend configuration */
  i_configure_cfg_dumm_free(); /* free dummy backend configuration */

  g_free( amidiplug_cfg_backend );
}


void i_configure_cfg_backend_read( void )
{
  pcfg_t *cfgfile;

  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  i_configure_cfg_alsa_read( cfgfile ); /* get alsa backend configuration */
  i_configure_cfg_fsyn_read( cfgfile ); /* get fluidsynth backend configuration */
  i_configure_cfg_dumm_read( cfgfile ); /* get dummy backend configuration */

  if ( cfgfile != NULL )
    i_pcfg_free(cfgfile);

  g_free( config_pathfilename );
}


void i_configure_cfg_backend_save( void )
{
  pcfg_t *cfgfile;
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if (!cfgfile)
    cfgfile = i_pcfg_new();

  i_configure_cfg_alsa_save( cfgfile ); /* save alsa backend configuration */
  i_configure_cfg_fsyn_save( cfgfile ); /* save fluidsynth backend configuration */
  i_configure_cfg_dumm_save( cfgfile ); /* save dummy backend configuration */

  i_pcfg_write_to_file( cfgfile , config_pathfilename );
  i_pcfg_free( cfgfile );
  g_free( config_pathfilename );
}


/* read only the amidi-plug part of configuration */
void i_configure_cfg_ap_read( void )
{
  pcfg_t *cfgfile;
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if (!cfgfile)
  {
    /* amidi-plug defaults */
    amidiplug_cfg_ap.ap_seq_backend = g_strdup( "alsa" );
    amidiplug_cfg_ap.ap_opts_length_precalc = 0;
    amidiplug_cfg_ap.ap_opts_lyrics_extract = 0;
    amidiplug_cfg_ap.ap_opts_comments_extract = 0;
  }
  else
  {
    i_pcfg_read_string( cfgfile , "general" , "ap_seq_backend" ,
                        &amidiplug_cfg_ap.ap_seq_backend , "alsa" );
    i_pcfg_read_integer( cfgfile , "general" , "ap_opts_length_precalc" ,
                         &amidiplug_cfg_ap.ap_opts_length_precalc , 0 );
    i_pcfg_read_integer( cfgfile , "general" , "ap_opts_lyrics_extract" ,
                         &amidiplug_cfg_ap.ap_opts_lyrics_extract , 0 );
    i_pcfg_read_integer( cfgfile , "general" , "ap_opts_comments_extract" ,
                         &amidiplug_cfg_ap.ap_opts_comments_extract , 0 );
    i_pcfg_free( cfgfile );
  }

  g_free( config_pathfilename );
}


void i_configure_cfg_ap_save( void )
{
  pcfg_t *cfgfile;
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if (!cfgfile)
    cfgfile = i_pcfg_new();

  /* save amidi-plug config information */
  i_pcfg_write_string( cfgfile , "general" , "ap_seq_backend" ,
                       amidiplug_cfg_ap.ap_seq_backend );
  i_pcfg_write_integer( cfgfile , "general" , "ap_opts_length_precalc" ,
                        amidiplug_cfg_ap.ap_opts_length_precalc );
  i_pcfg_write_integer( cfgfile , "general" , "ap_opts_lyrics_extract" ,
                        amidiplug_cfg_ap.ap_opts_lyrics_extract );
  i_pcfg_write_integer( cfgfile , "general" , "ap_opts_comments_extract" ,
                        amidiplug_cfg_ap.ap_opts_comments_extract );

  i_pcfg_write_to_file( cfgfile , config_pathfilename );
  i_pcfg_free( cfgfile );
  g_free( config_pathfilename );
}
