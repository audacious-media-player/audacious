/*
 * about.c
 * Copyright 2011-2012 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <gtk/gtk.h>

#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "libaudgui-gtk.h"

static const char about_text[] =
 "<big><b>Audacious " VERSION "</b></big>\n"
 "Copyright © 2001-2014 Audacious developers and others";

static const char website[] = "http://audacious-media-player.org";

static GtkWidget * about_window;

static GtkWidget * create_credits_notebook (const char * credits, const char * license)
{
    const char * titles[2] = {_("Credits"), _("License")};
    const char * text[2] = {credits, license};

    GtkWidget * notebook = gtk_notebook_new ();

    for (int i = 0; i < 2; i ++)
    {
        GtkWidget * label = gtk_label_new (titles[i]);

        GtkWidget * scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_set_size_request (scrolled, -1, 200);

        GtkTextBuffer * buffer = gtk_text_buffer_new (NULL);
        gtk_text_buffer_set_text (buffer, text[i], -1);
        GtkWidget * text = gtk_text_view_new_with_buffer (buffer);
        gtk_text_view_set_editable ((GtkTextView *) text, FALSE);
        gtk_text_view_set_cursor_visible ((GtkTextView *) text, FALSE);
        gtk_text_view_set_left_margin ((GtkTextView *) text, 6);
        gtk_text_view_set_right_margin ((GtkTextView *) text, 6);
        gtk_container_add ((GtkContainer *) scrolled, text);

        gtk_notebook_append_page ((GtkNotebook *) notebook, scrolled, label);
    }

    return notebook;
}

EXPORT void audgui_show_about_window (void)
{
    if (about_window)
    {
        gtk_window_present ((GtkWindow *) about_window);
        return;
    }

    about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) about_window, _("About Audacious"));
    gtk_window_set_resizable ((GtkWindow *) about_window, FALSE);
    gtk_container_set_border_width ((GtkContainer *) about_window, 3);

    audgui_destroy_on_escape (about_window);
    g_signal_connect (about_window, "destroy", (GCallback) gtk_widget_destroyed,
     & about_window);

    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add ((GtkContainer *) about_window, vbox);

    SPRINTF (logo_path, "%s/images/about-logo.png", aud_get_path (AUD_PATH_DATA_DIR));
    GtkWidget * image = gtk_image_new_from_file (logo_path);
    gtk_box_pack_start ((GtkBox *) vbox, image, FALSE, FALSE, 0);

    GtkWidget * label = gtk_label_new (NULL);
    gtk_label_set_markup ((GtkLabel *) label, about_text);
    gtk_label_set_justify ((GtkLabel *) label, GTK_JUSTIFY_CENTER);
    gtk_box_pack_start ((GtkBox *) vbox, label, FALSE, FALSE, 0);

    GtkWidget * button = gtk_link_button_new (website);
    gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
    gtk_box_pack_start ((GtkBox *) vbox, button, FALSE, FALSE, 0);

    char * credits, * license;

    SPRINTF (credits_path, "%s/AUTHORS", aud_get_path (AUD_PATH_DATA_DIR));
    if (! g_file_get_contents (credits_path, & credits, NULL, NULL))
        credits = g_strdup_printf ("Unable to load %s; check your installation.", credits_path);

    SPRINTF (license_path, "%s/COPYING", aud_get_path (AUD_PATH_DATA_DIR));
    if (! g_file_get_contents (license_path, & license, NULL, NULL))
        license = g_strdup_printf ("Unable to load %s; check your installation.", license_path);

    g_strchomp (credits);
    g_strchomp (license);

    GtkWidget * notebook = create_credits_notebook (credits, license);
    gtk_widget_set_size_request (notebook, 600, 250);
    gtk_box_pack_start ((GtkBox *) vbox, notebook, TRUE, TRUE, 0);

    g_free (credits);
    g_free (license);

    gtk_widget_show_all (about_window);
}
