/*
 *  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "ui_credits.h"
#include "plugin.h"

#include "platform/smartinclude.h"

static GtkWidget *about_window = NULL;
static GdkPixbuf *about_pixbuf = NULL;
static GdkPixmap *mask_pixmap_window1 = NULL,
        *mask_pixmap_window2 = NULL;
static GdkBitmap *mask_bitmap_window1 = NULL,
        *mask_bitmap_window2 = NULL;

static gboolean
on_about_window_expose(GtkWidget *widget, GdkEventExpose *expose, gpointer data)
{
	g_return_val_if_fail(widget != NULL, FALSE);
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

	audgui_show_credits_window();

	return FALSE;
}

void
audgui_show_about_window(void)
{
    GtkWidget *about_fixedbox;
    GtkWidget *close_button;
    GtkWidget *credits_button , *credits_button_hbox, *credits_button_image, *credits_button_label;
    GtkWidget *brief_label;
    gchar *filename = DATA_DIR G_DIR_SEPARATOR_S "images" G_DIR_SEPARATOR_S "about-logo.png";
    gchar *text;
    PangoAttrList *brief_label_attrs;
    PangoAttribute *brief_label_foreground;
    static const gchar *audacious_brief;

    if (about_window != NULL)
    {
        gtk_window_present(GTK_WINDOW(about_window));
        return;
    }

    aud_get_audacious_credits(&audacious_brief, NULL, NULL);

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

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

    gtk_widget_shape_combine_mask(GTK_WIDGET(about_window), mask_bitmap_window2, 0, 0);

    /* GtkFixed hasn't got its GdkWindow, this means that it can be used to
       display widgets while the logo below will be displayed anyway;
       however fixed positions are not that great, cause the button sizes may (will)
       vary depending on the gtk style used, so it's not possible to center
       them unless a fixed width and heigth is forced (and this may bring to cutted
       text if someone, i.e., uses a big font for gtk widgets);
       other types of container most likely have their GdkWindow, this simply
       means that the logo must be drawn on the container widget, instead of the
       window; otherwise, it won't be displayed correctly */
    about_fixedbox = gtk_fixed_new();
    gtk_container_add( GTK_CONTAINER(about_window) , about_fixedbox );

    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

    g_signal_connect(close_button, "clicked",
	G_CALLBACK(on_close_button_clicked), NULL);

    gtk_fixed_put( GTK_FIXED(about_fixedbox) , close_button , 375 , 220 );
    gtk_widget_set_size_request( close_button , 100 , -1 );

    credits_button = gtk_button_new();
    credits_button_hbox = gtk_hbox_new( FALSE , 0 );
    credits_button_image = gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO , GTK_ICON_SIZE_BUTTON );
    gtk_misc_set_alignment( GTK_MISC(credits_button_image) , 1 , 0.5 );
    credits_button_label = gtk_label_new( _("Credits") );
    gtk_misc_set_alignment( GTK_MISC(credits_button_label) , 0 , 0.5 );
    gtk_box_pack_start( GTK_BOX(credits_button_hbox) , credits_button_image ,
                        TRUE , TRUE , 2 );
    gtk_box_pack_start( GTK_BOX(credits_button_hbox) , credits_button_label ,
                        TRUE , TRUE , 2 );
    gtk_container_add( GTK_CONTAINER(credits_button) , credits_button_hbox );

    g_signal_connect(credits_button, "clicked",
	G_CALLBACK(on_credits_button_clicked), NULL);

    gtk_fixed_put( GTK_FIXED(about_fixedbox) , credits_button , 25 , 220 );
    gtk_widget_set_size_request( credits_button , 100 , -1 );

    brief_label = gtk_label_new(NULL);
    text = g_strdup_printf(_(audacious_brief), VERSION);

    brief_label_foreground = pango_attr_foreground_new(0, 0, 0);
    brief_label_attrs = pango_attr_list_new();
    pango_attr_list_insert(brief_label_attrs, brief_label_foreground);

    gtk_label_set_markup(GTK_LABEL(brief_label), text);
    gtk_label_set_justify(GTK_LABEL(brief_label), GTK_JUSTIFY_CENTER);
    gtk_label_set_attributes(GTK_LABEL(brief_label), brief_label_attrs);
    g_free(text);

    gtk_fixed_put(GTK_FIXED(about_fixedbox), brief_label, 20, 145);
    gtk_widget_set_size_request( brief_label , 460 , -1 );

    gtk_widget_show_all(about_window);
    gtk_window_present(GTK_WINDOW(about_window));
}

void
audgui_hide_about_window(void)
{
    g_return_if_fail(about_window);
    gtk_widget_hide(GTK_WIDGET(about_window));
}
