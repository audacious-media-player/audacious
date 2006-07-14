/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 *
 * Libconsole preferences GUI by Giacomo Lozito
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "libaudacious/configdb.h"
#include "Audacious_Config.h"


void console_cfg_load( void )
{
	ConfigDb *db = bmp_cfg_db_open();
	bmp_cfg_db_get_int(db, "console", "loop_length", &audcfg.loop_length);
	bmp_cfg_db_get_bool(db, "console", "resample", &audcfg.resample);
	bmp_cfg_db_get_int(db, "console", "resample_rate", &audcfg.resample_rate);
	bmp_cfg_db_get_bool(db, "console", "nsfe_playlist", &audcfg.nsfe_playlist);
	bmp_cfg_db_get_int(db, "console", "treble", &audcfg.treble);
	bmp_cfg_db_get_int(db, "console", "bass", &audcfg.bass);
	bmp_cfg_db_get_bool(db, "console", "ignore_spc_length", &audcfg.ignore_spc_length);
	bmp_cfg_db_close(db);
}


void console_cfg_save( void )
{
	ConfigDb *db = bmp_cfg_db_open();
	bmp_cfg_db_set_int(db, "console", "loop_length", audcfg.loop_length);
	bmp_cfg_db_set_bool(db, "console", "resample", audcfg.resample);
	bmp_cfg_db_set_int(db, "console", "resample_rate", audcfg.resample_rate);
	bmp_cfg_db_set_bool(db, "console", "nsfe_playlist", audcfg.nsfe_playlist);
	bmp_cfg_db_set_int(db, "console", "treble", audcfg.treble);
	bmp_cfg_db_set_int(db, "console", "bass", audcfg.bass);
	bmp_cfg_db_set_bool(db, "console", "ignore_spc_length", audcfg.ignore_spc_length);
	bmp_cfg_db_close(db);
}


// CONFIGURATION PANEL (GTK+2 GUI)

static void i_cfg_ev_resample_enable_commit( gpointer cbt )
{
  audcfg.resample = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(cbt) );
}

static void i_cfg_ev_resample_value_commit( gpointer spbt )
{
  audcfg.resample_rate = (gint)gtk_spin_button_get_value( GTK_SPIN_BUTTON(spbt) );
}

static void i_cfg_ev_bass_value_commit( gpointer spbt )
{
  audcfg.bass = (gint)gtk_spin_button_get_value( GTK_SPIN_BUTTON(spbt) );
}

static void i_cfg_ev_treble_value_commit( gpointer spbt )
{
  audcfg.treble = (gint)gtk_spin_button_get_value( GTK_SPIN_BUTTON(spbt) );
}

static void i_cfg_ev_deflen_value_commit( gpointer spbt )
{
  audcfg.loop_length = (gint)gtk_spin_button_get_value( GTK_SPIN_BUTTON(spbt) );
}

static void i_cfg_ev_nsfeoptpl_enable_commit( gpointer cbt )
{
  audcfg.nsfe_playlist = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(cbt) );
}

static void i_cfg_ev_ignorespclen_enable_commit( gpointer cbt )
{
  audcfg.ignore_spc_length = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(cbt) );
}

static void i_cfg_ev_bok( gpointer configwin )
{
  console_cfg_save();
  gtk_widget_destroy( GTK_WIDGET(configwin) );
}

static void i_cfg_ev_toggle_resample( GtkToggleButton *tbt , gpointer val_hbox )
{
  gtk_widget_set_sensitive( GTK_WIDGET(val_hbox) ,
	gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(tbt) ) );
}


