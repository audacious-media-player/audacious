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


#include "i_configure-fluidsynth.h"
#include "backend-fluidsynth/b-fluidsynth-config.h"
#include "backend-fluidsynth/backend-fluidsynth-icon.xpm"


void i_configure_ev_toggle_default( GtkToggleButton *togglebutton , gpointer hbox )
{
  if ( gtk_toggle_button_get_active( togglebutton ) )
    gtk_widget_set_sensitive( GTK_WIDGET(hbox) , FALSE );
  else
    gtk_widget_set_sensitive( GTK_WIDGET(hbox) , TRUE );
}


void i_configure_ev_buffertuner_valuechanged( GtkRange *buffertuner_range )
{
  gint bufsize_val, bufmarginsize_val;
  gint i = (gint)gtk_range_get_value( GTK_RANGE(buffertuner_range) );
  GtkWidget *bufsize_spin = g_object_get_data( G_OBJECT(buffertuner_range) , "bufsize_spin" );
  GtkWidget *bufmarginsize_spin = g_object_get_data( G_OBJECT(buffertuner_range) , "bufmarginsize_spin" );

  if ( i < 33 )
  {
    bufsize_val = 256 + (i * 16); /* linear growth of 16i - bufsize_val <= 768 */
    if ( i > 16 )
      bufmarginsize_val = 15 + ((i - 15) / 2); /* linear growth of i/2 */
    else
      bufmarginsize_val = 15; /* do not go below 10 even when bufsize < 512 */
  }
  else if ( i < 42 )
  {
    bufsize_val = 768 + ((i - 33) * 32); /* linear growth of 32i - bufsize_val <= 1024 */
    bufmarginsize_val = 15 + ( (i - 16) / 2 ); /* linear growth of i/2 */
  }
  else
  {
    bufsize_val = 1024 + ( 32 << (i - 42) ); /* exponential growth - bufsize_val > 1024 */
    bufmarginsize_val = 15 + ( (i - 16) / 2 ); /* linear growth of i/2 */
  }

  gtk_spin_button_set_value( GTK_SPIN_BUTTON(bufsize_spin) , bufsize_val );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(bufmarginsize_spin) , bufmarginsize_val );
}


void i_configure_ev_sffile_commit( gpointer sffile_entry )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  g_free( fsyncfg->fsyn_soundfont_file ); /* free previous */
  fsyncfg->fsyn_soundfont_file = g_strdup( gtk_entry_get_text(GTK_ENTRY(sffile_entry)) );
}


void i_configure_ev_sfload_commit( gpointer sfload_radiobt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  GSList *group = gtk_radio_button_get_group( GTK_RADIO_BUTTON(sfload_radiobt) );
  while ( group != NULL )
  {
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(group->data) ) )
      fsyncfg->fsyn_soundfont_load = GPOINTER_TO_INT(g_object_get_data( G_OBJECT(group->data) , "val" ));
    group = group->next;
  }
}


void i_configure_ev_sygain_commit( gpointer gain_spinbt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  if ( GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(gain_spinbt)) )
    fsyncfg->fsyn_synth_gain = (gint)(gtk_spin_button_get_value(GTK_SPIN_BUTTON(gain_spinbt)) * 10);
  else
    fsyncfg->fsyn_synth_gain = -1;
}


void i_configure_ev_sypoly_commit( gpointer poly_spinbt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  if ( GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(poly_spinbt)) )
    fsyncfg->fsyn_synth_poliphony = (gint)(gtk_spin_button_get_value(GTK_SPIN_BUTTON(poly_spinbt)));
  else
    fsyncfg->fsyn_synth_poliphony = -1;
}


void i_configure_ev_syreverb_commit( gpointer reverb_yes_radiobt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  if ( GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(reverb_yes_radiobt)) )
  {
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(reverb_yes_radiobt) ) )
      fsyncfg->fsyn_synth_reverb = 1;
    else
      fsyncfg->fsyn_synth_reverb = 0;
  }
  else
    fsyncfg->fsyn_synth_reverb = -1;
}


void i_configure_ev_sychorus_commit( gpointer chorus_yes_radiobt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  if ( GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(chorus_yes_radiobt)) )
  {
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(chorus_yes_radiobt) ) )
      fsyncfg->fsyn_synth_chorus = 1;
    else
      fsyncfg->fsyn_synth_chorus = 0;
  }
  else
    fsyncfg->fsyn_synth_chorus = -1;
}


void i_configure_ev_sysamplerate_togglecustom( GtkToggleButton *custom_radiobt , gpointer custom_entry )
{
  if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(custom_radiobt) ) )
    gtk_widget_set_sensitive( GTK_WIDGET(custom_entry) , TRUE );
  else
    gtk_widget_set_sensitive( GTK_WIDGET(custom_entry) , FALSE );
}


void i_configure_ev_sysamplerate_commit( gpointer samplerate_custom_radiobt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(samplerate_custom_radiobt)) )
  {
    GtkWidget *customentry = g_object_get_data( G_OBJECT(samplerate_custom_radiobt) , "customentry" );
    gint customvalue = strtol( gtk_entry_get_text(GTK_ENTRY(customentry)) , NULL , 10 );
    if (( customvalue > 22050 ) && ( customvalue < 96000 ))
      fsyncfg->fsyn_synth_samplerate = customvalue;
    else
      fsyncfg->fsyn_synth_samplerate = 44100;
  }
  else
  {
    GSList *group = gtk_radio_button_get_group( GTK_RADIO_BUTTON(samplerate_custom_radiobt) );
    while ( group != NULL )
    {
      if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(group->data) ) )
        fsyncfg->fsyn_synth_samplerate = GPOINTER_TO_INT(g_object_get_data( G_OBJECT(group->data) , "val" ));
      group = group->next;
    }
  }
}


