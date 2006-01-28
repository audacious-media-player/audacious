/* LIRC Plugin

   Copyright (C) 2005 Audacious development team

   Copyright (c) 1998-1999 Carl van Schaik (carl@leg.uct.ac.za)
   code from gtuner lirc plugin
   cRadio, kTuner, gtuner (c) 1998-1999 Carl van Schaik
   
   Copyright (C) 2000 Christoph Bartelmus (xmms@bartelmus.de)
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>

#include "lirc.h"
#include "lirc_image.h"
#include "lirc_mini.xpm"

#include <gdk/gdkx.h>

static void win_set_icon (GtkWidget *win, char** image);

static GtkWidget *dialog = NULL;
static GdkPixmap *icon;
static GdkBitmap *icon_mask;

void about_close_cb(GtkWidget *w,gpointer data)
{
	gtk_widget_destroy(dialog);
	gdk_pixmap_unref(icon);
	gdk_bitmap_unref(icon_mask);
}

void about(void)
{
        GdkPixmap *pixmap;
	GtkWidget *bbox,*about_credits_logo_box,*about_credits_logo_frame;
	GtkWidget *about_credits_logo;
	GtkWidget *button,*label;
        GString *logo_text;
	
	if(dialog) return;
	
	dialog=gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),
			     "About LIRC xmms-plugin " VERSION);
	g_signal_connect(G_OBJECT(dialog),"destroy",
			   G_CALLBACK(gtk_widget_destroyed),
			   &dialog);
	gtk_widget_realize(dialog);
	
	pixmap=gdk_pixmap_create_from_xpm_d(dialog->window,
					    NULL, NULL, lirc_image);
	
	about_credits_logo_box = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   about_credits_logo_box, FALSE, FALSE, 0);
	about_credits_logo_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
				  GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
			   about_credits_logo_frame, FALSE, FALSE, 0);
	
	about_credits_logo = gtk_pixmap_new(pixmap, NULL);
	gdk_pixmap_unref(pixmap);
	
	gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
			  about_credits_logo);
	
	gtk_container_border_width(GTK_CONTAINER(dialog),5);

        logo_text = g_string_new( "" );
        g_string_append( logo_text , "LIRC Plugin " );
        g_string_append( logo_text , VERSION );
        g_string_append( logo_text , "\n"
"A simple plugin that lets you control\n"
"audacious using the LIRC remote control daemon\n\n"
"Adapted for audacious usage by Tony Vroon <chainsaw@gentoo.org>\n"
"from the XMMS LIRC plugin by:\n"
"Carl van Schaik <carl@leg.uct.ac.za>\n"
"Christoph Bartelmus <xmms@bartelmus.de>\n"
"You can get LIRC information at:\n"
"http://fsinfo.cs.uni-sb.de/~columbus/lirc/index.html" );
	
	label=gtk_label_new( logo_text->str );
        g_string_free( logo_text, TRUE );

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,
                           TRUE,TRUE,10);
	
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   bbox, FALSE, FALSE, 0);
	
	button = gtk_button_new_with_label(("Close"));
	g_signal_connect_object(G_OBJECT(button), "clicked",
				  G_CALLBACK(about_close_cb), NULL,G_CONNECT_SWAPPED) ;
	
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_grab_focus(button);
	
        win_set_icon(dialog, lirc_mini_xpm);
	
	gtk_widget_show_all(dialog);
}

static void win_set_icon (GtkWidget *win, char** image)
{
	GdkAtom icon_atom;
	glong data[2];
	
	icon=gdk_pixmap_create_from_xpm_d(win->window, &icon_mask,
					  &win->style->bg[GTK_STATE_NORMAL],
					  image);
	data[0] = GDK_WINDOW_XWINDOW(icon);
	data[1] = GDK_WINDOW_XWINDOW(icon_mask);
	
	icon_atom = gdk_atom_intern ("KWM_WIN_ICON", FALSE);
	gdk_property_change (win->window, icon_atom, icon_atom, 32,
			     GDK_PROP_MODE_REPLACE, (guchar *)data, 2);
}
