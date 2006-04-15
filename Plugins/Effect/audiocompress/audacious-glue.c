/* audacious-glue.c
** Audacious effect plugin for AudioCompress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "audacious/plugin.h"
#include "libaudacious/configdb.h"

#include "config.h"
#include "compress.h"

typedef struct {
	gboolean anticlip;
	gint target;
	gint gainmax;
	gint gainsmooth;
	gint buckets;

	GtkWidget *dialog;
} CompressorPrefs;

void initPrefs(CompressorPrefs *prefs);
void freePrefs(CompressorPrefs *prefs);
void showPrefs(CompressorPrefs *prefs);
void savePrefs(CompressorPrefs *prefs);

static CompressorPrefs prefs;
static void myInit(void);
static void myCleanup(void);
static void myAbout(void);
static void myPrefs(void);
static int myModify(gpointer * data, gint length, AFormat fmt,
		    gint srate, gint nch);


static EffectPlugin xmms_plugin = {
	NULL, NULL,
	"AudioCompressor AGC plugin",
	myInit,
	myCleanup,
	myAbout,
	myPrefs,
	myModify,
 	NULL
};

EffectPlugin *get_eplugin_info(void)
{
	return &xmms_plugin;
}

void myInit(void)
{
	static int inited = 0;
	if (!inited)
	{
		initPrefs(&prefs);
		CompressCfg(prefs.anticlip,
			    prefs.target,
			    prefs.gainmax,
			    prefs.gainsmooth,
			    prefs.buckets);
	}
	inited = 1;
}

void myCleanup(void)
{
	savePrefs(&prefs);
	freePrefs(&prefs);
	CompressFree();
}

int myModify(gpointer * data, gint length, AFormat fmt, gint srate, gint nch)
{
	if (fmt == FMT_S16_NE ||
	    (fmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) ||
	    (fmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN))
		CompressDo(*data, length);

	return length;
}

void closeAbout(GtkWidget* wg, GtkWidget* win)
{
	
	gtk_widget_destroy(GTK_WIDGET(win));
}

void myAbout(void)
{
	gchar *window_title = "About AudioCompress";
	gchar *about_text =
		"AudioCompress " ACVERSION "\n"
                "(c)2003 trikuare studios(http://trikuare.cx)\n"
                "Ported to Audacious by Tony Vroon (chainsaw@gentoo.org)\n\n"
                "Simple dynamic range compressor for transparently\n"
                "keeping the volume level more or less consistent";

	GtkWidget *about_xmms_compress;
	GtkWidget *vbox1;
	GtkWidget *label1;
	GtkWidget *hseparator1;
	GtkWidget *hbuttonbox1;
	GtkWidget *button1;

	about_xmms_compress = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(about_xmms_compress),
				       10);
	gtk_window_set_title(GTK_WINDOW(about_xmms_compress), window_title);
	gtk_window_set_wmclass(GTK_WINDOW(about_xmms_compress), "about",
			       "xmms");

	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(about_xmms_compress), vbox1);

	label1 = gtk_label_new(about_text);
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(vbox1), label1, FALSE, FALSE, 0);

	hseparator1 = gtk_hseparator_new();
	gtk_widget_show(hseparator1);
	gtk_box_pack_start(GTK_BOX(vbox1), hseparator1, TRUE, TRUE, 0);

	hbuttonbox1 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox1, TRUE, TRUE, 0);

	button1 = gtk_button_new_with_label("Ok");
	gtk_widget_show(button1);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), button1);
	GTK_WIDGET_SET_FLAGS(button1, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button1);

	gtk_signal_connect(GTK_OBJECT(button1), "clicked",
			   GTK_SIGNAL_FUNC(closeAbout),
			   about_xmms_compress);

	gtk_widget_show(GTK_WIDGET(about_xmms_compress));
}

void myPrefs(void)
{
	myInit();
	showPrefs(&prefs);
}


GtkWidget *create_prefs_dialog(CompressorPrefs * prefs);

/* Manage preferences */

