/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2000-2003  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "credits.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static GtkWidget *about_window = NULL;
static GdkPixbuf *about_pixbuf = NULL;
static GdkPixmap *mask_pixmap_window1 = NULL,
        *mask_pixmap_window2 = NULL;
static GdkBitmap *mask_bitmap_window1 = NULL,
        *mask_bitmap_window2 = NULL;
    
static gboolean
on_about_window_expose(GtkWidget *widget, GdkEventExpose *expose, gpointer data)
{
	g_return_val_if_fail(GTK_IS_WIDGET (widget), FALSE);

	gdk_window_set_back_pixmap(GDK_WINDOW(widget->window), mask_pixmap_window2, 0);
	gdk_window_clear(GDK_WINDOW(widget->window));

	return FALSE;
}

static gboolean
on_about_window_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	g_return_val_if_fail(GTK_IS_WIDGET (widget), FALSE);

	if (event->keyval == GDK_Escape)
	{      
		gtk_widget_hide(widget);
	}

	return FALSE;
}

static gboolean
on_close_button_clicked (GtkWidget *widget, gpointer data)
{
	g_return_val_if_fail(GTK_IS_WIDGET (widget), FALSE);

	gtk_widget_hide(about_window);

	return FALSE;
}

static gboolean
on_credits_button_clicked (GtkWidget *widget, gpointer data)
{
	g_return_val_if_fail(GTK_IS_WIDGET (widget), FALSE);

	show_credits_window();

	return FALSE;
}

void
show_about_window(void)
{
    GtkWidget *about_fixedbox;
    GtkWidget *close_button;
    GtkWidget *credits_button , *credits_button_hbox, *credits_button_image, *credits_button_label;
    gchar *filename = DATA_DIR G_DIR_SEPARATOR_S "images" G_DIR_SEPARATOR_S "about-logo.png";

    if (about_window != NULL)
    {
        gtk_window_present(GTK_WINDOW(about_window));
        return;
    }

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(about_window),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    g_signal_connect(about_window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &about_window);

    gtk_widget_realize(about_window);

    about_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    gtk_widget_set_size_request(GTK_WIDGET (about_window),
                   gdk_pixbuf_get_width (about_pixbuf),
                   gdk_pixbuf_get_height (about_pixbuf));

    gtk_widget_set_app_paintable(about_window, TRUE);
    gtk_window_set_title(GTK_WINDOW(about_window), _("About Audacious"));
    gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(about_window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(about_window), FALSE);

    gdk_pixbuf_render_pixmap_and_mask(about_pixbuf,
                     &mask_pixmap_window1,
                     &mask_bitmap_window1,
                     0);

    gdk_pixbuf_render_pixmap_and_mask(about_pixbuf,
                     &mask_pixmap_window2,
                     &mask_bitmap_window2,
                     128);

    gtk_widget_add_events(about_window, GDK_ALL_EVENTS_MASK);

    g_signal_connect(about_window, "expose-event",
	G_CALLBACK(on_about_window_expose), &about_window);

    g_signal_connect(about_window, "key-press-event",
	G_CALLBACK(on_about_window_key_press), &about_window);

#if 0
    GdkPixmap *beep_logo_pmap = NULL, *beep_logo_mask = NULL;
    GtkWidget *about_vbox;
    GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
    GtkWidget *about_credits_logo;
    GtkWidget *about_notebook;
    GtkWidget *list;
    GtkWidget *bbox, *close_btn;
    GtkWidget *label;
    gchar *text;

    if (about_window)
        return;

    gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);


    about_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(about_window), about_vbox);

    if (!beep_logo_pmap)
        beep_logo_pmap =
            gdk_pixmap_create_from_xpm_d(about_window->window,
                                         &beep_logo_mask, NULL, audacious_logo_xpm);

    about_credits_logo_box = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
                       FALSE, FALSE, 0);

    about_credits_logo_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
                              GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
                       about_credits_logo_frame, FALSE, FALSE, 0);

    about_credits_logo = gtk_pixmap_new(beep_logo_pmap, beep_logo_mask);
    gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
                      about_credits_logo);

    label = gtk_label_new(NULL);
    text = g_strdup_printf(_(bmp_brief), VERSION);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    g_free(text);

    gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);

    about_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);

    list = generate_credit_list(credit_text, TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Credits")));

    list = generate_credit_list(translators, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Translators")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

    close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(close_btn, "clicked",
                             G_CALLBACK(gtk_widget_destroy), about_window);
    GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
    gtk_widget_grab_default(close_btn);
#endif

    gtk_widget_shape_combine_mask(GTK_WIDGET(about_window), mask_bitmap_window2, 0, 0);

    /* GtkFixed hasn't got its GdkWindow, this means that it can be used to
       display widgets while the logo below will be displayed anyway;
       however I don't like the fixed position cause the button sizes may (will)
       vary depending on the gtk style used, so it's not possible to center
       them unless I force a fixed width and heigth (and this may bring to cutted
       text if someone, i.e., uses a big font for gtk widgets);
       other types of container most likely have their GdkWindow, this simply
       means that the logo must be drawn on the container widget, instead of the
       window; otherwise, it won't be displayed correctly */
    about_fixedbox = gtk_fixed_new();
    gtk_container_add( GTK_CONTAINER(about_window) , about_fixedbox );

    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

    g_signal_connect(close_button, "clicked",
	G_CALLBACK(on_close_button_clicked), NULL);

    gtk_fixed_put( GTK_FIXED(about_fixedbox) , close_button , 390 , 220 );

    credits_button = gtk_button_new();
    credits_button_hbox = gtk_hbox_new( FALSE , 0 );
    credits_button_image = gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO , GTK_ICON_SIZE_BUTTON );
    credits_button_label = gtk_label_new( "Credits" );
    gtk_box_pack_start( GTK_BOX(credits_button_hbox) , credits_button_image ,
                        TRUE , TRUE , 0 );
    gtk_box_pack_start( GTK_BOX(credits_button_hbox) , credits_button_label ,
                        TRUE , TRUE , 5 );
    gtk_container_add( GTK_CONTAINER(credits_button) , credits_button_hbox );

    g_signal_connect(credits_button, "clicked",
	G_CALLBACK(on_credits_button_clicked), NULL);

    gtk_fixed_put( GTK_FIXED(about_fixedbox) , credits_button , 60 , 220 );

    gtk_widget_show_all(about_window);
    gtk_window_present(GTK_WINDOW(about_window));
}
