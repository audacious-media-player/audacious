/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Håvard Kvålen
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

//#include <glib/gi18n.h>
#include "coreaudio.h"
#include "libaudacious/configdb.h"
#include <CoreAudio/CoreAudio.h>

static GtkWidget *configure_win = NULL;
static GtkWidget *mixer_usemaster_check, *buffer_size_spin, *buffer_pre_spin;
static GtkWidget *adevice_use_alt_check, *audio_alt_device_entry;
static GtkWidget *mdevice_use_alt_check, *mixer_alt_device_entry;
static gint audio_device, mixer_device;

static void configure_win_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigDb *cfgfile;

	osx_cfg.audio_device = audio_device;
	osx_cfg.mixer_device = mixer_device;
	osx_cfg.buffer_size =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_size_spin));
	osx_cfg.prebuffer =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_pre_spin));
	osx_cfg.use_master =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mixer_usemaster_check));
	osx_cfg.use_alt_audio_device =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(adevice_use_alt_check));
	osx_cfg.use_alt_mixer_device =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mdevice_use_alt_check));
	g_free(osx_cfg.alt_audio_device);
	osx_cfg.alt_audio_device =
		gtk_editable_get_chars(GTK_EDITABLE(audio_alt_device_entry), 0, -1);
	g_strstrip(osx_cfg.alt_audio_device);
	g_free(osx_cfg.alt_mixer_device);
	osx_cfg.alt_mixer_device =
		gtk_editable_get_chars(GTK_EDITABLE(mixer_alt_device_entry), 0, -1);
	g_strstrip(osx_cfg.alt_mixer_device);

	if (osx_cfg.use_alt_audio_device)
		/* do a minimum of sanity checking */
		if (osx_cfg.alt_audio_device[0] != '/') 
			osx_cfg.use_alt_audio_device = FALSE;
	if (osx_cfg.use_alt_mixer_device)
		if (osx_cfg.alt_mixer_device[0] != '/') 
			osx_cfg.use_alt_mixer_device = FALSE;
	
	cfgfile = bmp_cfg_db_open();

	bmp_cfg_db_set_int(cfgfile, "OSX", "audio_device", osx_cfg.audio_device);
	bmp_cfg_db_set_int(cfgfile, "OSX", "mixer_device", osx_cfg.mixer_device);
	bmp_cfg_db_set_int(cfgfile, "OSX", "buffer_size", osx_cfg.buffer_size);
	bmp_cfg_db_set_int(cfgfile, "OSX", "prebuffer", osx_cfg.prebuffer);
	bmp_cfg_db_set_bool(cfgfile,"OSX","use_master",osx_cfg.use_master);
	bmp_cfg_db_set_bool(cfgfile, "OSX", "use_alt_audio_device", osx_cfg.use_alt_audio_device);
	bmp_cfg_db_set_string(cfgfile, "OSX", "alt_audio_device", osx_cfg.alt_audio_device);
	bmp_cfg_db_set_bool(cfgfile, "OSX", "use_alt_mixer_device", osx_cfg.use_alt_mixer_device);
	bmp_cfg_db_set_string(cfgfile, "OSX", "alt_mixer_device", osx_cfg.alt_mixer_device);
	bmp_cfg_db_close(cfgfile);

	gtk_widget_destroy(configure_win);
}

static void configure_win_audio_dev_cb(GtkWidget * widget, gint device)
{
	audio_device = device;
}

static void configure_win_mixer_dev_cb(GtkWidget * widget, gint device)
{
	mixer_device = device;
}

static void audio_device_toggled(GtkToggleButton * widget, gpointer data)
{
	gboolean use_alt_audio_device = gtk_toggle_button_get_active(widget);
	gtk_widget_set_sensitive(GTK_WIDGET(data), !use_alt_audio_device);
	gtk_widget_set_sensitive(audio_alt_device_entry, use_alt_audio_device);
}

static void mixer_device_toggled(GtkToggleButton * widget, gpointer data)
{
	gboolean use_alt_device = gtk_toggle_button_get_active(widget);
	gtk_widget_set_sensitive(GTK_WIDGET(data), !use_alt_device);
	gtk_widget_set_sensitive(mixer_alt_device_entry, use_alt_device);
}

static void scan_devices(gchar * type, GtkWidget * option_menu, GtkSignalFunc sigfunc)
{
	GtkWidget *menu, *item;
	FILE *file;
	UInt32 size,len,i;
	AudioDeviceID * devicelist;
	char device_name[128];

	menu = gtk_menu_new();

	if (AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,NULL))
	{
		item = gtk_menu_item_new_with_label("Default");
		gtk_signal_connect(GTK_OBJECT(item), "activate", sigfunc, (gpointer) 0);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
	}
	else
	{
		printf("there are %d devices\n",size/sizeof(AudioDeviceID));
		
		devicelist = (AudioDeviceID*) malloc(size);
		
		if (AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,devicelist))
		{
			printf("failed to get device list");
		}
		
		for (i = 0; i < size/sizeof(AudioDeviceID); i++)
		{
			len = 128;
			AudioDeviceGetProperty(devicelist[i],1,0,kAudioDevicePropertyDeviceName,&len,device_name);
			
			item = gtk_menu_item_new_with_label(device_name);
			gtk_signal_connect(GTK_OBJECT(item), "activate", sigfunc, (gpointer) 0);
			gtk_widget_show(item);
			gtk_menu_append(GTK_MENU(menu), item);
			
		}
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
}

