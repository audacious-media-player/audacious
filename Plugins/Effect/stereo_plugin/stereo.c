#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "libaudacious/util.h"

#include <audacious/plugin.h>
#include <libaudacious/configdb.h>

#include "../../../config.h"

static void init(void);
static void about(void);
static void configure(void);
static int mod_samples(gpointer *d, gint length, AFormat afmt, gint srate, gint nch);



EffectPlugin stereo_ep =
{
	NULL,
	NULL,
	NULL, /* Description */
	init,
	NULL,
	about,
	configure,
	mod_samples
};

static const char *about_text = N_("Extra Stereo Plugin\n\n"
                                   "By Johan Levin 1999.");

static GtkWidget *conf_dialog = NULL;
static gdouble value;

EffectPlugin *get_eplugin_info(void)
{
	stereo_ep.description =
		g_strdup_printf(_("Extra Stereo Plugin %s"), VERSION);
	return &stereo_ep;
}

static void init(void)
{
	ConfigDb *db;
	db = bmp_cfg_db_open();
	if (!bmp_cfg_db_get_double(db, "extra_stereo", "intensity", &value))
		value = 2.5;
	bmp_cfg_db_close(db);
}

static void about(void)
{
	static GtkWidget *about_dialog = NULL;
	
	if (about_dialog != NULL)
		return;

	about_dialog = xmms_show_message(_("About Extra Stereo Plugin"),
					 _(about_text), _("Ok"), FALSE,
					 NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(about_dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &about_dialog);
}

static void conf_ok_cb(GtkButton * button, gpointer data)
{
	ConfigDb *db;

	value = *(gdouble *) data;
	
	db = bmp_cfg_db_open();
	bmp_cfg_db_set_double(db, "extra_stereo", "intensity", value);
	bmp_cfg_db_close(db);
	gtk_widget_destroy(conf_dialog);
}

static void conf_cancel_cb(GtkButton * button, gpointer data)
{
	gtk_widget_destroy(conf_dialog);
}

static void conf_apply_cb(GtkButton *button, gpointer data)
{
	value = *(gdouble *) data;
}

static void configure(void)
{
	GtkWidget *hbox, *label, *scale, *button, *bbox;
	GtkObject *adjustment;

	if (conf_dialog != NULL)
		return;

	conf_dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(conf_dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &conf_dialog);
	gtk_window_set_title(GTK_WINDOW(conf_dialog), _("Configure Extra Stereo"));

	label = gtk_label_new(_("Effect intensity:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(conf_dialog)->vbox), label,
			   TRUE, TRUE, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(conf_dialog)->vbox), hbox,
			   TRUE, TRUE, 10);
	gtk_widget_show(hbox);

	adjustment = gtk_adjustment_new(value, 0.0, 15.0 + 1.0, 0.1, 1.0, 1.0);
	scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 10);
	gtk_widget_show(scale);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX((GTK_DIALOG(conf_dialog)->action_area)),
			   bbox, TRUE, TRUE, 0);

	button = gtk_button_new_with_label(_("Ok"));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(conf_ok_cb),
			   &GTK_ADJUSTMENT(adjustment)->value);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(conf_cancel_cb), NULL);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Apply"));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(conf_apply_cb),
			   &GTK_ADJUSTMENT(adjustment)->value);
	gtk_widget_show(button);

	gtk_widget_show(bbox);

	gtk_widget_show(conf_dialog);
}

static int mod_samples(gpointer *d, gint length, AFormat afmt, gint srate, gint nch)
{
	gint i;
	gdouble avg, ldiff, rdiff, tmp, mul;
	gint16  *data = (gint16 *)*d;

	if (!(afmt == FMT_S16_NE ||
	      (afmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) ||
	      (afmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN)) ||
	    nch != 2)
		return length;

	mul = value;
	
	for (i = 0; i < length / 2; i += 2)
	{
		avg = (data[i] + data[i + 1]) / 2;
		ldiff = data[i] - avg;
		rdiff = data[i + 1] - avg;

		tmp = avg + ldiff * mul;
		if (tmp < -32768)
			tmp = -32768;
		if (tmp > 32767)
			tmp = 32767;
		data[i] = tmp;

		tmp = avg + rdiff * mul;
		if (tmp < -32768)
			tmp = -32768;
		if (tmp > 32767)
			tmp = 32767;
		data[i + 1] = tmp;
	}
	return length;
}
