/*
 *  Copyright (C) 2001  CubeSoft Communications, Inc.
 *  <http://www.csoft.org>
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

#include <errno.h>
#include "sun.h"
#include "libaudacious/util.h"
#include "libaudacious/configdb.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "mixer.h"

struct sun_statsframe stats_frame;

static GtkWidget *configure_win;
static GtkWidget *buffer_size_spin, *buffer_pre_spin;
static GtkWidget *adevice_entry, *actldevice_entry, *mdevice_entry;
static GtkWidget *keepopen_cbutton;
static char devaudio[64], devaudioctl[64], devmixer[64], mixer_toggle[64];

static void configure_win_destroy();

static void configure_win_ok_cb(GtkWidget *w, gpointer data)
{
	ConfigDb *cfgfile;

	strcpy(audio.devaudio, gtk_entry_get_text(GTK_ENTRY(adevice_entry)));
	strcpy(audio.devmixer, gtk_entry_get_text(GTK_ENTRY(mdevice_entry)));

	audio.req_buffer_size = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(buffer_size_spin));
	audio.req_prebuffer_size = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(buffer_pre_spin));

	if (sun_mixer_open() == 0)
	{
		audio.mixer_keepopen = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(keepopen_cbutton));
		sun_mixer_close();
	}

	cfgfile = bmp_cfg_db_open();

	bmp_cfg_db_set_string(cfgfile, "sun",
			      "audio_devaudio", audio.devaudio);
	bmp_cfg_db_set_string(cfgfile, "sun",
			      "audio_devaudioctl", audio.devaudioctl);
	bmp_cfg_db_set_string(cfgfile, "sun",
			      "audio_devmixer", audio.devmixer);

	bmp_cfg_db_set_string(cfgfile, "sun",
			      "mixer_voldev", audio.mixer_voldev);
	bmp_cfg_db_set_bool(cfgfile, "sun",
			       "mixer_keepopen", audio.mixer_keepopen);

	bmp_cfg_db_set_int(cfgfile, "sun",
			   "buffer_size", audio.req_buffer_size);
	bmp_cfg_db_set_int(cfgfile, "sun",
			   "prebuffer_size", audio.req_prebuffer_size);

	bmp_cfg_db_close(cfgfile);

	configure_win_destroy();
}

static void configure_win_cancel_cb(GtkWidget *w, gpointer data)
{
	configure_win_destroy();
}

static void mixer_cbutton_toggled_cb(GtkWidget *w, int id)
{
	mixer_ctrl_t mixer;

	if (sun_mixer_open() == 0)
	{
		mixer.type = AUDIO_MIXER_ENUM;
		mixer.dev = id;
		mixer_toggle[id] = !mixer_toggle[id];
		mixer.un.ord = mixer_toggle[id];

		if (ioctl(audio.mixerfd, AUDIO_MIXER_WRITE, &mixer) != 0)
			g_warning("Could not toggle mixer setting %i", id);
		sun_mixer_close();
	}
}

static void configure_win_mixer_volume_dev_cb(GtkWidget *w, gint voldev_index)
{
	mixer_devinfo_t info;

	if (sun_mixer_open() == 0)
	{
		info.index = voldev_index;
		if (!ioctl(audio.mixerfd, AUDIO_MIXER_DEVINFO, &info))
			strcpy(audio.mixer_voldev, info.label.name);
		sun_mixer_close();
	}
}

static void configure_win_destroy(void)
{
	stats_frame.active = 0;

	if (!pthread_mutex_lock(&stats_frame.active_mutex))
	{
		if (!pthread_mutex_lock(&stats_frame.audioctl_mutex))
		{
			if (stats_frame.fd)
			{
				close(stats_frame.fd);
				stats_frame.fd = 0;
			}
			pthread_mutex_unlock(&stats_frame.audioctl_mutex);
			pthread_mutex_destroy(&stats_frame.audioctl_mutex);
		}
		pthread_mutex_unlock(&stats_frame.active_mutex);
		pthread_mutex_destroy(&stats_frame.active_mutex);
	}
	gtk_widget_destroy(configure_win);
	configure_win = NULL;
}

static void configure_mixer_volumedev_scan(gchar *type, GtkWidget *option_menu)
{
	mixer_devinfo_t info;
	GtkWidget *menu;

	if (sun_mixer_open() < 0)
		return;

	menu = gtk_menu_new();

	/* FIXME: info is used while undefined here */
	for (info.index = 0;
	     ioctl(audio.mixerfd, AUDIO_MIXER_DEVINFO, &info) == 0;
	     info.index++)
	{
		GtkWidget *item;

		if (info.type == AUDIO_MIXER_VALUE)
		{
			item = gtk_menu_item_new_with_label(info.label.name);
			g_signal_connect(G_OBJECT(item), "activate",
					   (GCallback) configure_win_mixer_volume_dev_cb,
					   (gpointer) info.index);

			gtk_widget_show(item);
			gtk_menu_append(GTK_MENU(menu), item);

			if (!strcmp(info.label.name, audio.mixer_voldev))
				gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
		}
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);

	sun_mixer_close();
}