void initPrefs(CompressorPrefs * prefs)
{
	ConfigDb *db;

	prefs->anticlip = ANTICLIP;
	prefs->target = TARGET;
	prefs->gainmax = GAINMAX;
	prefs->gainsmooth = GAINSMOOTH;
	prefs->buckets = BUCKETS;

	db = bmp_cfg_db_open();

	bmp_cfg_db_get_bool(db, "AudioCompress",
			       "anticlip", &prefs->anticlip);
	bmp_cfg_db_get_int(db, "AudioCompress", "target",
			   &prefs->target);
	bmp_cfg_db_get_int(db, "AudioCompress", "gainmax",
			   &prefs->gainmax);
	bmp_cfg_db_get_int(db, "AudioCompress",
			   "gainsmooth", &prefs->gainsmooth);
	bmp_cfg_db_get_int(db, "AudioCompress", "buckets",
			   &prefs->buckets);

	prefs->dialog = create_prefs_dialog(prefs);

	bmp_cfg_db_close(db);
}

void freePrefs(CompressorPrefs * prefs)
{
	gtk_widget_destroy(prefs->dialog);

}

void savePrefs(CompressorPrefs * prefs)
{
	ConfigDb *db;

	db = bmp_cfg_db_open();

	bmp_cfg_db_set_bool(db, "AudioCompress", "anticlip",
				prefs->anticlip);
	bmp_cfg_db_set_int(db, "AudioCompress", "target",
			    prefs->target);
	bmp_cfg_db_set_int(db, "AudioCompress", "gainmax",
			    prefs->gainmax);
	bmp_cfg_db_set_int(db, "AudioCompress", "gainsmooth",
			    prefs->gainsmooth);
	bmp_cfg_db_set_int(db, "AudioCompress", "buckets",
			    prefs->buckets);

	bmp_cfg_db_close(db);
}


/* Dialogs */

GtkWidget *lookup_widget(GtkWidget * widget, const gchar * widget_name)
{
	GtkWidget *parent, *found_widget;

	for (;;)
	{
		if (GTK_IS_MENU(widget))
			parent = gtk_menu_get_attach_widget(GTK_MENU
							    (widget));
		else
			parent = GTK_WIDGET(widget)->parent;
		if (parent == NULL)
			break;
		widget = parent;
	}

	found_widget =(GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget),
							widget_name);
	if (!GTK_IS_WIDGET(found_widget))
		g_warning("Widget not found: %s", widget_name);
	return found_widget;
}


/* Preferences dialog callbacks */

void close_window(GtkWidget * button, GtkWidget * win)
{
	gtk_widget_hide(GTK_WIDGET(win));
}

void on_apply_preferences_clicked(GtkWidget * wg, CompressorPrefs * prefs)
{
	/* Read spinbuttons values and update variables */

	GtkWidget *look;

	look = lookup_widget(prefs->dialog, "anticlip");
	prefs->anticlip = GTK_TOGGLE_BUTTON(look)->active;

	look = lookup_widget(prefs->dialog, "target");
	prefs->target =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(look));

	look = lookup_widget(prefs->dialog, "gainmax");
	prefs->gainmax =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(look));

	look = lookup_widget(prefs->dialog, "gainsmooth");
	prefs->gainsmooth =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(look));

	look = lookup_widget(prefs->dialog, "buckets");
	prefs->buckets =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(look));

	savePrefs(prefs);
	CompressCfg(prefs->anticlip, prefs->target, prefs->gainmax,
		    prefs->gainsmooth, prefs->buckets);
}

