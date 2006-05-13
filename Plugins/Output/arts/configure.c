/*
 *  aRts ouput plugin for xmms
 *
 *  Copyright (C) 2000,2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licenced under GNU GPL version 2.
 *
 *  Audacious port by Giacomo Lozito from develia.org
 *
 */

#include "arts.h"
#define _(string) (string)

#include <gtk/gtk.h>

static GtkWidget *configure_win = NULL;
static GtkWidget *buffer_size_spin;

static void configure_win_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigDb *db;

	artsxmms_cfg.buffer_size =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(buffer_size_spin));
	
	db = bmp_cfg_db_open();
	bmp_cfg_db_set_int(db, "arts", "buffer_size", artsxmms_cfg.buffer_size);
	bmp_cfg_db_close(db);

	gtk_widget_destroy(configure_win);
}


void artsxmms_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *buffer_frame, *buffer_vbox, *buffer_table;
	GtkWidget *buffer_size_box, *buffer_size_label;
	GtkWidget *bbox, *ok, *cancel;

	GtkObject *buffer_size_adj;

	if (configure_win)
		return;
	
	configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint( GTK_WINDOW(configure_win), GDK_WINDOW_TYPE_HINT_DIALOG );
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win), _("aRts Driver configuration"));
	gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(configure_win), 10);
	
	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), vbox);
	
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	
	buffer_frame = gtk_frame_new(_("Buffering:"));
	gtk_container_set_border_width(GTK_CONTAINER(buffer_frame), 5);
	
	buffer_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(buffer_frame), buffer_vbox);
	
	buffer_table = gtk_table_new(2, 1, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(buffer_table), 5);
	gtk_box_pack_start(GTK_BOX(buffer_vbox), buffer_table, FALSE, FALSE, 0);
	
	buffer_size_box = gtk_hbox_new(FALSE, 5);
	gtk_table_attach_defaults(GTK_TABLE(buffer_table), buffer_size_box, 0, 1, 0, 1);
	buffer_size_label = gtk_label_new(_("Buffer size (ms):"));
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_label, FALSE, FALSE, 0);
	gtk_widget_show(buffer_size_label);
	buffer_size_adj = gtk_adjustment_new(artsxmms_cfg.buffer_size,
					     200, 10000, 100, 100, 100);
	buffer_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(buffer_size_adj), 8, 0);
	gtk_widget_set_usize(buffer_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(buffer_size_box), buffer_size_spin, FALSE, FALSE, 0);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buffer_frame,
				 gtk_label_new(_("Buffering")));
	
	bbox = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	
	ok = gtk_button_new_with_label(_("Ok"));
	cancel = gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(configure_win));
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
	
	gtk_widget_show_all(configure_win);
}