void console_cfg_ui( void )
{
	static GtkWidget *configwin = NULL;
	GtkWidget *configwin_vbox;
	GtkWidget *configwin_gen_vbox, *configwin_nsf_vbox, *configwin_spc_vbox;
	GtkWidget *configwin_gen_resample_frame, *configwin_gen_resample_vbox;
	GtkWidget *configwin_gen_resample_cbt, *configwin_gen_resample_val_hbox;
	GtkWidget *configwin_gen_resample_val_spbt;
	GtkWidget *configwin_gen_playback_frame, *configwin_gen_playback_vbox;
	GtkWidget *configwin_gen_playback_tb_hbox;
	GtkWidget *configwin_gen_playback_tb_bass_hbox, *configwin_gen_playback_tb_bass_spbt;
	GtkWidget *configwin_gen_playback_tb_treble_hbox, *configwin_gen_playback_tb_treble_spbt;
	GtkWidget *configwin_gen_playback_deflen_hbox, *configwin_gen_playback_deflen_spbt;
	GtkWidget *configwin_nsf_nsfeoptpl_cbt;
	GtkWidget *configwin_spc_ignorespclen_cbt;
	GtkWidget *hseparator, *hbuttonbox, *button_ok, *button_cancel;
	GtkWidget *configwin_notebook;
	GtkTooltips *tips;

	if ( configwin != NULL )
		return;

	configwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_type_hint( GTK_WINDOW(configwin), GDK_WINDOW_TYPE_HINT_DIALOG );
	gtk_window_set_title( GTK_WINDOW(configwin), _("Console Music Decoder") );
	gtk_container_set_border_width( GTK_CONTAINER(configwin), 10 );
	g_signal_connect( G_OBJECT(configwin) , "destroy" ,
		G_CALLBACK(gtk_widget_destroyed) , &configwin );
	button_ok = gtk_button_new_from_stock( GTK_STOCK_OK );

	configwin_vbox = gtk_vbox_new( FALSE , 6 );
	gtk_container_add( GTK_CONTAINER(configwin) , configwin_vbox );

	tips = gtk_tooltips_new();
	g_object_set_data_full( G_OBJECT(configwin) , "tt" , tips , g_object_unref );

	configwin_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos( GTK_NOTEBOOK(configwin_notebook) , GTK_POS_TOP );
	gtk_box_pack_start( GTK_BOX(configwin_vbox) , configwin_notebook , TRUE , TRUE , 2 );

	// GENERAL PAGE
	configwin_gen_vbox = gtk_vbox_new( FALSE , 3 );
	gtk_container_set_border_width( GTK_CONTAINER(configwin_gen_vbox), 5 );
	gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
		configwin_gen_vbox , gtk_label_new( _("General") ) );
	// GENERAL PAGE - PLAYBACK FRAME
	configwin_gen_playback_frame = gtk_frame_new( _("Playback") );
	gtk_box_pack_start( GTK_BOX(configwin_gen_vbox) ,
		configwin_gen_playback_frame , TRUE , TRUE , 0 );
	configwin_gen_playback_vbox = gtk_vbox_new( FALSE , 4 );
	gtk_container_set_border_width( GTK_CONTAINER(configwin_gen_playback_vbox), 4 );
	gtk_container_add( GTK_CONTAINER(configwin_gen_playback_frame) , configwin_gen_playback_vbox );
	configwin_gen_playback_tb_hbox = gtk_hbox_new( FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_vbox) ,
		configwin_gen_playback_tb_hbox , FALSE , FALSE , 0 );
	configwin_gen_playback_tb_bass_hbox = gtk_hbox_new( FALSE , 4 );
	configwin_gen_playback_tb_bass_spbt = gtk_spin_button_new_with_range( -100 , 100 , 1 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(configwin_gen_playback_tb_bass_spbt) , audcfg.bass );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_bass_value_commit) , configwin_gen_playback_tb_bass_spbt );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_bass_hbox) ,
		gtk_label_new(_("Bass:")) , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_bass_hbox) ,
		configwin_gen_playback_tb_bass_spbt , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_bass_hbox) ,
		gtk_label_new(_("secs")) , FALSE , FALSE , 0 );
	configwin_gen_playback_tb_treble_hbox = gtk_hbox_new( FALSE , 4 );
	configwin_gen_playback_tb_treble_spbt = gtk_spin_button_new_with_range( -100 , 100 , 1 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(configwin_gen_playback_tb_treble_spbt) , audcfg.treble );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_treble_value_commit) , configwin_gen_playback_tb_treble_spbt );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_treble_hbox) ,
		gtk_label_new(_("Treble:")) , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_treble_hbox) ,
		configwin_gen_playback_tb_treble_spbt , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_treble_hbox) ,
		gtk_label_new(_("secs")) , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_hbox) ,
		configwin_gen_playback_tb_bass_hbox , TRUE , TRUE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_hbox) ,
		gtk_vseparator_new() , FALSE , FALSE , 4 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_tb_hbox) ,
		configwin_gen_playback_tb_treble_hbox , TRUE , TRUE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_vbox) ,
		gtk_hseparator_new() , FALSE , FALSE , 0 );
	configwin_gen_playback_deflen_hbox = gtk_hbox_new( FALSE , 4 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_vbox) ,
		configwin_gen_playback_deflen_hbox , FALSE , FALSE , 0 );
	configwin_gen_playback_deflen_spbt = gtk_spin_button_new_with_range( 1 , 7200 , 10 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(configwin_gen_playback_deflen_spbt) , audcfg.loop_length );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_deflen_value_commit) , configwin_gen_playback_deflen_spbt );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_deflen_hbox) ,
		gtk_label_new(_("Default song length:")) , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_deflen_hbox) ,
		configwin_gen_playback_deflen_spbt , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_playback_deflen_hbox) ,
		gtk_label_new(_("secs")) , FALSE , FALSE , 0 );
	// GENERAL PAGE - RESAMPLING FRAME
	configwin_gen_resample_frame = gtk_frame_new( _("Resampling") );
	gtk_box_pack_start( GTK_BOX(configwin_gen_vbox) ,
		configwin_gen_resample_frame , TRUE , TRUE , 0 );
	configwin_gen_resample_vbox = gtk_vbox_new( FALSE , 4 );
	gtk_container_set_border_width( GTK_CONTAINER(configwin_gen_resample_vbox), 4 );
	gtk_container_add( GTK_CONTAINER(configwin_gen_resample_frame) , configwin_gen_resample_vbox );
	configwin_gen_resample_cbt = gtk_check_button_new_with_label( _("Enable audio resampling") );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_resample_enable_commit) , configwin_gen_resample_cbt );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_vbox) ,
		configwin_gen_resample_cbt , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_vbox) ,
		gtk_hseparator_new() , FALSE , FALSE , 0 );
	configwin_gen_resample_val_hbox = gtk_hbox_new( FALSE , 4 );
	configwin_gen_resample_val_spbt = gtk_spin_button_new_with_range( 11025 , 96000 , 100 );
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(configwin_gen_resample_val_spbt) , audcfg.resample_rate );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_resample_value_commit) , configwin_gen_resample_val_spbt );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_vbox) ,
		configwin_gen_resample_val_hbox , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_val_hbox) ,
		gtk_label_new(_("Resampling rate:")) , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_val_hbox) ,
		configwin_gen_resample_val_spbt , FALSE , FALSE , 0 );
	gtk_box_pack_start( GTK_BOX(configwin_gen_resample_val_hbox) ,
		gtk_label_new(_("Hz")) , FALSE , FALSE , 0 );
	gtk_widget_set_sensitive( GTK_WIDGET(configwin_gen_resample_val_hbox) , audcfg.resample );
	g_signal_connect( G_OBJECT(configwin_gen_resample_cbt) , "toggled" ,
		G_CALLBACK(i_cfg_ev_toggle_resample) , configwin_gen_resample_val_hbox );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(configwin_gen_resample_cbt) , audcfg.resample );

	// NSF/NSFE PAGE
	configwin_nsf_vbox = gtk_vbox_new( FALSE , 3 );
	gtk_container_set_border_width( GTK_CONTAINER(configwin_nsf_vbox), 5 );
	gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
		configwin_nsf_vbox , gtk_label_new( _("NSF/NSFE") ) );
	configwin_nsf_nsfeoptpl_cbt = gtk_check_button_new_with_label( _("Use optional NSFE playlist") );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(configwin_nsf_nsfeoptpl_cbt) , audcfg.nsfe_playlist );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_nsfeoptpl_enable_commit) , configwin_nsf_nsfeoptpl_cbt );
	gtk_box_pack_start( GTK_BOX(configwin_nsf_vbox) ,
		configwin_nsf_nsfeoptpl_cbt , FALSE , FALSE , 0 );

	// SPC PAGE
	configwin_spc_vbox = gtk_vbox_new( FALSE , 3 );
	gtk_container_set_border_width( GTK_CONTAINER(configwin_spc_vbox), 5 );
	gtk_notebook_append_page( GTK_NOTEBOOK(configwin_notebook) ,
		configwin_spc_vbox , gtk_label_new( _("SPC") ) );
	configwin_spc_ignorespclen_cbt = gtk_check_button_new_with_label( _("Ignore length from SPC tags") );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(configwin_spc_ignorespclen_cbt) , audcfg.ignore_spc_length );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_ignorespclen_enable_commit) , configwin_spc_ignorespclen_cbt );
	gtk_box_pack_start( GTK_BOX(configwin_spc_vbox) ,
		configwin_spc_ignorespclen_cbt , FALSE , FALSE , 0 );

	// horizontal separator and buttons
	hbuttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout( GTK_BUTTON_BOX(hbuttonbox) , GTK_BUTTONBOX_END );
	button_cancel = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	g_signal_connect_swapped( G_OBJECT(button_cancel) , "clicked" ,
		G_CALLBACK(gtk_widget_destroy) , configwin );
	gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_cancel );
	g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
		G_CALLBACK(i_cfg_ev_bok) , configwin );
	gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
	gtk_box_pack_start( GTK_BOX(configwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

	gtk_tooltips_set_tip( GTK_TOOLTIPS(tips) , configwin_gen_playback_deflen_spbt ,
		_("* Default song length *\n"
		"The default song length, expressed in seconds, is used for songs "
		"that do not provide length information (i.e. looping tracks).") , "" );

	gtk_widget_show_all( configwin );
}
