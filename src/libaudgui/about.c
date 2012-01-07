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

#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "config.h"
#include "libaudgui-gtk.h"

static GtkWidget * about_window;

static GtkTextBuffer * create_text_buffer (const char * const * items)
{
    GString * string = g_string_new ("");

    for (; items[0] || items[1]; items ++)
    {
        if (items[0])
            g_string_append (string, _(items[0]));

        g_string_append_c (string, '\n');
    }

    GtkTextBuffer * buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, string->str, string->len - 1);
    g_string_free (string, TRUE);
    return buffer;
}

static GtkWidget * create_credits_notebook (const char * const * credits,
 const char * const * translators)
{
    const char * titles[2] = {_("Credits"), _("Translators")};
    const char * const * lists[2] = {credits, translators};

    GtkWidget * notebook = gtk_notebook_new ();

    for (int i = 0; i < 2; i ++)
    {
        GtkWidget * label = gtk_label_new (titles[i]);

        GtkWidget * scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled,
         GTK_SHADOW_IN);
        gtk_widget_set_size_request (scrolled, -1, 200);

        GtkWidget * text = gtk_text_view_new_with_buffer (create_text_buffer (lists[i]));
        gtk_text_view_set_editable ((GtkTextView *) text, FALSE);
        gtk_text_view_set_cursor_visible ((GtkTextView *) text, FALSE);
        gtk_text_view_set_left_margin ((GtkTextView *) text, 6);
        gtk_text_view_set_right_margin ((GtkTextView *) text, 6);
        gtk_container_add ((GtkContainer *) scrolled, text);

        gtk_notebook_append_page ((GtkNotebook *) notebook, scrolled, label);
    }

    return notebook;
}

void audgui_show_about_window (void)
{
    if (about_window)
    {
        gtk_window_present ((GtkWindow *) about_window);
        return;
    }

    const char * brief, * const * credits, * const * translators;
    aud_get_audacious_credits (& brief, & credits, & translators);

    about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) about_window, _("About Audacious"));
    gtk_window_set_resizable ((GtkWindow *) about_window, FALSE);
    gtk_container_set_border_width ((GtkContainer *) about_window, 3);

    audgui_destroy_on_escape (about_window);
    g_signal_connect (about_window, "destroy", (GCallback) gtk_widget_destroyed,
     & about_window);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) about_window, vbox);

    char * name = g_strdup_printf ("%s/images/about-logo.png", aud_get_path
     (AUD_PATH_DATA_DIR));
    GtkWidget * image = gtk_image_new_from_file (name);
    gtk_box_pack_start ((GtkBox *) vbox, image, FALSE, FALSE, 0);
    g_free (name);

    char * markup = g_strdup_printf (brief, VERSION);
    GtkWidget * label = gtk_label_new (NULL);
    gtk_label_set_markup ((GtkLabel *) label, markup);
    gtk_label_set_justify ((GtkLabel *) label, GTK_JUSTIFY_CENTER);
    gtk_box_pack_start ((GtkBox *) vbox, label, FALSE, FALSE, 0);
    g_free (markup);

    GtkWidget * exp = gtk_expander_new (_("Credits"));
    GtkWidget * notebook = create_credits_notebook (credits, translators);
    gtk_container_add ((GtkContainer *) exp, notebook);
    gtk_box_pack_start ((GtkBox *) vbox, exp, TRUE, TRUE, 0);

    gtk_widget_show_all (about_window);
}
