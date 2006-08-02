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


#include "i_configure-dummy.h"
#include "backend-dummy/b-dummy-config.h"
#include "backend-dummy/backend-dummy-icon.xpm"


void i_configure_ev_logfile_toggle( GtkToggleButton *diflogfiles_radiobt , gpointer logfile_table )
{
  GList * tc_list = gtk_container_get_children( GTK_CONTAINER(logfile_table) );
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(diflogfiles_radiobt) ) )
  {
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,0) , TRUE ); /* dir bbutton */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,1) , TRUE ); /* dir entry */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,2) , TRUE ); /* dir label */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,4) , FALSE ); /* file bbutton */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,5) , FALSE ); /* file entry */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,6) , FALSE ); /* file label */
  }
  else
  {
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,0) , FALSE ); /* dir bbutton */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,1) , FALSE ); /* dir entry */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,2) , FALSE ); /* dir label */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,4) , TRUE ); /* file bbutton */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,5) , TRUE ); /* file entry */
    gtk_widget_set_sensitive( g_list_nth_data(tc_list,6) , TRUE ); /* file label */
  }
}


void i_configure_ev_enablelog_toggle( GtkToggleButton *logwithfile_radiobt , gpointer logfile_table )
{
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(logwithfile_radiobt) ) )
  {
    gtk_widget_set_sensitive( GTK_WIDGET(logfile_table) , TRUE );
    i_configure_ev_logfile_toggle( g_object_get_data(G_OBJECT(logfile_table),"lfstyle-opt2") , logfile_table );
  }
  else
  {
    gtk_widget_set_sensitive( GTK_WIDGET(logfile_table) , FALSE );
  }
}


void i_configure_ev_enablelog_commit( gpointer enablelog_radiobt )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  GSList *group = gtk_radio_button_get_group( GTK_RADIO_BUTTON(enablelog_radiobt) );
  while ( group != NULL )
  {
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(group->data) ) )
    {
      dummcfg->dumm_logger_enable = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(group->data),"val"));
      group = NULL; /* do not iterate further */
    }
    else
      group = group->next;
  }
}

void i_configure_ev_lfstyle_commit( gpointer lfstyle_radiobt )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  GSList *group = gtk_radio_button_get_group( GTK_RADIO_BUTTON(lfstyle_radiobt) );
  while ( group != NULL )
  {
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(group->data) ) )
    {
      dummcfg->dumm_logger_lfstyle = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(group->data),"val"));
      group = NULL; /* do not iterate further */
    }
    else
      group = group->next;
  }
}


void i_configure_ev_plspeed_commit( gpointer plspeed_normal_radiobt )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(plspeed_normal_radiobt) ) )
    dummcfg->dumm_playback_speed = 0;
  else
    dummcfg->dumm_playback_speed = 1;
}


void i_configure_ev_logfname_commit( gpointer logfname_entry )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  g_free( dummcfg->dumm_logger_logfile ); /* free previous */
  dummcfg->dumm_logger_logfile = g_strdup( gtk_entry_get_text(GTK_ENTRY(logfname_entry)) );
}


void i_configure_ev_logdir_commit( gpointer logdir_entry )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  g_free( dummcfg->dumm_logger_logdir ); /* free previous */
  dummcfg->dumm_logger_logdir = g_strdup( gtk_entry_get_text(GTK_ENTRY(logdir_entry)) );
}