void i_configure_ev_bufsize_commit( gpointer bufsize_spinbt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  fsyncfg->fsyn_buffer_size = (gint)(gtk_spin_button_get_value(GTK_SPIN_BUTTON(bufsize_spinbt)));
}


void i_configure_ev_bufmarginsize_commit( gpointer marginsize_spinbt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  fsyncfg->fsyn_buffer_margin = (gint)(gtk_spin_button_get_value(GTK_SPIN_BUTTON(marginsize_spinbt)));
}


void i_configure_ev_bufmargininc_commit( gpointer margininc_spinbt )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  fsyncfg->fsyn_buffer_increment = (gint)(gtk_spin_button_get_value(GTK_SPIN_BUTTON(margininc_spinbt)));
}


void i_configure_buffertuner_setvalue( GtkWidget * buftuner_hscale , gint bufsize )
{
  gint scale_value = 0;
  if ( bufsize <= 768 )
  {
    scale_value = ( bufsize - 256 ) / 16;
  }
  else if ( bufsize <= 1024 )
  {
    scale_value = ( bufsize + 288 ) / 32;
  }
  else
  {
    gint tcount = 0, tval = bufsize - 1024;
    tval = tval >> 1;
    while ( tval > 0 )
    {
      tval = tval >> 1;
      tcount++;
    }
    scale_value = 37 + tcount;
  }
  if ( scale_value < 0 )
    scale_value = 0;
  else if ( scale_value > 53 )
    scale_value = 53;
  gtk_range_set_value( GTK_RANGE(buftuner_hscale) , scale_value );
}


void i_configure_ev_buffertuner_default( gpointer buffertuner_hscale )
{
  GtkWidget *bufsize_spin = g_object_get_data( G_OBJECT(buffertuner_hscale) , "bufsize_spin" );
  GtkWidget *bufmarginsize_spin = g_object_get_data( G_OBJECT(buffertuner_hscale) , "bufmarginsize_spin" );
  GtkWidget *bufmargininc_spin = g_object_get_data( G_OBJECT(buffertuner_hscale) , "bufmargininc_spin" );
  i_configure_buffertuner_setvalue( GTK_WIDGET(buffertuner_hscale) , 512 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(bufsize_spin) , 512 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(bufmarginsize_spin) , 15 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(bufmargininc_spin) , 18 );
}


