/*
 * about.c
 * Copyright 2011-2013 John Lindgren
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

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/vfs.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static const char about_text[] = "<big><b>Audacious " VERSION "</b></big>\n" COPYRIGHT;
static const char website[] = "https://audacious-media-player.org";

static GtkWidget * create_credits_notebook (const char * credits, const char * license)
{
    const char * titles[2] = {N_("Credits"), N_("License")};
    const char * text[2] = {credits, license};

    GtkWidget * notebook = gtk_notebook_new ();

    for (int i = 0; i < 2; i ++)
    {
        GtkWidget * label = gtk_label_new (_(titles[i]));

        GtkWidget * scrolled = gtk_scrolled_window_new (nullptr, nullptr);
        gtk_widget_set_size_request (scrolled, -1, 2 * audgui_get_dpi ());
        gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled,
         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

        GtkTextBuffer * buffer = gtk_text_buffer_new (nullptr);
        gtk_text_buffer_set_text (buffer, text[i], -1);
        GtkWidget * text = gtk_text_view_new_with_buffer (buffer);
        gtk_text_view_set_editable ((GtkTextView *) text, false);
        gtk_text_view_set_cursor_visible ((GtkTextView *) text, false);
        gtk_text_view_set_left_margin ((GtkTextView *) text, 6);
        gtk_text_view_set_right_margin ((GtkTextView *) text, 6);
        gtk_container_add ((GtkContainer *) scrolled, text);

        gtk_notebook_append_page ((GtkNotebook *) notebook, scrolled, label);
    }

    return notebook;
}

static GtkWidget * create_about_window ()
{
    const char * data_dir = aud_get_path (AudPath::DataDir);

    int dpi = audgui_get_dpi ();

    GtkWidget * about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) about_window, _("About Audacious"));
    gtk_window_set_resizable ((GtkWindow *) about_window, false);
    gtk_container_set_border_width ((GtkContainer *) about_window, 3);

    audgui_destroy_on_escape (about_window);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_add ((GtkContainer *) about_window, vbox);

    AudguiPixbuf logo (gdk_pixbuf_new_from_resource_at_scale
     ("/org/audacious/about-logo.svg", 4 * dpi, 2 * dpi, true, nullptr));
    GtkWidget * image = gtk_image_new_from_pixbuf (logo.get ());
    gtk_box_pack_start ((GtkBox *) vbox, image, false, false, 0);

    GtkWidget * label = gtk_label_new (nullptr);
    gtk_label_set_markup ((GtkLabel *) label, about_text);
    gtk_label_set_justify ((GtkLabel *) label, GTK_JUSTIFY_CENTER);
    gtk_box_pack_start ((GtkBox *) vbox, label, false, false, 0);

    GtkWidget * align = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_box_pack_start ((GtkBox *) vbox, align, false, false, 0);

    GtkWidget * button = gtk_link_button_new (website);
    gtk_container_add ((GtkContainer *) align, button);

    auto credits = VFSFile::read_file (filename_build ({data_dir, "AUTHORS"}), VFS_APPEND_NULL);
    auto license = VFSFile::read_file (filename_build ({data_dir, "COPYING"}), VFS_APPEND_NULL);

    GtkWidget * notebook = create_credits_notebook (credits.begin (), license.begin ());
    gtk_widget_set_size_request (notebook, 6 * dpi, 2 * dpi);
    gtk_box_pack_start ((GtkBox *) vbox, notebook, true, true, 0);

    return about_window;
}

EXPORT void audgui_show_about_window ()
{
    if (! audgui_reshow_unique_window (AUDGUI_ABOUT_WINDOW))
        audgui_show_unique_window (AUDGUI_ABOUT_WINDOW, create_about_window ());
}

EXPORT void audgui_hide_about_window ()
{
    audgui_hide_unique_window (AUDGUI_ABOUT_WINDOW);
}