void i_configure_gui_tab_dumm( GtkWidget * dumm_page_alignment ,
                               gpointer backend_list_p ,
                               gpointer commit_button )
{
  GtkWidget *dumm_page_vbox;
  GtkWidget *title_widget;
  GtkWidget *content_vbox;
  GSList * backend_list = backend_list_p;
  gboolean dumm_module_ok = FALSE;
  gchar * dumm_module_pathfilename;

  dumm_page_vbox = gtk_vbox_new( FALSE , 0 );

  title_widget = i_configure_gui_draw_title( _("DUMMY BACKEND CONFIGURATION") );
  gtk_box_pack_start( GTK_BOX(dumm_page_vbox) , title_widget , FALSE , FALSE , 2 );

  content_vbox = gtk_vbox_new( FALSE , 2 );

  /* check if the dummy module is available */
  while ( backend_list != NULL )
  {
    amidiplug_sequencer_backend_name_t * mn = backend_list->data;
    if ( !strcmp( mn->name , "dummy" ) )
    {
      dumm_module_ok = TRUE;
      dumm_module_pathfilename = mn->filename;
      break;
    }
    backend_list = backend_list->next;
  }

  if ( dumm_module_ok )
  {
    GtkWidget *midilogger_frame, *midilogger_vbox;
    GtkWidget *midilogger_enablelog_vbox, *midilogger_enablelog_option[4];
    GtkWidget *midilogger_logfile_frame, *midilogger_logfile_table, *midilogger_logfile_option[3];
    GtkWidget *midilogger_logfile_logdir_labelalign, *midilogger_logfile_logdir_label;
    GtkWidget *midilogger_logfile_logdir_entry, *midilogger_logfile_logdir_bbutton;
    GtkWidget *midilogger_logfile_logfname_labelalign, *midilogger_logfile_logfname_label;
    GtkWidget *midilogger_logfile_logfname_entry, *midilogger_logfile_logfname_bbutton;
    GtkWidget *plspeed_frame, *plspeed_vbox, *plspeed_option[2];
    GtkTooltips *tips;

    amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;

    tips = gtk_tooltips_new();
    g_object_set_data_full( G_OBJECT(dumm_page_alignment) , "tt" , tips , g_object_unref );

    /* midilogger settings */
    midilogger_frame = gtk_frame_new( _("MIDI logger settings") );
    midilogger_vbox = gtk_vbox_new( FALSE , 4 );
    gtk_container_set_border_width( GTK_CONTAINER(midilogger_vbox), 4 );
    gtk_container_add( GTK_CONTAINER(midilogger_frame) , midilogger_vbox );

    /* midilogger settings - enable/disable */
    midilogger_enablelog_vbox = gtk_vbox_new( FALSE , 0 );
    midilogger_enablelog_option[0] = gtk_radio_button_new_with_label( NULL ,
                                       _("Do not log anything") );
    g_object_set_data( G_OBJECT(midilogger_enablelog_option[0]) , "val" , GINT_TO_POINTER(0) );
    midilogger_enablelog_option[1] = gtk_radio_button_new_with_label_from_widget(
                                       GTK_RADIO_BUTTON(midilogger_enablelog_option[0]) ,
                                       _("Log MIDI events to standard output") );
    g_object_set_data( G_OBJECT(midilogger_enablelog_option[1]) , "val" , GINT_TO_POINTER(1) );
    midilogger_enablelog_option[2] = gtk_radio_button_new_with_label_from_widget(
                                       GTK_RADIO_BUTTON(midilogger_enablelog_option[0]) ,
                                       _("Log MIDI events to standard error") );
    g_object_set_data( G_OBJECT(midilogger_enablelog_option[2]) , "val" , GINT_TO_POINTER(2) );
    midilogger_enablelog_option[3] = gtk_radio_button_new_with_label_from_widget(
                                       GTK_RADIO_BUTTON(midilogger_enablelog_option[0]) ,
                                       _("Log MIDI events to file") );
    g_object_set_data( G_OBJECT(midilogger_enablelog_option[3]) , "val" , GINT_TO_POINTER(3) );
    gtk_box_pack_start( GTK_BOX(midilogger_enablelog_vbox) , midilogger_enablelog_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(midilogger_enablelog_vbox) , midilogger_enablelog_option[1] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(midilogger_enablelog_vbox) , midilogger_enablelog_option[2] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(midilogger_enablelog_vbox) , midilogger_enablelog_option[3] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(midilogger_vbox) , midilogger_enablelog_vbox , FALSE , FALSE , 0 );

    /* midilogger settings - logfile options */
    midilogger_logfile_frame = gtk_frame_new( _("Logfile settings") );
    midilogger_logfile_table = gtk_table_new( 5 , 3 , FALSE );
    gtk_table_set_col_spacings( GTK_TABLE(midilogger_logfile_table) , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(midilogger_logfile_table), 4 );
    gtk_container_add( GTK_CONTAINER(midilogger_logfile_frame) , midilogger_logfile_table );
    gtk_box_pack_start( GTK_BOX(midilogger_vbox) , midilogger_logfile_frame , FALSE , FALSE , 0 );
    midilogger_logfile_option[0] = gtk_radio_button_new_with_label( NULL ,
                                     _("Use a single file to log everything (rewrite)") );
    g_object_set_data( G_OBJECT(midilogger_logfile_option[0]) , "val" , GINT_TO_POINTER(0) );
    midilogger_logfile_option[1] = gtk_radio_button_new_with_label_from_widget(
                                     GTK_RADIO_BUTTON(midilogger_logfile_option[0]) ,
                                     _("Use a single file to log everything (append)") );
    g_object_set_data( G_OBJECT(midilogger_logfile_option[1]) , "val" , GINT_TO_POINTER(1) );
    midilogger_logfile_option[2] = gtk_radio_button_new_with_label_from_widget(
                                     GTK_RADIO_BUTTON(midilogger_logfile_option[0]) ,
                                     _("Use a different logfile for each MIDI file") );
    g_object_set_data( G_OBJECT(midilogger_logfile_option[2]) , "val" , GINT_TO_POINTER(2) );
    g_object_set_data( G_OBJECT(midilogger_logfile_table) , "lfstyle-opt2" , midilogger_logfile_option[2] );
    /* midilogger settings - logfile options - dir entry box */
    midilogger_logfile_logdir_labelalign = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
    gtk_alignment_set_padding( GTK_ALIGNMENT(midilogger_logfile_logdir_labelalign) , 0 , 0 , 15 , 0 );
    midilogger_logfile_logdir_label = gtk_label_new( _("\302\273 Log dir:") );
    gtk_container_add( GTK_CONTAINER(midilogger_logfile_logdir_labelalign) , midilogger_logfile_logdir_label );
    midilogger_logfile_logdir_entry = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY(midilogger_logfile_logdir_entry) , dummcfg->dumm_logger_logdir );
    g_object_set_data( G_OBJECT(midilogger_logfile_logdir_entry) , "fc-act",
                       GINT_TO_POINTER(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) );
    midilogger_logfile_logdir_bbutton = gtk_button_new_with_label( _("browse") );
    g_signal_connect_swapped( G_OBJECT(midilogger_logfile_logdir_bbutton) , "clicked" ,
                              G_CALLBACK(i_configure_ev_browse_for_entry) , midilogger_logfile_logdir_entry );
    /* midilogger settings - logfile options - file entry box */
    midilogger_logfile_logfname_labelalign = gtk_alignment_new( 0.5 , 0.5 , 1 , 1 );
    gtk_alignment_set_padding( GTK_ALIGNMENT(midilogger_logfile_logfname_labelalign) , 0 , 0 , 15 , 0 );
    midilogger_logfile_logfname_label = gtk_label_new( _("\302\273 Log file:") );
    gtk_container_add( GTK_CONTAINER(midilogger_logfile_logfname_labelalign) , midilogger_logfile_logfname_label );
    midilogger_logfile_logfname_entry = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY(midilogger_logfile_logfname_entry) , dummcfg->dumm_logger_logfile );
    g_object_set_data( G_OBJECT(midilogger_logfile_logfname_entry) , "fc-act",
                       GINT_TO_POINTER(GTK_FILE_CHOOSER_ACTION_OPEN) );
    midilogger_logfile_logfname_bbutton = gtk_button_new_with_label( _("browse") );
    g_signal_connect_swapped( G_OBJECT(midilogger_logfile_logfname_bbutton) , "clicked" ,
                              G_CALLBACK(i_configure_ev_browse_for_entry) , midilogger_logfile_logfname_entry );
    /* midilogger settings - logfile options - pack everything in table */
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_option[0] ,
                      0 , 3 , 0 , 1 , GTK_EXPAND | GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_option[1] ,
                      0 , 3 , 1 , 2 , GTK_EXPAND | GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logfname_labelalign ,
                      0 , 1 , 2 , 3 , GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logfname_entry ,
                      1 , 2 , 2 , 3 , GTK_EXPAND | GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logfname_bbutton ,
                      2 , 3 , 2 , 3 , GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_option[2] ,
                      0 , 3 , 3 , 4 , GTK_EXPAND | GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logdir_labelalign ,
                      0 , 1 , 4 , 5 , GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logdir_entry ,
                      1 , 2 , 4 , 5 , GTK_EXPAND | GTK_FILL , 0 , 0 , 0 );
    gtk_table_attach( GTK_TABLE(midilogger_logfile_table) , midilogger_logfile_logdir_bbutton ,
                      2 , 3 , 4 , 5 , GTK_FILL , 0 , 0 , 0 );

    gtk_box_pack_start( GTK_BOX(content_vbox) , midilogger_frame , FALSE , FALSE , 0 );

    /* playback speed settings */
    plspeed_frame = gtk_frame_new( _("Playback speed") );
    plspeed_vbox = gtk_vbox_new( FALSE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(plspeed_vbox), 4 );
    gtk_container_add( GTK_CONTAINER(plspeed_frame) , plspeed_vbox );
    plspeed_option[0] = gtk_radio_button_new_with_label( NULL ,
                          _("Play at normal speed") );
    plspeed_option[1] = gtk_radio_button_new_with_label_from_widget(
                          GTK_RADIO_BUTTON(plspeed_option[0]) ,
                          _("Play as fast as possible") );
    gtk_box_pack_start( GTK_BOX(plspeed_vbox) , plspeed_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(plspeed_vbox) , plspeed_option[1] , FALSE , FALSE , 0 );

    gtk_box_pack_start( GTK_BOX(content_vbox) , plspeed_frame , FALSE , FALSE , 0 );

    /* set values */
    /* we check this before connecting signals, it would be a useless work to
       set/unset widget sensitivity at this time; in fact everything in the table is
       going to lose sensitivity anyway, until i_configure_ev_enablelog_toggle is called */
    switch ( dummcfg->dumm_logger_lfstyle )
    {
      case 1: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_logfile_option[1]) , TRUE ); break;
      case 2: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_logfile_option[2]) , TRUE ); break;
      case 0:
      default: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_logfile_option[0]) , TRUE ); break;
    }
    gtk_widget_set_sensitive( GTK_WIDGET(midilogger_logfile_table) , FALSE );
    g_signal_connect( G_OBJECT(midilogger_enablelog_option[3]) , "toggled" ,
                      G_CALLBACK(i_configure_ev_enablelog_toggle) , midilogger_logfile_table );
    g_signal_connect( G_OBJECT(midilogger_logfile_option[2]) , "toggled" ,
                      G_CALLBACK(i_configure_ev_logfile_toggle) , midilogger_logfile_table );
    switch ( dummcfg->dumm_logger_enable )
    {
      case 1: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_enablelog_option[1]) , TRUE ); break;
      case 2: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_enablelog_option[2]) , TRUE ); break;
      case 3: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_enablelog_option[3]) , TRUE ); break;
      case 0:
      default: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(midilogger_enablelog_option[0]) , TRUE ); break;
    }
    switch ( dummcfg->dumm_playback_speed )
    {
      case 1: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(plspeed_option[1]) , TRUE ); break;
      case 0:
      default: gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(plspeed_option[0]) , TRUE ); break;
    }

    /* commit events */
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_enablelog_commit) , midilogger_enablelog_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_lfstyle_commit) , midilogger_logfile_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_plspeed_commit) , plspeed_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_logfname_commit) , midilogger_logfile_logfname_entry );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_logdir_commit) , midilogger_logfile_logdir_entry );

  }
  else
  {
    /* display "not available" information */
    GtkWidget * info_label;
    info_label = gtk_label_new( _("Dummy Backend not loaded or not available") );
    gtk_box_pack_start( GTK_BOX(dumm_page_vbox) , info_label , FALSE , FALSE , 2 );
  }

  gtk_box_pack_start( GTK_BOX(dumm_page_vbox) , content_vbox , TRUE , TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(dumm_page_alignment) , dumm_page_vbox );
}