void i_configure_gui_tab_fsyn( GtkWidget * fsyn_page_alignment ,
                               gpointer backend_list_p ,
                               gpointer commit_button )
{
  GtkWidget *fsyn_page_vbox;
  GtkWidget *title_widget;
  GtkWidget *content_vbox; /* this vbox will contain the various frames for config sections */
  GSList * backend_list = backend_list_p;
  gboolean fsyn_module_ok = FALSE;
  gchar * fsyn_module_pathfilename;

  fsyn_page_vbox = gtk_vbox_new( FALSE , 0 );

  title_widget = i_configure_gui_draw_title( _("FLUIDSYNTH BACKEND CONFIGURATION") );
  gtk_box_pack_start( GTK_BOX(fsyn_page_vbox) , title_widget , FALSE , FALSE , 2 );

  content_vbox = gtk_vbox_new( FALSE , 2 );

  /* check if the FluidSynth module is available */
  while ( backend_list != NULL )
  {
    amidiplug_sequencer_backend_name_t * mn = backend_list->data;
    if ( !strcmp( mn->name , "fluidsynth" ) )
    {
      fsyn_module_ok = TRUE;
      fsyn_module_pathfilename = mn->filename;
      break;
    }
    backend_list = backend_list->next;
  }

  if ( fsyn_module_ok )
  {
    GtkWidget *soundfont_frame, *soundfont_vbox;
    GtkWidget *soundfont_file_label, *soundfont_file_entry, *soundfont_file_bbutton, *soundfont_file_hbox;
    GtkWidget *soundfont_load_hsep, *soundfont_load_vbox, *soundfont_load_option[2];
    GtkWidget *synth_frame, *synth_hbox, *synth_leftcol_vbox, *synth_rightcol_vbox;
    GtkWidget *synth_samplerate_frame, *synth_samplerate_vbox, *synth_samplerate_option[4];
    GtkWidget *synth_samplerate_optionhbox, *synth_samplerate_optionentry, *synth_samplerate_optionlabel;
    GtkWidget *synth_gain_frame, *synth_gain_hbox, *synth_gain_value_hbox;
    GtkWidget *synth_gain_value_label, *synth_gain_value_spin, *synth_gain_defcheckbt;
    GtkWidget *synth_poly_frame, *synth_poly_hbox, *synth_poly_value_hbox;
    GtkWidget *synth_poly_value_label, *synth_poly_value_spin, *synth_poly_defcheckbt;
    GtkWidget *synth_reverb_frame, *synth_reverb_hbox, *synth_reverb_value_hbox;
    GtkWidget *synth_reverb_value_option[2], *synth_reverb_defcheckbt;
    GtkWidget *synth_chorus_frame, *synth_chorus_hbox, *synth_chorus_value_hbox;
    GtkWidget *synth_chorus_value_option[2], *synth_chorus_defcheckbt;
    GtkWidget *buffer_frame, *buffer_table , *buffer_vsep[4];
    GtkWidget *buffer_tuner_label, *buffer_tuner_hscale;
    GtkWidget *buffer_tuner_defbt, *buffer_tuner_defbt_label;
    GtkWidget *buffer_bufsize_label, *buffer_bufsize_spin;
    GtkWidget *buffer_marginsize_label, *buffer_marginsize_spin;
    GtkWidget *buffer_margininc_label, *buffer_margininc_spin;
    GtkTooltips *tips;

    amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;

    tips = gtk_tooltips_new();
    g_object_set_data_full( G_OBJECT(fsyn_page_alignment) , "tt" , tips , g_object_unref );

    /* soundfont settings */
    soundfont_frame = gtk_frame_new( _("SoundFont settings") );
    soundfont_vbox = gtk_vbox_new( FALSE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(soundfont_vbox), 4 );
    gtk_container_add( GTK_CONTAINER(soundfont_frame) , soundfont_vbox );
    /* soundfont settings - file */
    soundfont_file_hbox = gtk_hbox_new( FALSE , 2 );
    soundfont_file_label = gtk_label_new( _("SoundFont filename:") );
    soundfont_file_entry = gtk_entry_new();
    g_object_set_data( G_OBJECT(soundfont_file_entry) , "fc-act" ,
                       GINT_TO_POINTER(GTK_FILE_CHOOSER_ACTION_OPEN) );
    gtk_entry_set_text( GTK_ENTRY(soundfont_file_entry) , fsyncfg->fsyn_soundfont_file );
    soundfont_file_bbutton = gtk_button_new_with_label( _("browse") );
    g_signal_connect_swapped( G_OBJECT(soundfont_file_bbutton) , "clicked" ,
                              G_CALLBACK(i_configure_ev_browse_for_entry) , soundfont_file_entry );
    gtk_box_pack_start( GTK_BOX(soundfont_file_hbox) , soundfont_file_label , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(soundfont_file_hbox) , soundfont_file_entry , TRUE , TRUE , 0 );
    gtk_box_pack_start( GTK_BOX(soundfont_file_hbox) , soundfont_file_bbutton , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(soundfont_vbox) , soundfont_file_hbox , FALSE , FALSE , 0 );
    /* soundfont settings - load */
    soundfont_load_hsep = gtk_hseparator_new();
    soundfont_load_vbox = gtk_vbox_new( FALSE , 0 );
    soundfont_load_option[0] = gtk_radio_button_new_with_label( NULL ,
                                 _("Load SoundFont on player start") );
    g_object_set_data( G_OBJECT(soundfont_load_option[0]) , "val" , GINT_TO_POINTER(0) );
    soundfont_load_option[1] = gtk_radio_button_new_with_label_from_widget(
                                 GTK_RADIO_BUTTON(soundfont_load_option[0]) ,
                                 _("Load SoundFont on first midifile play") );
    g_object_set_data( G_OBJECT(soundfont_load_option[1]) , "val" , GINT_TO_POINTER(1) );
    if ( fsyncfg->fsyn_soundfont_load == 0 )
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(soundfont_load_option[0]) , TRUE );
    else
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(soundfont_load_option[1]) , TRUE );
    gtk_box_pack_start( GTK_BOX(soundfont_load_vbox) , soundfont_load_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(soundfont_load_vbox) , soundfont_load_option[1] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(soundfont_vbox) , soundfont_load_hsep , FALSE , FALSE , 3 );
    gtk_box_pack_start( GTK_BOX(soundfont_vbox) , soundfont_load_vbox , FALSE , FALSE , 0 );

    gtk_box_pack_start( GTK_BOX(content_vbox) , soundfont_frame , FALSE , FALSE , 0 );

    /* synth settings */
    synth_frame = gtk_frame_new( _("Synthesizer settings") );
    synth_hbox = gtk_hbox_new( FALSE , 4 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_hbox), 4 );
    gtk_container_add( GTK_CONTAINER(synth_frame) , synth_hbox );
    synth_leftcol_vbox = gtk_vbox_new( TRUE , 0 );
    synth_rightcol_vbox = gtk_vbox_new( FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_hbox) , synth_leftcol_vbox , TRUE , TRUE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_hbox) , synth_rightcol_vbox , FALSE , FALSE , 0 );
    /* synth settings - gain */
    synth_gain_frame = gtk_frame_new( _("gain") );
    gtk_frame_set_label_align( GTK_FRAME(synth_gain_frame) , 0.5 , 0.5 );
    gtk_box_pack_start( GTK_BOX(synth_leftcol_vbox) , synth_gain_frame , TRUE , TRUE , 0 );
    synth_gain_hbox = gtk_hbox_new( TRUE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_gain_hbox), 2 );
    gtk_container_add( GTK_CONTAINER(synth_gain_frame) , synth_gain_hbox );
    synth_gain_defcheckbt = gtk_check_button_new_with_label( _("use default") );
    gtk_box_pack_start( GTK_BOX(synth_gain_hbox) , synth_gain_defcheckbt , FALSE , FALSE , 0 );
    synth_gain_value_hbox = gtk_hbox_new( FALSE , 4 );
    synth_gain_value_label = gtk_label_new( _("value:") );
    synth_gain_value_spin = gtk_spin_button_new_with_range( 0.0 , 10.0 , 0.1 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(synth_gain_value_spin) , 0.2 );
    g_signal_connect( G_OBJECT(synth_gain_defcheckbt) , "toggled" ,
                      G_CALLBACK(i_configure_ev_toggle_default) , synth_gain_value_hbox );
    if ( fsyncfg->fsyn_synth_gain < 0 )
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_gain_defcheckbt) , TRUE );
    }
    else
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_gain_defcheckbt) , FALSE );
      gtk_spin_button_set_value( GTK_SPIN_BUTTON(synth_gain_value_spin) ,
                                 (gdouble)fsyncfg->fsyn_synth_gain / 10 );
    }
    gtk_box_pack_start( GTK_BOX(synth_gain_hbox) , synth_gain_value_hbox , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_gain_value_hbox) , synth_gain_value_label , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_gain_value_hbox) , synth_gain_value_spin , FALSE , FALSE , 0 );
    /* synth settings - poliphony */
    synth_poly_frame = gtk_frame_new( _("poliphony") );
    gtk_frame_set_label_align( GTK_FRAME(synth_poly_frame) , 0.5 , 0.5 );
    gtk_box_pack_start( GTK_BOX(synth_leftcol_vbox) , synth_poly_frame , TRUE , TRUE , 0 );
    synth_poly_hbox = gtk_hbox_new( TRUE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_poly_hbox), 2 );
    gtk_container_add( GTK_CONTAINER(synth_poly_frame) , synth_poly_hbox );
    synth_poly_defcheckbt = gtk_check_button_new_with_label( _("use default") );
    gtk_box_pack_start( GTK_BOX(synth_poly_hbox) , synth_poly_defcheckbt , FALSE , FALSE , 0 );
    synth_poly_value_hbox = gtk_hbox_new( FALSE , 4 );
    synth_poly_value_label = gtk_label_new( _("value:") );
    synth_poly_value_spin = gtk_spin_button_new_with_range( 16 , 4096 , 1 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(synth_poly_value_spin) , 256 );
    g_signal_connect( G_OBJECT(synth_poly_defcheckbt) , "toggled" ,
                      G_CALLBACK(i_configure_ev_toggle_default) , synth_poly_value_hbox );
    if ( fsyncfg->fsyn_synth_poliphony < 0 )
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_poly_defcheckbt) , TRUE );
    }
    else
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_poly_defcheckbt) , FALSE );
      gtk_spin_button_set_value( GTK_SPIN_BUTTON(synth_poly_value_spin) ,
                                 (gdouble)fsyncfg->fsyn_synth_poliphony );
    }
    gtk_box_pack_start( GTK_BOX(synth_poly_hbox) , synth_poly_value_hbox , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_poly_value_hbox) , synth_poly_value_label , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_poly_value_hbox) , synth_poly_value_spin , FALSE , FALSE , 0 );
    /* synth settings - reverb */
    synth_reverb_frame = gtk_frame_new( _("reverb") );
    gtk_frame_set_label_align( GTK_FRAME(synth_reverb_frame) , 0.5 , 0.5 );
    gtk_box_pack_start( GTK_BOX(synth_leftcol_vbox) , synth_reverb_frame , TRUE , TRUE , 0 );
    synth_reverb_hbox = gtk_hbox_new( TRUE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_reverb_hbox), 2 );
    gtk_container_add( GTK_CONTAINER(synth_reverb_frame) , synth_reverb_hbox );
    synth_reverb_defcheckbt = gtk_check_button_new_with_label( _("use default") );
    gtk_box_pack_start( GTK_BOX(synth_reverb_hbox) , synth_reverb_defcheckbt , FALSE , FALSE , 0 );
    synth_reverb_value_hbox = gtk_hbox_new( TRUE , 4 );
    synth_reverb_value_option[0] = gtk_radio_button_new_with_label( NULL , _("yes") );
    synth_reverb_value_option[1] = gtk_radio_button_new_with_label_from_widget(
                                     GTK_RADIO_BUTTON(synth_reverb_value_option[0]) , _("no") );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_reverb_value_option[0]) , TRUE );
    g_signal_connect( G_OBJECT(synth_reverb_defcheckbt) , "toggled" ,
                      G_CALLBACK(i_configure_ev_toggle_default) , synth_reverb_value_hbox );
    if ( fsyncfg->fsyn_synth_reverb < 0 )
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_reverb_defcheckbt) , TRUE );
    }
    else
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_reverb_defcheckbt) , FALSE );
      if ( fsyncfg->fsyn_synth_reverb == 0 )
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_reverb_value_option[1]) , TRUE );
      else
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_reverb_value_option[0]) , TRUE );
    }
    gtk_box_pack_start( GTK_BOX(synth_reverb_hbox) , synth_reverb_value_hbox , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_reverb_value_hbox) , synth_reverb_value_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_reverb_value_hbox) , synth_reverb_value_option[1] , FALSE , FALSE , 0 );
    /* synth settings - chorus */
    synth_chorus_frame = gtk_frame_new( _("chorus") );
    gtk_frame_set_label_align( GTK_FRAME(synth_chorus_frame) , 0.5 , 0.5 );
    gtk_box_pack_start( GTK_BOX(synth_leftcol_vbox) , synth_chorus_frame , TRUE , TRUE , 0 );
    synth_chorus_hbox = gtk_hbox_new( TRUE , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_chorus_hbox), 2 );
    gtk_container_add( GTK_CONTAINER(synth_chorus_frame) , synth_chorus_hbox );
    synth_chorus_defcheckbt = gtk_check_button_new_with_label( _("use default") );
    gtk_box_pack_start( GTK_BOX(synth_chorus_hbox) , synth_chorus_defcheckbt , FALSE , FALSE , 0 );
    synth_chorus_value_hbox = gtk_hbox_new( TRUE , 4 );
    synth_chorus_value_option[0] = gtk_radio_button_new_with_label( NULL , _("yes") );
    synth_chorus_value_option[1] = gtk_radio_button_new_with_label_from_widget(
                                     GTK_RADIO_BUTTON(synth_chorus_value_option[0]) , _("no") );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_chorus_value_option[0]) , TRUE );
    g_signal_connect( G_OBJECT(synth_chorus_defcheckbt) , "toggled" ,
                      G_CALLBACK(i_configure_ev_toggle_default) , synth_chorus_value_hbox );
    if ( fsyncfg->fsyn_synth_chorus < 0 )
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_chorus_defcheckbt) , TRUE );
    }
    else
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_chorus_defcheckbt) , FALSE );
      if ( fsyncfg->fsyn_synth_chorus == 0 )
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_chorus_value_option[1]) , TRUE );
      else
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_chorus_value_option[0]) , TRUE );
    }
    gtk_box_pack_start( GTK_BOX(synth_chorus_hbox) , synth_chorus_value_hbox , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_chorus_value_hbox) , synth_chorus_value_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_chorus_value_hbox) , synth_chorus_value_option[1] , FALSE , FALSE , 0 );
    /* synth settings - samplerate */
    synth_samplerate_frame = gtk_frame_new( _("sample rate") );
    gtk_frame_set_label_align( GTK_FRAME(synth_samplerate_frame) , 0.5 , 0.5 );
    synth_samplerate_vbox = gtk_vbox_new( TRUE , 0 );
    gtk_container_set_border_width( GTK_CONTAINER(synth_samplerate_vbox), 6 );
    gtk_container_add( GTK_CONTAINER(synth_samplerate_frame) , synth_samplerate_vbox );
    gtk_box_pack_start( GTK_BOX(synth_rightcol_vbox) , synth_samplerate_frame , FALSE , FALSE , 0 );
    synth_samplerate_option[0] = gtk_radio_button_new_with_label( NULL , "22050 Hz " );
    g_object_set_data( G_OBJECT(synth_samplerate_option[0]) , "val" , GINT_TO_POINTER(22050) );
    synth_samplerate_option[1] = gtk_radio_button_new_with_label_from_widget(
                                   GTK_RADIO_BUTTON(synth_samplerate_option[0]) , "44100 Hz " );
    g_object_set_data( G_OBJECT(synth_samplerate_option[1]) , "val" , GINT_TO_POINTER(44100) );
    synth_samplerate_option[2] = gtk_radio_button_new_with_label_from_widget(
                                   GTK_RADIO_BUTTON(synth_samplerate_option[0]) , "96000 Hz " );
    g_object_set_data( G_OBJECT(synth_samplerate_option[2]) , "val" , GINT_TO_POINTER(96000) );
    synth_samplerate_option[3] = gtk_radio_button_new_with_label_from_widget(
                                   GTK_RADIO_BUTTON(synth_samplerate_option[0]) , _("custom ") );
    synth_samplerate_optionhbox = gtk_hbox_new( FALSE , 4 );
    synth_samplerate_optionentry = gtk_entry_new();
    gtk_widget_set_sensitive( GTK_WIDGET(synth_samplerate_optionentry) , FALSE );
    gtk_entry_set_width_chars( GTK_ENTRY(synth_samplerate_optionentry) , 8 );
    gtk_entry_set_max_length( GTK_ENTRY(synth_samplerate_optionentry) , 5 );
    g_object_set_data( G_OBJECT(synth_samplerate_option[3]) , "customentry" , synth_samplerate_optionentry );
    g_signal_connect( G_OBJECT(synth_samplerate_option[3]) , "toggled" ,
                      G_CALLBACK(i_configure_ev_sysamplerate_togglecustom) , synth_samplerate_optionentry );
    synth_samplerate_optionlabel = gtk_label_new( "Hz " );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_optionhbox) , synth_samplerate_optionentry , TRUE , TRUE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_optionhbox) , synth_samplerate_optionlabel , FALSE , FALSE , 0 );
    switch ( fsyncfg->fsyn_synth_samplerate )
    {
      case 22050:
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_samplerate_option[0]) , TRUE );
        break;
      case 44100:
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_samplerate_option[1]) , TRUE );
        break;
      case 96000:
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_samplerate_option[2]) , TRUE );
        break;
      default:
        if (( fsyncfg->fsyn_synth_samplerate > 22050 ) && ( fsyncfg->fsyn_synth_samplerate < 96000 ))
        {
          gchar *samplerate_value = g_strdup_printf( "%i" , fsyncfg->fsyn_synth_samplerate );
          gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_samplerate_option[3]) , TRUE );
          gtk_entry_set_text( GTK_ENTRY(synth_samplerate_optionentry) , samplerate_value );
          g_free( samplerate_value );
        }
        else
          gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(synth_samplerate_option[1]) , TRUE );
    }
    gtk_box_pack_start( GTK_BOX(synth_samplerate_vbox) , synth_samplerate_option[0] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_vbox) , synth_samplerate_option[1] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_vbox) , synth_samplerate_option[2] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_vbox) , synth_samplerate_option[3] , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(synth_samplerate_vbox) , synth_samplerate_optionhbox , FALSE , FALSE , 0 );

    gtk_box_pack_start( GTK_BOX(content_vbox) , synth_frame , TRUE , TRUE , 0 );

    /* buffer settings */
    buffer_frame = gtk_frame_new( _("Buffer settings") );
    buffer_table = gtk_table_new( 2 , 9 , FALSE );
    gtk_table_set_col_spacings( GTK_TABLE(buffer_table) , 2 );
    gtk_container_set_border_width( GTK_CONTAINER(buffer_table), 4 );
    gtk_container_add( GTK_CONTAINER(buffer_frame) , buffer_table );
    /* buffer settings - slider */
    buffer_tuner_defbt = gtk_button_new();
    buffer_tuner_defbt_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(buffer_tuner_defbt_label) ,
                          _("<span size=\"smaller\">def</span>") );
    gtk_container_add( GTK_CONTAINER(buffer_tuner_defbt) , buffer_tuner_defbt_label );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_tuner_defbt , 0 , 1 , 0 , 2 ,
                      0 , GTK_FILL , 2 , 0 );
    buffer_vsep[0] = gtk_vseparator_new();
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_vsep[0] , 1 , 2 , 0 , 2 ,
                      0 , GTK_FILL , 0 , 0 );
    buffer_tuner_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(buffer_tuner_label) ,
                          _("<span size=\"smaller\">handy buffer tuner</span>") );
    buffer_tuner_hscale = gtk_hscale_new_with_range( 0 , 53 , 1 );
    gtk_scale_set_draw_value( GTK_SCALE(buffer_tuner_hscale) , FALSE );
    i_configure_buffertuner_setvalue( buffer_tuner_hscale , fsyncfg->fsyn_buffer_size );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_tuner_label , 2 , 3 , 0 , 1 ,
                      GTK_EXPAND | GTK_FILL , GTK_EXPAND | GTK_FILL , 0 , 0 );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_tuner_hscale , 2 , 3 , 1 , 2 ,
                      GTK_EXPAND | GTK_FILL , GTK_EXPAND | GTK_FILL , 0 , 0 );
    buffer_vsep[1] = gtk_vseparator_new();
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_vsep[1] , 3 , 4 , 0 , 2 ,
                      0 , GTK_FILL , 0 , 0 );
    /* buffer settings - size */
    buffer_bufsize_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(buffer_bufsize_label) ,
                          _("<span size=\"smaller\">size</span>") );
    buffer_bufsize_spin = gtk_spin_button_new_with_range( 100 , 99999 , 20 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(buffer_bufsize_spin) ,
                               (gdouble)fsyncfg->fsyn_buffer_size );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_bufsize_label , 4 , 5 , 0 , 1 ,
                      GTK_FILL , 0 , 1 , 1 );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_bufsize_spin , 4 , 5 , 1 , 2 ,
                      0 , 0 , 1 , 1 );
    buffer_vsep[2] = gtk_vseparator_new();
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_vsep[2] , 5 , 6 , 0 , 2 ,
                      0 , GTK_FILL , 0 , 0 );
    /* buffer settings - margin */
    buffer_marginsize_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(buffer_marginsize_label) ,
                          _("<span size=\"smaller\">margin</span>") );
    buffer_marginsize_spin = gtk_spin_button_new_with_range( 0 , 100 , 1 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(buffer_marginsize_spin) ,
                               (gdouble)fsyncfg->fsyn_buffer_margin );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_marginsize_label , 6 , 7 , 0 , 1 ,
                      GTK_FILL , 0 , 1 , 1 );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_marginsize_spin , 6 , 7 , 1 , 2 ,
                      0 , 0 , 1 , 1 );
    buffer_vsep[3] = gtk_vseparator_new();
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_vsep[3] , 7 , 8 , 0 , 2 ,
                      0 , GTK_FILL , 0 , 0 );
    /* buffer settings - increment */
    buffer_margininc_label = gtk_label_new( "" );
    gtk_label_set_markup( GTK_LABEL(buffer_margininc_label) ,
                          _("<span size=\"smaller\">increment</span>") );
    buffer_margininc_spin = gtk_spin_button_new_with_range( 6 , 1000 , 1 );
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(buffer_margininc_spin) ,
                               (gdouble)fsyncfg->fsyn_buffer_increment );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_margininc_label , 8 , 9 , 0 , 1 ,
                      GTK_FILL , 0 , 1 , 1 );
    gtk_table_attach( GTK_TABLE(buffer_table) , buffer_margininc_spin , 8 , 9 , 1 , 2 ,
                      0 , 0 , 1 , 1 );

    g_object_set_data( G_OBJECT(buffer_tuner_hscale) , "bufsize_spin" , buffer_bufsize_spin );
    g_object_set_data( G_OBJECT(buffer_tuner_hscale) , "bufmarginsize_spin" , buffer_marginsize_spin );
    g_object_set_data( G_OBJECT(buffer_tuner_hscale) , "bufmargininc_spin" , buffer_margininc_spin );
    g_signal_connect_swapped( G_OBJECT(buffer_tuner_defbt) , "clicked" ,
                              G_CALLBACK(i_configure_ev_buffertuner_default) , buffer_tuner_hscale );
    g_signal_connect( G_OBJECT(buffer_tuner_hscale) , "value-changed" ,
                      G_CALLBACK(i_configure_ev_buffertuner_valuechanged) , NULL );

    gtk_box_pack_start( GTK_BOX(content_vbox) , buffer_frame , FALSE , FALSE , 0 );

    /* commit events */
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sffile_commit) , soundfont_file_entry );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sfload_commit) , soundfont_load_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sygain_commit) , synth_gain_value_spin );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sypoly_commit) , synth_poly_value_spin );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_syreverb_commit) , synth_reverb_value_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sychorus_commit) , synth_chorus_value_option[0] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_sysamplerate_commit) , synth_samplerate_option[3] );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_bufsize_commit) , buffer_bufsize_spin );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_bufmarginsize_commit) , buffer_marginsize_spin );
    g_signal_connect_swapped( G_OBJECT(commit_button) , "clicked" ,
                              G_CALLBACK(i_configure_ev_bufmargininc_commit) , buffer_margininc_spin );

    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , soundfont_file_entry ,
                          _("* Select SoundFont *\n"
                          "In order to play MIDI with FluidSynth, you need to specify a "
                          "valid SoundFont file here (use absolute paths).") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , soundfont_load_option[0] ,
                          _("* Load SoundFont on player start *\n"
                          "Depending on your system speed, SoundFont loading in FluidSynth will "
                          "require up to a few seconds. This is a one-time task (the soundfont "
                          "will stay loaded until it is changed or the backend is unloaded) that "
                          "can be done at player start, or before the first MIDI file is played "
                          "(the latter is a better choice if you don't use your player to listen "
                          "MIDI files only).") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , soundfont_load_option[1] ,
                          _("* Load SoundFont on first midifile play *\n"
                          "Depending on your system speed, SoundFont loading in FluidSynth will "
                          "require up to a few seconds. This is a one-time task (the soundfont "
                          "will stay loaded until it is changed or the backend is unloaded) that "
                          "can be done at player start, or before the first MIDI file is played "
                          "(the latter is a better choice if you don't use your player to listen "
                          "MIDI files only).") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_gain_value_spin ,
                          _("* Synthesizer gain *\n"
                          "From FluidSynth docs: the gain is applied to the final or master output "
                          "of the synthesizer; it is set to a low value by default to avoid the "
                          "saturation of the output when random MIDI files are played.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_poly_value_spin ,
                          _("* Synthesizer polyphony *\n"
                          "From FluidSynth docs: the polyphony defines how many voices can be played "
                          "in parallel; the number of voices is not necessarily equivalent to the "
                          "number of notes played simultaneously; indeed, when a note is struck on a "
                          "specific MIDI channel, the preset on that channel may create several voices, "
                          "for example, one for the left audio channel and one for the right audio "
                          "channels; the number of voices activated depends on the number of instrument "
                          "zones that fall in the correspond to the velocity and key of the played "
                          "note.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_reverb_value_option[0] ,
                          _("* Synthesizer reverb *\n"
                          "From FluidSynth docs: when set to \"yes\" the reverb effects module is "
                          "activated; note that when the reverb module is active, the amount of "
                          "signal sent to the reverb module depends on the \"reverb send\" generator "
                          "defined in the SoundFont.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_reverb_value_option[1] ,
                          _("* Synthesizer reverb *\n"
                          "From FluidSynth docs: when set to \"yes\" the reverb effects module is "
                          "activated; note that when the reverb module is active, the amount of "
                          "signal sent to the reverb module depends on the \"reverb send\" generator "
                          "defined in the SoundFont.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_chorus_value_option[0] ,
                          _("* Synthesizer chorus *\n"
                          "From FluidSynth docs: when set to \"yes\" the chorus effects module is "
                          "activated; note that when the chorus module is active, the amount of "
                          "signal sent to the chorus module depends on the \"chorus send\" generator "
                          "defined in the SoundFont.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_chorus_value_option[1] ,
                          _("* Synthesizer chorus *\n"
                          "From FluidSynth docs: when set to \"yes\" the chorus effects module is "
                          "activated; note that when the chorus module is active, the amount of "
                          "signal sent to the chorus module depends on the \"chorus send\" generator "
                          "defined in the SoundFont.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_samplerate_option[0] ,
                          _("* Synthesizer samplerate *\n"
                          "The sample rate of the audio generated by the synthesizer. You can also specify "
                          "a custom value in the interval 22050Hz-96000Hz.\n"
                          "NOTE: the default buffer parameters are tuned for 44100Hz; changing the sample "
                          "rate may require buffer params tuning to obtain good sound quality.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_samplerate_option[1] ,
                          _("* Synthesizer samplerate *\n"
                          "The sample rate of the audio generated by the synthesizer. You can also specify "
                          "a custom value in the interval 22050Hz-96000Hz.\n"
                          "NOTE: the default buffer parameters are tuned for 44100Hz; changing the sample "
                          "rate may require buffer params tuning to obtain good sound quality.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_samplerate_option[2] ,
                          _("* Synthesizer samplerate *\n"
                          "The sample rate of the audio generated by the synthesizer. You can also specify "
                          "a custom value in the interval 22050Hz-96000Hz.\n"
                          "NOTE: the default buffer parameters are tuned for 44100Hz; changing the sample "
                          "rate may require buffer params tuning to obtain good sound quality.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , synth_samplerate_option[3] ,
                          _("* Synthesizer samplerate *\n"
                          "The sample rate of the audio generated by the synthesizer. You can also specify "
                          "a custom value in the interval 22050Hz-96000Hz.\n"
                          "NOTE: the default buffer parameters are tuned for 44100Hz; changing the sample "
                          "rate may require buffer params tuning to obtain good sound quality.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , buffer_tuner_defbt ,
                          _("* FluidSynth backend buffer *\n"
                          "This button resets the backend buffer parameters to default values.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , buffer_tuner_hscale ,
                          _("* FluidSynth backend buffer *\n"
                          "If you notice skips during song playback and your system is not performing "
                          "any cpu-intensive task (except FluidSynth itself), you may want to tune the "
                          "buffer in order to prevent skipping. Try to move the \"handy buffer tuner\" "
                          "a single step to the right until playback is fluid again.") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , buffer_bufsize_spin ,
                          _("* FluidSynth backend buffer *\n"
                          "It is a good idea to make buffer adjustments with the \"handy buffer tuner\" "
                          "before resorting to manual editing of buffer parameters.\n"
                          "However, if you want to fine-tune something and want to know what you're doing, "
                          "you can understand how these parameters work by reading the backend code "
                          "(b-fluidsynth.c). In short words, every amount of time "
                          "(proportional to buffer_SIZE and sample rate), right before gathering samples, "
                          "the buffer is resized as follows:\n"
                          "buffer_SIZE + buffer_MARGIN + extramargin\nwhere extramargin is a value "
                          "computed as number_of_seconds_of_playback / margin_INCREMENT .") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , buffer_marginsize_spin ,
                          _("* FluidSynth backend buffer *\n"
                          "It is a good idea to make buffer adjustments with the \"handy buffer tuner\" "
                          "before resorting to manual editing of buffer parameters.\n"
                          "However, if you want to fine-tune something and want to know what you're doing, "
                          "you can understand how these parameters work by reading the backend code "
                          "(b-fluidsynth.c). In short words, every amount of time "
                          "(proportional to buffer_SIZE and sample rate), right before gathering samples, "
                          "the buffer is resized as follows:\n"
                          "buffer_SIZE + buffer_MARGIN + extramargin\nwhere extramargin is a value "
                          "computed as number_of_seconds_of_playback / margin_INCREMENT .") , "" );
    gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , buffer_margininc_spin ,
                          _("* FluidSynth backend buffer *\n"
                          "It is a good idea to make buffer adjustments with the \"handy buffer tuner\" "
                          "before resorting to manual editing of buffer parameters.\n"
                          "However, if you want to fine-tune something and want to know what you're doing, "
                          "you can understand how these parameters work by reading the backend code "
                          "(b-fluidsynth.c). In short words, every amount of time "
                          "(proportional to buffer_SIZE and sample rate), right before gathering samples, "
                          "the buffer is resized as follows:\n"
                          "buffer_SIZE + buffer_MARGIN + extramargin\nwhere extramargin is a value "
                          "computed as number_of_seconds_of_playback / margin_INCREMENT .") , "" );
  }
  else
  {
    /* display "not available" information */
    GtkWidget * info_label;
    info_label = gtk_label_new( _("FluidSynth Backend not loaded or not available") );
    gtk_box_pack_start( GTK_BOX(fsyn_page_vbox) , info_label , FALSE , FALSE , 2 );
  }

  gtk_box_pack_start( GTK_BOX(fsyn_page_vbox) , content_vbox , TRUE , TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(fsyn_page_alignment) , fsyn_page_vbox );
}


void i_configure_gui_tablabel_fsyn( GtkWidget * fsyn_page_alignment ,
                                  gpointer backend_list_p ,
                                  gpointer commit_button )
{
  GtkWidget *pagelabel_vbox, *pagelabel_image, *pagelabel_label;
  GdkPixbuf *pagelabel_image_pix;
  pagelabel_vbox = gtk_vbox_new( FALSE , 1 );
  pagelabel_image_pix = gdk_pixbuf_new_from_xpm_data( (const gchar **)backend_fluidsynth_icon_xpm );
  pagelabel_image = gtk_image_new_from_pixbuf( pagelabel_image_pix ); g_object_unref( pagelabel_image_pix );
  pagelabel_label = gtk_label_new( "" );
  gtk_label_set_markup( GTK_LABEL(pagelabel_label) , "<span size=\"smaller\">FluidSynth\nbackend</span>" );
  gtk_label_set_justify( GTK_LABEL(pagelabel_label) , GTK_JUSTIFY_CENTER );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_image , FALSE , FALSE , 1 );
  gtk_box_pack_start( GTK_BOX(pagelabel_vbox) , pagelabel_label , FALSE , FALSE , 1 );
  gtk_container_add( GTK_CONTAINER(fsyn_page_alignment) , pagelabel_vbox );
  gtk_widget_show_all( fsyn_page_alignment );
  return;
}


void i_configure_cfg_fsyn_alloc( void )
{
  amidiplug_cfg_fsyn_t * fsyncfg = g_malloc(sizeof(amidiplug_cfg_fsyn_t));
  amidiplug_cfg_backend->fsyn = fsyncfg;
}


void i_configure_cfg_fsyn_free( void )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;
  g_free( fsyncfg->fsyn_soundfont_file );
}


void i_configure_cfg_fsyn_read( pcfg_t * cfgfile )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;

  if (!cfgfile)
  {
    /* fluidsynth backend defaults */
    fsyncfg->fsyn_soundfont_file = g_strdup( "" );
    fsyncfg->fsyn_soundfont_load = 1;
    fsyncfg->fsyn_synth_samplerate = 44100;
    fsyncfg->fsyn_synth_gain = -1;
    fsyncfg->fsyn_synth_poliphony = -1;
    fsyncfg->fsyn_synth_reverb = -1;
    fsyncfg->fsyn_synth_chorus = -1;
    fsyncfg->fsyn_buffer_size = 512;
    fsyncfg->fsyn_buffer_margin = 10;
    fsyncfg->fsyn_buffer_increment = 18;
  }
  else
  {
    i_pcfg_read_string( cfgfile , "fsyn" , "fsyn_soundfont_file" , &fsyncfg->fsyn_soundfont_file , "" );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_soundfont_load" , &fsyncfg->fsyn_soundfont_load , 1 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_samplerate" , &fsyncfg->fsyn_synth_samplerate , 44100 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_gain" , &fsyncfg->fsyn_synth_gain , -1 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_poliphony" , &fsyncfg->fsyn_synth_poliphony , -1 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_reverb" , &fsyncfg->fsyn_synth_reverb , -1 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_chorus" , &fsyncfg->fsyn_synth_chorus , -1 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_size" , &fsyncfg->fsyn_buffer_size , 512 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_margin" , &fsyncfg->fsyn_buffer_margin , 15 );
    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_increment" , &fsyncfg->fsyn_buffer_increment , 18 );
  }
}


void i_configure_cfg_fsyn_save( pcfg_t * cfgfile )
{
  amidiplug_cfg_fsyn_t * fsyncfg = amidiplug_cfg_backend->fsyn;

  i_pcfg_write_string( cfgfile , "fsyn" , "fsyn_soundfont_file" , fsyncfg->fsyn_soundfont_file );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_soundfont_load" , fsyncfg->fsyn_soundfont_load );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_synth_samplerate" , fsyncfg->fsyn_synth_samplerate );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_synth_gain" , fsyncfg->fsyn_synth_gain );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_synth_poliphony" , fsyncfg->fsyn_synth_poliphony );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_synth_reverb" , fsyncfg->fsyn_synth_reverb );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_synth_chorus" , fsyncfg->fsyn_synth_chorus );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_buffer_size" , fsyncfg->fsyn_buffer_size );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_buffer_margin" , fsyncfg->fsyn_buffer_margin );
  i_pcfg_write_integer( cfgfile , "fsyn" , "fsyn_buffer_increment" , fsyncfg->fsyn_buffer_increment );
}