static void configure_adevice_box(GtkWidget *dev_vbox)
{
	GtkWidget *adevice_frame, *adevice_vbox;

	adevice_frame = gtk_frame_new(_("Audio device:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), adevice_frame, FALSE, FALSE, 0);

	adevice_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(adevice_vbox), 5);
	gtk_container_add(GTK_CONTAINER(adevice_frame), adevice_vbox);

	strcpy(devaudio, audio.devaudio);

	adevice_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(adevice_entry), devaudio);
	gtk_box_pack_start_defaults(GTK_BOX(adevice_vbox), adevice_entry);
}

static void configure_actldevice_box(GtkWidget *dev_vbox)
{
	GtkWidget *actldevice_frame, *actldevice_vbox;

	actldevice_frame = gtk_frame_new(_("Audio control device:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), actldevice_frame,
			   FALSE, FALSE, 0);

	actldevice_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(actldevice_vbox), 5);
	gtk_container_add(GTK_CONTAINER(actldevice_frame), actldevice_vbox);

	strcpy(devaudioctl, audio.devaudioctl);

	actldevice_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(actldevice_entry), devaudioctl);
	gtk_box_pack_start_defaults(GTK_BOX(actldevice_vbox), actldevice_entry);
}

static void configure_mdevice_box(GtkWidget *dev_vbox)
{
	GtkWidget *mdevice_frame, *mdevice_vbox;

	mdevice_frame = gtk_frame_new(_("Mixer device:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), mdevice_frame, FALSE, FALSE, 0);

	mdevice_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mdevice_vbox), 5);
	gtk_container_add(GTK_CONTAINER(mdevice_frame), mdevice_vbox);

	strcpy(devmixer, audio.devmixer);

	mdevice_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(mdevice_entry), devmixer);
	gtk_box_pack_start_defaults(GTK_BOX(mdevice_vbox), mdevice_entry);

}


static void configure_devices_frame(GtkWidget *vbox, GtkWidget * notebook)
{
	GtkWidget *dev_vbox;

	dev_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

	configure_adevice_box(dev_vbox);
	configure_actldevice_box(dev_vbox);
	configure_mdevice_box(dev_vbox);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox,
				 gtk_label_new(_("Devices")));
}


static void configure_buffering_frame(GtkWidget *vbox, GtkWidget * notebook)
{
	GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
	GtkWidget *buffer_size_box, *buffer_size_label;
	GtkObject *buffer_size_adj, *buffer_pre_adj;
	GtkWidget *buffer_pre_box, *buffer_pre_label;

	buffer_frame = gtk_frame_new(_("Buffering:"));
	gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);

	buffer_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);

	buffer_table = gtk_table_new(2, 1, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
	gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);

	buffer_size_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults( GTK_TABLE(buffer_table),
	    buffer_size_box, 0, 1, 0, 1);
	buffer_size_label = gtk_label_new(_("Buffer size (ms):"));

	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_label,
			   FALSE, FALSE, 0);

	buffer_size_adj = gtk_adjustment_new(audio.req_buffer_size,
					     200, 131072, 100, 100, 100);

	buffer_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj),
					       8, 0);

	gtk_widget_set_usize(buffer_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_size_box),
			   buffer_size_spin, FALSE, FALSE, 0);

	buffer_pre_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table),
				  buffer_pre_box, 1, 2, 0, 1);
	buffer_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_label,
			   FALSE, FALSE, 0);

	buffer_pre_adj = gtk_adjustment_new(audio.req_prebuffer_size,
					    0, 90, 1, 1, 1);
	buffer_pre_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_pre_adj),
					      1, 0);

	gtk_widget_set_usize(buffer_pre_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_pre_box), buffer_pre_spin,
			   FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buffer_frame,
				 gtk_label_new(_("Buffering")));
}