void i_configure_gui_tablabel_dumm( GtkWidget * dumm_page_alignment ,
                                  gpointer backend_list_p ,
                                  gpointer commit_button )
{
  GtkWidget *pagelabel_vbox, *pagelabel_image, *pagelabel_label;
  GdkPixbuf *pagelabel_image_pix;
  pagelabel_vbox = gtk_vbox_new( FALSE , 1 );
  pagelabel_image_pix = gdk_pixbuf_new_from_xpm_data( (const gchar **)backend_dummy_icon_xpm );
  pagelabel_image = gtk_image_new_from_pixbuf( pagelabel_image_pix ); g_object_unref( pagelabel_image_pix );
  pagelabel_label = gtk_label_new( "" );
  gtk_label_set_markup( GTK_LABEL(pagelabel_label) , "<span size=\"smaller\">Dummy\nbackend</span>" );
  gtk_label_set_justify( GTK_LABEL(pagelabel_label) , GTK_JUSTIFY_CENTER );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_image , FALSE , FALSE , 1 );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_label , FALSE , FALSE , 1 );
  gtk_container_add( GTK_CONTAINER(dumm_page_alignment) , pagelabel_vbox );
  gtk_widget_show_all( dumm_page_alignment );
  return;
}


void i_configure_cfg_dumm_alloc( void )
{
  amidiplug_cfg_dumm_t * dummcfg = g_malloc(sizeof(amidiplug_cfg_dumm_t));
  amidiplug_cfg_backend->dumm = dummcfg;
}


