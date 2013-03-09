/*
 * jump-to-time.c
 * Copyright 2012 John Lindgren
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

#include <stdio.h>
#include <gtk/gtk.h>

#include <audacious/drct.h>
#include <audacious/i18n.h>

#include "init.h"
#include "libaudgui.h"

static GtkWidget * window;

static void response_cb (GtkWidget * dialog, int response)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget * entry = g_object_get_data ((GObject *) dialog, "entry");
        const char * text = gtk_entry_get_text ((GtkEntry *) entry);
        unsigned minutes, seconds;

        if (sscanf (text, "%u:%u", & minutes, & seconds) == 2 && aud_drct_get_playing ())
            aud_drct_seek ((minutes * 60 + seconds) * 1000);
    }

    gtk_widget_destroy (dialog);
}

EXPORT void audgui_jump_to_time (void)
{
    if (window)
        gtk_widget_destroy (window);

    window = gtk_dialog_new_with_buttons (_("Jump to Time"), NULL, 0,
     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_JUMP_TO,
     GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_set_size_request (window, 200, -1);
    gtk_window_set_resizable ((GtkWindow *) window, FALSE);
    gtk_dialog_set_default_response ((GtkDialog *) window, GTK_RESPONSE_ACCEPT);

    GtkWidget * box = gtk_dialog_get_content_area ((GtkDialog *) window);

    GtkWidget * label = gtk_label_new (_("Enter time (minutes:seconds):"));
    gtk_misc_set_alignment ((GtkMisc *) label, 0, 0);
    gtk_box_pack_start ((GtkBox *) box, label, FALSE, FALSE, 0);

    GtkWidget * entry = gtk_entry_new ();
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);
    gtk_box_pack_start ((GtkBox *) box, entry, FALSE, FALSE, 0);

    if (aud_drct_get_playing ())
    {
        char buf[16];
        int time = aud_drct_get_time () / 1000;
        snprintf (buf, sizeof buf, "%u:%02u", time / 60, time % 60);
        gtk_entry_set_text ((GtkEntry *) entry, buf);
    }

    g_object_set_data ((GObject *) window, "entry", entry);

    g_signal_connect (window, "response", (GCallback) response_cb, NULL);
    g_signal_connect (window, "destroy", (GCallback) gtk_widget_destroyed, & window);

    gtk_widget_show_all (window);
}

void audgui_jump_to_time_cleanup (void)
{
    if (window)
        gtk_widget_destroy (window);
}
