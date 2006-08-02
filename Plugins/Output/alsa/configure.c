/*  XMMS - ALSA output plugin
 *  Copyright (C) 2001-2003 Matthieu Sozeau <mattam@altern.org>
 *  Copyright (C) 2003-2005 Haavard Kvaalen
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#include "alsa.h"
#include <stdio.h>

static GtkWidget *configure_win = NULL;
static GtkWidget *buffer_time_spin, *period_time_spin;
static GtkWidget *softvolume_toggle_button;

static GtkWidget *devices_combo, *mixer_devices_combo;

static int current_mixer_card;

#define GET_SPIN_INT(spin) \
	gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin))
#define GET_TOGGLE(tb) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb))
#define GET_CHARS(edit) gtk_editable_get_chars(GTK_EDITABLE(edit), 0, -1)

static void configure_win_ok_cb(GtkWidget * w, gpointer data)
{
	g_free(alsa_cfg.pcm_device);
	alsa_cfg.pcm_device = GET_CHARS(GTK_COMBO(devices_combo)->entry);
	alsa_cfg.buffer_time = GET_SPIN_INT(buffer_time_spin);
	alsa_cfg.period_time = GET_SPIN_INT(period_time_spin);
	alsa_cfg.soft_volume = GET_TOGGLE(softvolume_toggle_button);
	alsa_cfg.mixer_card = current_mixer_card;
	alsa_cfg.mixer_device = GET_CHARS(GTK_COMBO(mixer_devices_combo)->entry);

	alsa_save_config();
	gtk_widget_destroy(configure_win);
}

void alsa_save_config(void)
{
	ConfigDb *cfgfile = bmp_cfg_db_open();

	bmp_cfg_db_set_int(cfgfile, "ALSA", "buffer_time", alsa_cfg.buffer_time);
	bmp_cfg_db_set_int(cfgfile, "ALSA", "period_time", alsa_cfg.period_time);
	bmp_cfg_db_set_string(cfgfile,"ALSA","pcm_device", alsa_cfg.pcm_device);
	bmp_cfg_db_set_int(cfgfile, "ALSA", "mixer_card", alsa_cfg.mixer_card);
	bmp_cfg_db_set_string(cfgfile,"ALSA","mixer_device", alsa_cfg.mixer_device);
	bmp_cfg_db_set_bool(cfgfile, "ALSA", "soft_volume",
			       alsa_cfg.soft_volume);
	bmp_cfg_db_set_int(cfgfile, "ALSA", "volume_left", alsa_cfg.vol.left);
	bmp_cfg_db_set_int(cfgfile, "ALSA", "volume_right", alsa_cfg.vol.right);
	bmp_cfg_db_close(cfgfile);
}

static int get_cards(GtkOptionMenu *omenu, GtkSignalFunc cb, int active)
{
	GtkWidget *menu, *item;
	int card = -1, err, set = 0, curr = -1;

	menu = gtk_menu_new();
	if ((err = snd_card_next(&card)) != 0)
		g_warning("snd_next_card() failed: %s", snd_strerror(-err));

	while (card > -1)
	{
		char *label;

		curr++;
		if (card == active)
			set = curr;
		if ((err = snd_card_get_name(card, &label)) != 0)
		{
			g_warning("snd_carg_get_name() failed: %s",
				  snd_strerror(-err));
			break;
		}

		item = gtk_menu_item_new_with_label(label);
		gtk_signal_connect(GTK_OBJECT(item), "activate", cb,
				   GINT_TO_POINTER(card));
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
		if ((err = snd_card_next(&card)) != 0)
		{
			g_warning("snd_next_card() failed: %s",
				  snd_strerror(-err));
			break;
		}
	}

	gtk_option_menu_set_menu(omenu, menu);
	return set;
}

static int get_mixer_devices(GtkCombo *combo, int card)
{
	GList *items = NULL;
	int err;
	snd_mixer_t *mixer;
	snd_mixer_elem_t *current;

	if ((err = alsa_get_mixer(&mixer, card)) < 0)
		return err;

	current = snd_mixer_first_elem(mixer);

	while (current)
	{
		const char *sname = snd_mixer_selem_get_name(current);
		if (snd_mixer_selem_is_active(current) &&
		    snd_mixer_selem_has_playback_volume(current))
			items = g_list_append(items, g_strdup(sname));
		current = snd_mixer_elem_next(current);
	}

	gtk_combo_set_popdown_strings(combo, items);

	return 0;
}

static void get_devices_for_card(GtkCombo *combo, int card)
{
	GtkWidget *item;
	int pcm_device = -1, err;
	snd_pcm_info_t *pcm_info;
	snd_ctl_t *ctl;
	char dev[64], *card_name;

	sprintf(dev, "hw:%i", card);

	if ((err = snd_ctl_open(&ctl, dev, 0)) < 0)
	{
		printf("snd_ctl_open() failed: %s", snd_strerror(-err));
		return;
	}

	if ((err = snd_card_get_name(card, &card_name)) != 0)
	{
		g_warning("snd_card_get_name() failed: %s", snd_strerror(-err));
		card_name = _("Unknown soundcard");
	}

	snd_pcm_info_alloca(&pcm_info);

	for (;;)
	{
		char *device, *descr;
		if ((err = snd_ctl_pcm_next_device(ctl, &pcm_device)) < 0)
		{
			g_warning("snd_ctl_pcm_next_device() failed: %s",
				  snd_strerror(-err));
			pcm_device = -1;
		}
		if (pcm_device < 0)
			break;

		snd_pcm_info_set_device(pcm_info, pcm_device);
		snd_pcm_info_set_subdevice(pcm_info, 0);
		snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

		if ((err = snd_ctl_pcm_info(ctl, pcm_info)) < 0)
		{
			if (err != -ENOENT)
				g_warning("get_devices_for_card(): "
					  "snd_ctl_pcm_info() "
					  "failed (%d:%d): %s.", card,
					  pcm_device, snd_strerror(-err));
			continue;
		}

		device = g_strdup_printf("hw:%d,%d", card, pcm_device);
		descr = g_strconcat(card_name, ": ",
				    snd_pcm_info_get_name(pcm_info),
				    " (", device, ")", NULL);
		item = gtk_list_item_new_with_label(descr);
		gtk_widget_show(item);
		g_free(descr);
		gtk_combo_set_item_string(combo, GTK_ITEM(item), device);
		g_free(device);
		gtk_container_add(GTK_CONTAINER(combo->list), item);
	}

	snd_ctl_close(ctl);
}



static void get_devices(GtkCombo *combo)
{
	GtkWidget *item;
	int card = -1;
	int err = 0;
	char *descr;

	descr = g_strdup_printf(_("Default PCM device (%s)"), "default");
	item = gtk_list_item_new_with_label(descr);
	gtk_widget_show(item);
	g_free(descr);
	gtk_combo_set_item_string(combo, GTK_ITEM(item), "default");
	gtk_container_add(GTK_CONTAINER(combo->list), item);

	if ((err = snd_card_next(&card)) != 0)
	{
		g_warning("snd_next_card() failed: %s", snd_strerror(-err));
		return;
	}

	while (card > -1)
	{
		get_devices_for_card(combo, card);
		if ((err = snd_card_next(&card)) != 0)
		{
			g_warning("snd_next_card() failed: %s",
				  snd_strerror(-err));
			break;
		}
	}
}

static void mixer_card_cb(GtkWidget * widget, gpointer card)
{
	if (current_mixer_card == GPOINTER_TO_INT(card))
		return;
	current_mixer_card = GPOINTER_TO_INT(card);
	get_mixer_devices(GTK_COMBO(mixer_devices_combo),
			  current_mixer_card);
}

static void softvolume_toggle_cb(GtkToggleButton * widget, gpointer data)
{
	gboolean softvolume = gtk_toggle_button_get_active(widget);
	gtk_widget_set_sensitive(GTK_WIDGET(data), !softvolume);
	gtk_widget_set_sensitive(mixer_devices_combo, !softvolume);
}

void alsa_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *dev_vbox, *adevice_frame, *adevice_box;
	GtkWidget *mixer_frame, *mixer_box, *mixer_table, *mixer_card_om;
	GtkWidget *mixer_card_label, *mixer_device_label;
	GtkWidget *advanced_vbox, *card_vbox;
	GtkWidget *buffer_table;
	GtkWidget *card_frame, *buffer_time_label, *period_time_label;
	GtkObject *buffer_time_adj, *period_time_adj;
	GtkWidget *bbox, *ok, *cancel;

	int mset;

	if (configure_win)
	{
		gdk_window_raise(configure_win->window);
		return;
	}

	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win),
			     _("ALSA Driver configuration"));
	gtk_window_set_policy(GTK_WINDOW(configure_win),
			      FALSE, TRUE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	dev_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

	adevice_frame = gtk_frame_new(_("Audio device:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), adevice_frame, FALSE, FALSE, 0);

	adevice_box = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(adevice_box), 5);
	gtk_container_add(GTK_CONTAINER(adevice_frame), adevice_box);

	devices_combo = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(adevice_box), devices_combo,
			   FALSE, FALSE, 0);
	get_devices(GTK_COMBO(devices_combo));
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(devices_combo)->entry),
			   alsa_cfg.pcm_device);

	mixer_frame = gtk_frame_new(_("Mixer:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), mixer_frame, FALSE, FALSE, 0);

	mixer_box = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(mixer_box), 5);
	gtk_container_add(GTK_CONTAINER(mixer_frame), mixer_box);

	softvolume_toggle_button = gtk_check_button_new_with_label(
		_("Use software volume control"));

	gtk_box_pack_start(GTK_BOX(mixer_box), softvolume_toggle_button,
			   FALSE, FALSE, 0);

	mixer_table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(mixer_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(mixer_table), 5);
	gtk_box_pack_start(GTK_BOX(mixer_box), mixer_table, FALSE, FALSE, 0);

	mixer_card_label = gtk_label_new(_("Mixer card:"));
	gtk_label_set_justify(GTK_LABEL(mixer_card_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(mixer_card_label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(mixer_table), mixer_card_label,
			 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	mixer_card_om = gtk_option_menu_new();
	mset = get_cards(GTK_OPTION_MENU(mixer_card_om),
			 (GtkSignalFunc)mixer_card_cb, alsa_cfg.mixer_card);

	gtk_table_attach(GTK_TABLE(mixer_table), mixer_card_om,
			 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	mixer_device_label = gtk_label_new(_("Mixer device:"));
	gtk_label_set_justify(GTK_LABEL(mixer_device_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(mixer_device_label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(mixer_table), mixer_device_label,
			 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	mixer_devices_combo = gtk_combo_new();
	gtk_option_menu_set_history(GTK_OPTION_MENU(mixer_card_om), mset);
	get_mixer_devices(GTK_COMBO(mixer_devices_combo), alsa_cfg.mixer_card);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(mixer_devices_combo)->entry),
			   alsa_cfg.mixer_device);

	gtk_table_attach(GTK_TABLE(mixer_table), mixer_devices_combo,
			 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 0, 0);

	gtk_signal_connect(GTK_OBJECT(softvolume_toggle_button), "toggled",
			   (GCallback)softvolume_toggle_cb, mixer_card_om);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(softvolume_toggle_button),
				     alsa_cfg.soft_volume);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox,
				 gtk_label_new(_("Device settings")));


	advanced_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(advanced_vbox), 5);

	card_frame = gtk_frame_new(_("Soundcard:"));
	gtk_box_pack_start_defaults(GTK_BOX(advanced_vbox), card_frame);

	card_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(card_frame), card_vbox);

	gtk_container_set_border_width(GTK_CONTAINER(card_vbox), 5);

	buffer_table = gtk_table_new(2, 2, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(buffer_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(buffer_table), 5);
	gtk_box_pack_start_defaults(GTK_BOX(card_vbox), buffer_table);

	buffer_time_label = gtk_label_new(_("Buffer time (ms):"));
	gtk_label_set_justify(GTK_LABEL(buffer_time_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(buffer_time_label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(buffer_table), buffer_time_label,
			 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	buffer_time_adj = gtk_adjustment_new(alsa_cfg.buffer_time,
					     200, 10000, 100, 100, 100);
	buffer_time_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_time_adj),
					       8, 0);
	gtk_widget_set_usize(buffer_time_spin, 60, -1);
	gtk_table_attach(GTK_TABLE(buffer_table), buffer_time_spin,
			 1, 2, 0, 1, 0, 0, 0, 0);

	period_time_label = gtk_label_new(_("Period time (ms):"));
	gtk_label_set_justify(GTK_LABEL(period_time_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(period_time_label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(buffer_table), period_time_label,
			 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	period_time_adj = gtk_adjustment_new(alsa_cfg.period_time,
					     1, 500, 1, 100, 100);
	period_time_spin = gtk_spin_button_new(GTK_ADJUSTMENT(period_time_adj),
					       8, 0);

	gtk_widget_set_usize(period_time_spin, 60, -1);
	gtk_table_attach(GTK_TABLE(buffer_table), period_time_spin,
			 1, 2, 1, 2, 0, 0, 0, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), advanced_vbox,
				 gtk_label_new(_("Advanced settings")));

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", (GCallback)configure_win_ok_cb, NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  (GCallback)gtk_widget_destroy, GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(configure_win);
}
