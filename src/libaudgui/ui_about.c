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

#include <limits.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <gdk/gdk.h>

#if GTK_CHECK_VERSION (3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "audacious/compatibility.h"

#include "ui_credits.h"

static GtkWidget *about_window = NULL;

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
    GtkWidget *close_button;
    GtkWidget *credits_button;
    GtkWidget *brief_label;
    gchar *text;
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

    gtk_window_set_title(GTK_WINDOW(about_window), _("About Audacious"));
    gtk_window_set_resizable(GTK_WINDOW(about_window), FALSE);

    g_signal_connect(about_window, "key-press-event",
	G_CALLBACK(on_about_window_key_press), &about_window);

    GtkWidget * vbox = gtk_vbox_new (0, 0);
    gtk_container_add ((GtkContainer *) about_window, vbox);

    gchar name[PATH_MAX];
    snprintf (name, sizeof name, "%s/images/about-logo.png", aud_get_path
     (AUD_PATH_DATA_DIR));
    GtkWidget * image = gtk_image_new_from_file (name);
    gtk_box_pack_start ((GtkBox *) vbox, image, FALSE, FALSE, 0);

    GtkWidget * align = gtk_alignment_new (0, 0, 1, 1);
    gtk_alignment_set_padding ((GtkAlignment *) align, 6, 6, 6, 6);
    gtk_container_add ((GtkContainer *) vbox, align);
    
    GtkWidget * vbox2 = gtk_vbox_new (0, 6);
    gtk_container_add ((GtkContainer *) align, vbox2);

    brief_label = gtk_label_new(NULL);
    text = g_strdup_printf(_(audacious_brief), VERSION);

    gtk_label_set_markup(GTK_LABEL(brief_label), text);
    gtk_label_set_justify(GTK_LABEL(brief_label), GTK_JUSTIFY_CENTER);
    g_free(text);
    
    gtk_box_pack_start ((GtkBox *) vbox2, brief_label, FALSE, FALSE, 0);

    GtkWidget * hbox = gtk_hbox_new (0, 6);
    gtk_box_pack_start ((GtkBox *) vbox2, hbox, FALSE, FALSE, 0);

    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_end ((GtkBox *) hbox, close_button, FALSE, FALSE, 0);

    g_signal_connect(close_button, "clicked",
	G_CALLBACK(on_close_button_clicked), NULL);

    credits_button = gtk_button_new_with_mnemonic (_("Credits"));
    gtk_button_set_image ((GtkButton *) credits_button, gtk_image_new_from_stock
     (GTK_STOCK_ABOUT, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start ((GtkBox *) hbox, credits_button, FALSE, FALSE, 0);

    g_signal_connect(credits_button, "clicked",
	G_CALLBACK(on_credits_button_clicked), NULL);

    gtk_widget_show_all(about_window);
}

void
audgui_hide_about_window(void)
{
    g_return_if_fail(about_window);
    gtk_widget_hide(GTK_WIDGET(about_window));
}