void i_configure_cfg_dumm_free( void )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  g_free( dummcfg->dumm_logger_logfile );
  g_free( dummcfg->dumm_logger_logdir );
}


void i_configure_cfg_dumm_read( pcfg_t * cfgfile )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;
  gchar * def_logfile = g_strjoin( "" , g_get_home_dir() , "/amidi-plug.log" , NULL );
  gchar * def_logdir = (gchar*)g_get_home_dir();

  if (!cfgfile)
  {
    /* fluidsynth backend defaults */
    dummcfg->dumm_logger_enable = 0;
    dummcfg->dumm_logger_lfstyle = 0;
    dummcfg->dumm_playback_speed = 0;
    dummcfg->dumm_logger_logfile = g_strdup( def_logfile );
    dummcfg->dumm_logger_logdir = g_strdup( def_logdir );
  }
  else
  {
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_logger_enable" , &dummcfg->dumm_logger_enable , 0 );
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_logger_lfstyle" , &dummcfg->dumm_logger_lfstyle , 0 );
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_playback_speed" , &dummcfg->dumm_playback_speed , 0 );
    i_pcfg_read_string( cfgfile , "dumm" , "dumm_logger_logfile" , &dummcfg->dumm_logger_logfile , def_logfile );
    i_pcfg_read_string( cfgfile , "dumm" , "dumm_logger_logdir" , &dummcfg->dumm_logger_logdir , def_logdir );
  }

  g_free( def_logfile );
}


void i_configure_cfg_dumm_save( pcfg_t * cfgfile )
{
  amidiplug_cfg_dumm_t * dummcfg = amidiplug_cfg_backend->dumm;

  i_pcfg_write_integer( cfgfile , "dumm" , "dumm_logger_enable" , dummcfg->dumm_logger_enable );
  i_pcfg_write_integer( cfgfile , "dumm" , "dumm_logger_lfstyle" , dummcfg->dumm_logger_lfstyle );
  i_pcfg_write_integer( cfgfile , "dumm" , "dumm_playback_speed" , dummcfg->dumm_playback_speed );
  i_pcfg_write_string( cfgfile , "dumm" , "dumm_logger_logfile" , dummcfg->dumm_logger_logfile );
  i_pcfg_write_string( cfgfile , "dumm" , "dumm_logger_logdir" , dummcfg->dumm_logger_logdir );
}