void osx_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *dev_vbox, *adevice_frame, *adevice_box, *adevice;
	GtkWidget *mdevice_frame, *mdevice_box, *mdevice;
	GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
	GtkWidget *buffer_size_box, *buffer_size_label;
	GtkObject *buffer_size_adj, *buffer_pre_adj;
	GtkWidget *buffer_pre_box, *buffer_pre_label;
	GtkWidget *audio_alt_box, *mixer_alt_box;
	GtkWidget *bbox, *ok, *cancel;
	GtkWidget *mixer_table, *mixer_frame;
	
	if (configure_win)
	{
		gdk_window_raise(configure_win->window);
		return;
	}

	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win), "CoreAudio Plugin Configuration");
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);
	

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);
	
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	
	dev_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

	adevice_frame = gtk_frame_new("Audio device:");
	gtk_box_pack_start(GTK_BOX(dev_vbox), adevice_frame, FALSE, FALSE, 0);
	
	adevice_box = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(adevice_box), 5);
	gtk_container_add(GTK_CONTAINER(adevice_frame), adevice_box);

	adevice = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(adevice_box), adevice, TRUE, TRUE, 0);

	scan_devices("Audio devices:", adevice, configure_win_audio_dev_cb);

	audio_device = osx_cfg.audio_device;
	gtk_option_menu_set_history(GTK_OPTION_MENU(adevice), osx_cfg.audio_device);

	/*
	  audio_alt_box = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start_defaults(GTK_BOX(adevice_box), audio_alt_box);
		adevice_use_alt_check = gtk_check_button_new_with_label("Use alternate device:");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(adevice_use_alt_check), osx_cfg.use_alt_audio_device);
		gtk_signal_connect(GTK_OBJECT(adevice_use_alt_check), "toggled", audio_device_toggled, adevice);
   
		gtk_box_pack_start(GTK_BOX(audio_alt_box), adevice_use_alt_check, FALSE, FALSE, 0);
		audio_alt_device_entry = gtk_entry_new();

	gtk_box_pack_start_defaults(GTK_BOX(audio_alt_box), audio_alt_device_entry);
	*/

	gtk_box_pack_start(GTK_BOX(dev_vbox), mdevice_frame, FALSE, FALSE, 0);
	
	mdevice_box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mdevice_box), 5);
	gtk_container_add(GTK_CONTAINER(mdevice_frame), mdevice_box);

	mdevice = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(mdevice_box), mdevice, TRUE, TRUE, 0);

	scan_devices("Mixers:", mdevice, configure_win_mixer_dev_cb);

	mixer_device = osx_cfg.mixer_device;
	gtk_option_menu_set_history(GTK_OPTION_MENU(mdevice), osx_cfg.mixer_device);
	mixer_alt_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(mdevice_box), mixer_alt_box);
	mdevice_use_alt_check = gtk_check_button_new_with_label("Use alternate device:");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mdevice_use_alt_check), osx_cfg.use_alt_mixer_device);
	gtk_signal_connect(GTK_OBJECT(mdevice_use_alt_check), "toggled", mixer_device_toggled, mdevice);
	gtk_box_pack_start(GTK_BOX(mixer_alt_box), mdevice_use_alt_check, FALSE, FALSE, 0);
	mixer_alt_device_entry = gtk_entry_new();

	if (osx_cfg.alt_mixer_device != NULL)
		gtk_entry_set_text(GTK_ENTRY(mixer_alt_device_entry), osx_cfg.alt_mixer_device);
	else
		gtk_entry_set_text(GTK_ENTRY(mixer_alt_device_entry), "/dev/mixer");

	gtk_box_pack_start_defaults(GTK_BOX(mixer_alt_box), mixer_alt_device_entry);

	if (osx_cfg.use_alt_mixer_device)
		gtk_widget_set_sensitive(mdevice, FALSE);
	else
		gtk_widget_set_sensitive(mixer_alt_device_entry, FALSE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox, gtk_label_new("Devices"));


	buffer_frame = gtk_frame_new("Buffering:");
	gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);

	buffer_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);

	buffer_table = gtk_table_new(2, 1, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
	gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);

	buffer_size_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_size_box, 0, 1, 0, 1);
	buffer_size_label = gtk_label_new("Buffer size (ms):");
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_label, FALSE, FALSE, 0);
	buffer_size_adj = gtk_adjustment_new(osx_cfg.buffer_size, 200, 10000, 100, 100, 100);
	buffer_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj), 8, 0);
	gtk_widget_set_usize(buffer_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_spin, FALSE, FALSE, 0);

	buffer_pre_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_pre_box, 1, 2, 0, 1);
	buffer_pre_label = gtk_label_new("Pre-buffer (percent):");
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_label, FALSE, FALSE, 0);
	buffer_pre_adj = gtk_adjustment_new(osx_cfg.prebuffer, 0, 90, 1, 1, 1);
	buffer_pre_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_pre_adj), 1, 0);
	gtk_widget_set_usize(buffer_pre_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_spin, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buffer_frame, gtk_label_new("Buffering"));


	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(configure_win);
}