void
on_load_default_values_clicked(GtkWidget * wg, CompressorPrefs * prefs)
{

	/* When the button is pressed, sets the values from config.h */

	GtkWidget *look;

	look = lookup_widget(prefs->dialog, "anticlip");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(look), FALSE);
	
	look = lookup_widget(prefs->dialog, "target");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),(gfloat)TARGET);

	look = lookup_widget(prefs->dialog, "gainmax");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),(gfloat)GAINMAX);

	look = lookup_widget(prefs->dialog, "gainsmooth");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),
				  (gfloat)GAINSMOOTH);

	look = lookup_widget(prefs->dialog, "buckets");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),(gfloat)BUCKETS);

}

void
on_ok_preferences_clicked(GtkWidget * wg, CompressorPrefs * prefs)
{

	/* The same as apply, but also closes the window */

	on_apply_preferences_clicked(NULL, prefs);
	close_window(NULL, prefs->dialog);

}

/* Create / show prefs */

void showPrefs(CompressorPrefs * prefs)
{

	GtkWidget *look;

	/* Set values */ 
	
	look = lookup_widget(prefs->dialog, "anticlip");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(look),
				     prefs->anticlip);

	look = lookup_widget(prefs->dialog, "target");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look), prefs->target);

	look = lookup_widget(prefs->dialog, "gainmax");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),
				  (gfloat)prefs->gainmax);

	look = lookup_widget(prefs->dialog, "gainsmooth");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),
				  (gfloat)prefs->gainsmooth);

	look = lookup_widget(prefs->dialog, "buckets");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(look),
				  (gfloat)prefs->buckets);

	gtk_widget_show(prefs->dialog);

}

