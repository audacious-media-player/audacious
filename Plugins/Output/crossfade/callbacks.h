/*
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <gtk/gtk.h>

/* configure.c*/
void on_config_ok_clicked(GtkButton *button, gpointer user_data);
void on_config_apply_clicked(GtkButton *button, gpointer user_data);
void on_output_plugin_radio_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_output_none_radio_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_config_adevice_alt_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_config_mdevice_alt_check_toggled(GtkToggleButton *togglebutton,gpointer user_data);
void on_osshwb_maxbuf_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_config_crossfade_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_config_mixopt_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_output_plugin_configure_button_clicked (GtkButton *button, gpointer user_data);
void on_output_plugin_about_button_clicked(GtkButton *button, gpointer user_data);
void on_op_throttle_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_op_maxblock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_op_maxblock_spin_changed(GtkEditable *editable, gpointer user_data);
void on_op_forcereopen_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_ep_configure_button_clicked(GtkButton *button, gpointer user_data);
void on_ep_about_button_clicked(GtkButton *button, gpointer user_data);
void on_ep_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_volnorm_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xftfp_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xftfp_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_xftffi_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xftffi_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_xftffi_volume_spin_changed(GtkEditable *editable, gpointer user_data);
void on_pause_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_simple_length_spin_changed(GtkEditable *editable,gpointer user_data);
void on_xf_buffer_spin_changed(GtkEditable *editable, gpointer user_data);
void on_xf_autobuf_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_fadeout_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xfofs_none_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xfofs_none_radiobutton_clicked(GtkButton *button, gpointer user_data);
void on_xfofs_lockout_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xfofs_lockout_radiobutton_clicked(GtkButton *button, gpointer user_data);
void on_xfofs_lockin_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xfofs_lockin_radiobutton_clicked(GtkButton *button, gpointer user_data);
void on_xfofs_custom_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_xfofs_custom_radiobutton_clicked(GtkButton *button, gpointer user_data);
void on_xfofs_custom_spin_changed(GtkEditable *editable, gpointer user_data);
void on_fadein_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_fadein_lock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_lgap_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_tgap_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data); 
void on_tgap_lock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_fadeout_length_spin_changed (GtkEditable *editable, gpointer user_data);
void on_fadeout_volume_spin_changed(GtkEditable *editable, gpointer user_data);
void on_fadein_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_fadein_volume_spin_changed(GtkEditable *editable, gpointer user_data);
void on_lgap_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_lgap_level_spin_changed(GtkEditable *editable, gpointer user_data);
void on_tgap_length_spin_changed(GtkEditable *editable, gpointer user_data);
void on_tgap_level_spin_changed(GtkEditable *editable, gpointer user_data);
void on_gapkiller_default_button_clicked(GtkButton *button, gpointer user_data);
void on_moth_songchange_spin_changed(GtkEditable *editable, gpointer user_data);
void on_moth_opmaxused_check_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_misc_default_button_clicked(GtkButton *button, gpointer user_data);
void on_presets_list_click_column(GtkCList *clist, gint column, gpointer user_data);

/* monitor.c */
gboolean on_monitor_display_drawingarea_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
gboolean on_monitor_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);

/* help.c (not yet implemented) */
void on_help_close_button_clicked(GtkButton *button, gpointer user_data);

#endif  /* _CALLBACKS_H_ */
