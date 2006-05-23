/*  
   xmms-sid - SIDPlay input plugin for X MultiMedia System (XMMS)

   Aboutbox dialog
   
   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "xmms-sid.h"
#include <gtk/gtk.h>
#include "xmms-sid-logo.xpm"


static GtkWidget *xs_aboutwin = NULL;


gint xs_about_ok(void)
{
	gtk_widget_destroy(xs_aboutwin);
	xs_aboutwin = NULL;
	return 0;
}


/*
 * Execute the about dialog
 */
void xs_about(void)
{
	GtkWidget *about_vbox1;
	GtkWidget *about_frame;
	GtkWidget *about_logo;
	GdkPixmap *about_logo_pixmap = NULL, *about_logo_mask = NULL;
	GtkWidget *about_scrwin;
	GtkWidget *about_text;
	GtkWidget *alignment6;
	GtkWidget *about_close;

	/* Check if there already is an open about window */
	if (xs_aboutwin != NULL) {
		gdk_window_raise(xs_aboutwin->window);
		return;
	}

	/* No, create one ... */
	xs_aboutwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW(xs_aboutwin), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_name(xs_aboutwin, "xs_aboutwin");
	g_object_set_data(G_OBJECT(xs_aboutwin), "xs_aboutwin", xs_aboutwin);
	gtk_window_set_title(GTK_WINDOW(xs_aboutwin), "About " PACKAGE_STRING);
	gtk_window_set_default_size(GTK_WINDOW(xs_aboutwin), 300, -1);

	about_vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_name(about_vbox1, "about_vbox1");
	gtk_widget_ref(about_vbox1);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_vbox1", about_vbox1,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_vbox1);
	gtk_container_add(GTK_CONTAINER(xs_aboutwin), about_vbox1);

	about_frame = gtk_frame_new(NULL);
	gtk_widget_set_name(about_frame, "about_frame");
	gtk_widget_ref(about_frame);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_frame", about_frame,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_frame);
	gtk_box_pack_start(GTK_BOX(about_vbox1), about_frame, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(about_frame), 4);

	gtk_frame_set_shadow_type(GTK_FRAME(about_frame), GTK_SHADOW_OUT);

	/* Create the Gdk data for logo pixmap */
	gtk_widget_realize(xs_aboutwin);
	about_logo_pixmap = gdk_pixmap_create_from_xpm_d(xs_aboutwin->window,
							 &about_logo_mask, NULL, (gchar **) xmms_sid_logo_xpm);

	about_logo = gtk_pixmap_new(about_logo_pixmap, about_logo_mask);

	/* Create logo widget */
	gtk_widget_set_name(about_logo, "about_logo");
	gtk_widget_ref(about_logo);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_logo", about_logo,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_logo);
	gtk_container_add(GTK_CONTAINER(about_frame), about_logo);
	gtk_misc_set_padding(GTK_MISC(about_logo), 0, 6);

	about_scrwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_name(about_scrwin, "about_scrwin");
	gtk_widget_ref(about_scrwin);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_scrwin", about_scrwin,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_scrwin);
	gtk_box_pack_start(GTK_BOX(about_vbox1), about_scrwin, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(about_scrwin), 8);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(about_scrwin), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	about_text = gtk_text_view_new();
	gtk_widget_set_name(about_text, "about_text");
	gtk_widget_ref(about_text);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_text", about_text,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_text);
	gtk_container_add(GTK_CONTAINER(about_scrwin), about_text);
	gtk_widget_set_usize(about_text, -2, 100);
	gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(about_text))),
			"\n"
			"(C) Copyright 1999-2005\n"
			"\tTecnic Software productions (TNSP)\n"
			"\n" "Programming and design\n" "\tMatti 'ccr' H\303\244m\303\244l\303\244inen\n" "\n"
#ifdef HAVE_SIDPLAY1
			"libSIDPlay1 created by\n" "\tMichael Schwendt\n" "\n"
#endif
#ifdef HAVE_SIDPLAY2
			"libSIDPlay2 and reSID created by\n"
			"\tSimon White, Dag Lem,\n" "\tMichael Schwendt and rest.\n" "\n"
#endif
			"BMP-SID and Audacious port written by\n"
			"\tGiacomo Lozito from develia.org\n"
			"\n"
			"Original XMMS-SID (v0.4) by\n" "\tWillem Monsuwe\n" "\n"
			"Greetings fly out to ...\n"
			"\tEveryone at #Linux.Fi, #Fireball,\n"
			"\t#TNSP and #c-64 of IRCNet, #xmms\n"
			"\tof Freenode.net.\n"
			"\n"
			"\tDekadence, PWP, Byterapers,\n"
			"\tmfx, Unique, Fairlight, iSO,\n"
			"\tWrath Designs, Padua, Extend,\n"
			"\tPHn, Creators, Cosine, tAAt,\n" "\tViruz, Crest and Skalaria.\n" "\n"
			"Special thanks\n"
			"\tGerfried 'Alfie' Fuchs\n"
			"\tAndreas 'mrsid' Varga\n" "\tAll the betatesters.\n" "\tAll the users!\n",
			-1);

	alignment6 = gtk_alignment_new(0.5, 0.5, 0.18, 1);
	gtk_widget_set_name(alignment6, "alignment6");
	gtk_widget_ref(alignment6);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "alignment6", alignment6,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(alignment6);
	gtk_box_pack_start(GTK_BOX(about_vbox1), alignment6, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(alignment6), 8);

	about_close = gtk_button_new_with_label("Close");
	gtk_widget_set_name(about_close, "about_close");
	gtk_widget_ref(about_close);
	g_object_set_data_full(G_OBJECT(xs_aboutwin), "about_close", about_close,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show(about_close);
	gtk_container_add(GTK_CONTAINER(alignment6), about_close);
	GTK_WIDGET_SET_FLAGS(about_close, GTK_CAN_DEFAULT);

	gtk_signal_connect(GTK_OBJECT(about_close), "clicked", GTK_SIGNAL_FUNC(xs_about_ok), NULL);

	gtk_widget_show(xs_aboutwin);
}