GtkWidget *
create_prefs_dialog(CompressorPrefs * prefs)
{

	/* Preferences dialog */

	gchar *clip_tip =
		"If checked, when the sound peaks the volume will be "
		"cut instantly; otherwise, it will ramp down just in time "
		"for the peak (but some minor clipping may still occur).";
	gchar *gainmax_tip =
		"The maximum amount to amplify the audio by";
	gchar *gainsmooth_tip =
		"Defines how smoothly the volume will ramp up";
	gchar *target_tip =
		"The target audio level for ramping up. Lowering the "
		"value gives a bit more dynamic range for peaks, but "
		"will make the overall sound quieter.";
	gchar *hist_text = "How long of a window to maintain";

	GtkWidget *preferences;
	GtkWidget *vbox2;
	GtkWidget *notebook1;
	GtkWidget *vbox6;
	GtkWidget *vbox5;
	GtkWidget *frame1;
	GtkWidget *clipping;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkObject *gainmax_adj;
	GtkWidget *gainmax_sp;
	GtkObject *gainsmooth_adj;
	GtkWidget *gainsmooth_sp;
	GtkObject *target_adj;
	GtkWidget *target_sp;
	GtkWidget *label4;
	GtkWidget *hbuttonbox2;
	GtkWidget *hbuttonbox7;
	GtkWidget *button3;
	GtkWidget *button4;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *frame3;
	GtkWidget *table3;
	GtkWidget *label9;
	GtkWidget *buckets_sp;
	GtkWidget *hseparator3;
	GtkObject *buckets_adj;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new();

	preferences = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data(GTK_OBJECT(preferences), "preferences",
			     preferences);
	gtk_container_set_border_width(GTK_CONTAINER(preferences), 10);
	gtk_window_set_title(GTK_WINDOW(preferences),
			      "AudioCompress preferences");
	gtk_window_set_policy(GTK_WINDOW(preferences), TRUE, TRUE, FALSE);
	gtk_window_set_wmclass(GTK_WINDOW(preferences), "prefs", "xmms");

	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(preferences), vbox2);

	notebook1 = gtk_notebook_new();
	gtk_widget_show(notebook1);
	gtk_box_pack_start(GTK_BOX(vbox2), notebook1, TRUE, TRUE, 0);

	vbox6 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox6);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox6);

	vbox5 = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox5);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox5), 5);

	frame1 = gtk_frame_new(" Quality Options ");
	gtk_widget_show(frame1);
	gtk_box_pack_start(GTK_BOX(vbox5), frame1, FALSE, FALSE, 0);

	clipping = gtk_check_button_new_with_label
		(" Aggressively prevent clipping");
	gtk_widget_ref(clipping);
	gtk_object_set_data_full(GTK_OBJECT(preferences), "anticlip",
				  clipping,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clipping),
				      prefs->anticlip);
	gtk_widget_show(clipping);
	gtk_container_add(GTK_CONTAINER(frame1), clipping);
	gtk_container_set_border_width(GTK_CONTAINER(clipping), 5);
	gtk_tooltips_set_tip(tooltips, clipping, clip_tip, NULL);

	frame2 = gtk_frame_new(" Target & gain");
	gtk_widget_show(frame2);
	gtk_box_pack_start(GTK_BOX(vbox5), frame2, TRUE, TRUE, 0);

	table1 = gtk_table_new(4, 2, TRUE);
	gtk_widget_show(table1);
	gtk_container_add(GTK_CONTAINER(frame2), table1);
	gtk_container_set_border_width(GTK_CONTAINER(table1), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table1), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table1), 5);

	label5 = gtk_label_new("Target audio level:");
	gtk_widget_show(label5);
	gtk_table_attach(GTK_TABLE(table1), label5, 0, 1, 0, 1,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label5), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label5), 0, 0.5);

	label6 = gtk_label_new("Maximum gain:");
	gtk_widget_show(label6);
	gtk_table_attach(GTK_TABLE(table1), label6, 0, 1, 1, 2,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label6), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label6), 0, 0.5);

	label7 = gtk_label_new("Gain smooth:");
	gtk_widget_show(label7);
	gtk_table_attach(GTK_TABLE(table1), label7, 0, 1, 2, 3,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label7), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label7), 0, 0.5);

	gainmax_adj =
		gtk_adjustment_new(prefs->gainmax, 0,
				   1 << (31 - GAINSHIFT), 1, 2, 5);
	gainmax_sp = gtk_spin_button_new(GTK_ADJUSTMENT(gainmax_adj), 1, 0);
	gtk_widget_ref(gainmax_sp);
	gtk_object_set_data_full(GTK_OBJECT(preferences), "gainmax",
				  gainmax_sp,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(gainmax_sp);
	gtk_table_attach(GTK_TABLE(table1), gainmax_sp, 1, 2, 1, 2,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_tooltips_set_tip(tooltips, gainmax_sp, gainmax_tip, NULL);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gainmax_sp), TRUE);

	gainsmooth_adj =
		gtk_adjustment_new(prefs->gainsmooth, 0, GAINSHIFT - 1,
				   1, 1, 1);
	gainsmooth_sp =
		gtk_spin_button_new(GTK_ADJUSTMENT(gainsmooth_adj), 1, 0);
	gtk_widget_ref(gainsmooth_sp);
	gtk_object_set_data_full(GTK_OBJECT(preferences), "gainsmooth",
				  gainsmooth_sp,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(gainsmooth_sp);
	gtk_table_attach(GTK_TABLE(table1), gainsmooth_sp, 1, 2, 2, 3,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_tooltips_set_tip(tooltips, gainsmooth_sp, gainsmooth_tip, NULL);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gainsmooth_sp), TRUE);

	target_adj = gtk_adjustment_new(prefs->target, 0, 100000, 1, 10, 10);
	target_sp = gtk_spin_button_new(GTK_ADJUSTMENT(target_adj), 1, 0);
	gtk_widget_ref(target_sp);
	gtk_object_set_data_full(GTK_OBJECT(preferences), "target",
				  target_sp,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(target_sp);
	gtk_table_attach(GTK_TABLE(table1), target_sp, 1, 2, 0, 1,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_tooltips_set_tip(tooltips, target_sp, target_tip, NULL);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(target_sp), TRUE);

	frame3 = gtk_frame_new(" History ");
	gtk_widget_show(frame3);
	gtk_box_pack_start(GTK_BOX(vbox5), frame3, TRUE, TRUE, 0);

	table3 = gtk_table_new(1, 2, TRUE);
	gtk_widget_show(table3);
	gtk_container_add(GTK_CONTAINER(frame3), table3);
	gtk_container_set_border_width(GTK_CONTAINER(table3), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table3), 5);

	/* Buckets label and spin */

	label9 = gtk_label_new(hist_text);
	gtk_widget_show(label9);
	gtk_table_attach(GTK_TABLE(table3), label9, 0, 1, 0, 1,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label9), 7.45058e-09, 0.5);

	buckets_adj =
		gtk_adjustment_new(prefs->buckets, 1, 10000, 1, 10, 10);
	buckets_sp = gtk_spin_button_new(GTK_ADJUSTMENT(buckets_adj), 1, 0);
	gtk_widget_ref(buckets_sp);
	gtk_object_set_data_full(GTK_OBJECT(preferences), "buckets",
				  buckets_sp,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(buckets_sp);
	gtk_table_attach(GTK_TABLE(table3), buckets_sp, 1, 2, 0, 1,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);

	gtk_tooltips_set_tip(tooltips, buckets_sp,
			     "How long of a history to maintain.  "
			     "A higher number will make the volume changes "
			     "less responsive.",
			      NULL);

	hbuttonbox7 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox7);
	gtk_box_pack_start(GTK_BOX(vbox5), hbuttonbox7, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox7),
				   GTK_BUTTONBOX_END);

	button8 = gtk_button_new_with_label("Load default values");
	gtk_widget_show(button8);
	gtk_container_add(GTK_CONTAINER(hbuttonbox7), button8);
	GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

	label4 = gtk_label_new("Audio values");
	gtk_widget_show(label4);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1),
				    gtk_notebook_get_nth_page(GTK_NOTEBOOK
							      (notebook1),
							       1), label4);

	hseparator3 = gtk_hseparator_new();
	gtk_widget_show(hseparator3);
	gtk_box_pack_start(GTK_BOX(vbox2), hseparator3, TRUE, TRUE, 0);

	hbuttonbox2 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox2);
	gtk_box_pack_start(GTK_BOX(vbox2), hbuttonbox2, TRUE, TRUE, 0);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbuttonbox2), 5);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(hbuttonbox2), 80, 0);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(hbuttonbox2), 0,
					   0);

	button3 = gtk_button_new_with_label("Ok");
	gtk_widget_show(button3);
	gtk_container_add(GTK_CONTAINER(hbuttonbox2), button3);
	GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);

	button4 = gtk_button_new_with_label("Cancel");
	gtk_widget_show(button4);
	gtk_container_add(GTK_CONTAINER(hbuttonbox2), button4);
	GTK_WIDGET_SET_FLAGS(button4, GTK_CAN_DEFAULT);

	button7 = gtk_button_new_with_label("Apply");
	gtk_widget_show(button7);
	gtk_container_add(GTK_CONTAINER(hbuttonbox2), button7);
	GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

	gtk_widget_grab_default(button4);

	gtk_signal_connect(GTK_OBJECT(button3), "clicked",
			    GTK_SIGNAL_FUNC(on_ok_preferences_clicked),
			    prefs);
	gtk_signal_connect(GTK_OBJECT(button4), "clicked",
			    GTK_SIGNAL_FUNC(close_window), preferences);
	gtk_signal_connect(GTK_OBJECT(button7), "clicked",
			    GTK_SIGNAL_FUNC(on_apply_preferences_clicked),
			    prefs);
	gtk_signal_connect(GTK_OBJECT(button8), "clicked",
			    GTK_SIGNAL_FUNC(on_load_default_values_clicked),
			    prefs);

	return preferences;
}