static void configure_mixer_toggle_button(GtkWidget *vbox, gchar *devname, gchar *label)
{
	GtkWidget *toggle_cbutton;
	gint devid;
	mixer_ctrl_t mixer;

	if (!sun_mixer_get_dev(audio.mixerfd, &devid, devname))
	{
		mixer.type = AUDIO_MIXER_ENUM;
		mixer.dev = devid;

		if (!ioctl(audio.mixerfd, AUDIO_MIXER_READ, &mixer))
		{
			toggle_cbutton =
				gtk_check_button_new_with_label(_(label));
			gtk_box_pack_start_defaults(GTK_BOX(vbox),
						    toggle_cbutton);

			if (mixer.un.ord)
			{
				gtk_toggle_button_set_active(
				    GTK_TOGGLE_BUTTON(toggle_cbutton), TRUE);
				mixer_toggle[mixer.dev]++;
			}
			else
			{
				mixer_toggle[mixer.dev] = 0;
			}

			gtk_signal_connect(GTK_OBJECT(toggle_cbutton),
			    "toggled",
			    GTK_SIGNAL_FUNC(mixer_cbutton_toggled_cb),
			    (gpointer) mixer.dev);
		}
	}
}


static void configure_mixer_box(GtkWidget *vbox, GtkWidget *notebook)
{
	GtkWidget *mixervol_frame, *mixervol_box;
	GtkWidget *mixervol_menu;

	mixervol_frame = gtk_frame_new(_("Volume controls device:"));
	gtk_box_pack_start(GTK_BOX(vbox), mixervol_frame, FALSE, FALSE, 0);

	mixervol_box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mixervol_box), 5);
	gtk_container_add(GTK_CONTAINER(mixervol_frame), mixervol_box);

	mixervol_menu = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(mixervol_box), mixervol_menu, TRUE, TRUE, 0);

	configure_mixer_volumedev_scan("Volume devices:", mixervol_menu);

	keepopen_cbutton = gtk_check_button_new_with_label(
		_("XMMS uses mixer exclusively."));
	if (audio.mixer_keepopen)
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(keepopen_cbutton), TRUE);

	gtk_box_pack_start_defaults(GTK_BOX(vbox), keepopen_cbutton);

	if (sun_mixer_open() == 0)
	{
		configure_mixer_toggle_button(vbox, "bassboost", "Bass boost");
		configure_mixer_toggle_button(vbox, "loudness", "Loudness");
		configure_mixer_toggle_button(vbox, "spatial", "Spatial");
		configure_mixer_toggle_button(vbox, "surround", "Surround");
		configure_mixer_toggle_button(vbox, "enhanced", "Enhanced");
		configure_mixer_toggle_button(vbox, "preamp", "Preamp");
		configure_mixer_toggle_button(vbox, "swap", "Swap channels");
		sun_mixer_close();
	}
}


static void configure_mixer_frame(GtkWidget *vbox, GtkWidget *notebook)
{
	GtkWidget *mixervol_vbox;

	mixervol_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(mixervol_vbox), 5);

	configure_mixer_box(mixervol_vbox, notebook);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 mixervol_vbox, gtk_label_new(_("Mixer")));
}


static void configure_stats_loop(void)
{
	if (pthread_mutex_lock(&stats_frame.active_mutex) != 0)
	{
		perror("active_mutex");
		return;
	}

	while (stats_frame.active && stats_frame.fd)
	{
		audio_info_t info;
		char sl[32];

		pthread_mutex_lock(&stats_frame.audioctl_mutex);

		sl[0] = '\0';

		if (!ioctl(stats_frame.fd, AUDIO_GETINFO, &info))
		{
			char s[128];

			sprintf(s, "Currently %s",
				(info.mode == AUMODE_PLAY) ? "playing" :
				(info.mode == AUMODE_RECORD) ? "recording" :
				(info.mode == AUMODE_PLAY_ALL) ? "playing" :
				"not playing");

			if (info.mode == AUMODE_PLAY)
			{
				sprintf(s, "%s at %i Hz (%i-bit %s)", s,
					info.play.sample_rate, info.play.precision,
					audio.output->name);
				sprintf(sl,"%i samples, %i error(s). %s",
					info.play.samples, info.play.error,
					info.play.active ?
					"I/O in progress." : "");
			}
			gtk_label_set_text(GTK_LABEL(stats_frame.mode_label),
					   s);

			sprintf(s, "H/W block: %i bytes. Buffer: %i bytes",
				info.blocksize, info.play.buffer_size);
			gtk_label_set_text(
				GTK_LABEL(stats_frame.blocksize_label), s);
		}
		gtk_label_set_text(GTK_LABEL(stats_frame.ooffs_label), sl);

		pthread_mutex_unlock(&stats_frame.audioctl_mutex);
		xmms_usleep(400000);
	}
	pthread_mutex_unlock(&stats_frame.active_mutex);

	pthread_exit(NULL);
}

static void configure_status_frame(GtkWidget *vbox, GtkWidget *notebook)
{
	GtkWidget *status_vbox;
	GtkWidget *name_label, *props_label;
	pthread_t loop_thread;

	memset(&stats_frame, 0, sizeof(struct sun_statsframe));

	if (pthread_mutex_init(&stats_frame.audioctl_mutex, NULL) != 0)
	{
		perror("audioctl_mutex");
		return;
	}
	if (pthread_mutex_init(&stats_frame.active_mutex, NULL) != 0)
	{
		perror("active_mutex");
		return;
	}
	status_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(status_vbox), 5);

	name_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(status_vbox), name_label);
	props_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(status_vbox), props_label);

	stats_frame.mode_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(status_vbox), stats_frame.mode_label);
	stats_frame.blocksize_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(status_vbox),
	    stats_frame.blocksize_label);
	stats_frame.ooffs_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(status_vbox), stats_frame.ooffs_label);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), status_vbox,
				 gtk_label_new(_("Status")));

	if ((stats_frame.fd = open(audio.devaudioctl, O_RDWR)) >= 0)
	{
		audio_device_t device;
		int props;

		if (ioctl(stats_frame.fd, AUDIO_GETDEV, &device) >= 0)
		{
			char *s = g_strdup_printf("%s - %s(4) %s",
				device.name, device.config, device.version);
			gtk_label_set_text(GTK_LABEL(name_label), s);
			g_free(s);
		}
		if (ioctl(stats_frame.fd, AUDIO_GETPROPS, &props) >= 0)
		{
			char s[32];
			s[0] = '\0';

			if ((props & AUDIO_PROP_FULLDUPLEX) ==
			    AUDIO_PROP_FULLDUPLEX)
				sprintf(s, "FULLDUPLEX ");
			if ((props & AUDIO_PROP_MMAP) == AUDIO_PROP_MMAP)
				sprintf(s, "%s MMAP ", s);
			if ((props & AUDIO_PROP_INDEPENDENT) ==
			    AUDIO_PROP_INDEPENDENT)
				sprintf(s, "%s INDEPENDENT ", s);

			gtk_label_set_text(GTK_LABEL(props_label), s);
		}
	}
	stats_frame.active++;
	pthread_create(&loop_thread, NULL, (void *) configure_stats_loop, NULL);
}

void sun_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *bbox, *ok, *cancel;

	if (configure_win)
	{
		gdk_window_raise(configure_win->window);
		return;
	}
	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy",
			   GTK_SIGNAL_FUNC(configure_win_destroy), NULL);

	gtk_window_set_title(GTK_WINDOW(configure_win),
			     _("Sun driver configuration"));
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	configure_devices_frame(vbox, notebook);
	configure_buffering_frame(vbox, notebook);
	configure_mixer_frame(vbox, notebook);
	configure_status_frame(vbox, notebook);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);

	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  GTK_SIGNAL_FUNC(configure_win_cancel_cb),
				  GTK_OBJECT(configure_win));

	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(configure_win);
}

