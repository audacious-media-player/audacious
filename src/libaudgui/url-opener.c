/*
 * url-opener.c
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

#include <gtk/gtk.h>

#include <audacious/drct.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "init.h"
#include "libaudgui.h"

static GtkWidget * window;

static void response_cb (GtkWidget * dialog, int response)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget * entry = g_object_get_data ((GObject *) dialog, "entry");
        const char * text = gtk_entry_get_text ((GtkEntry *) entry);
        bool_t open = GPOINTER_TO_INT (g_object_get_data ((GObject *) dialog, "open"));

        if (open)
            aud_drct_pl_open (text);
        else
            aud_drct_pl_add (text, -1);

        aud_history_add (text);
    }

    gtk_widget_destroy (dialog);
}

EXPORT void audgui_show_add_url_window (bool_t open)
{
    if (window)
        gtk_widget_destroy (window);

    window = gtk_dialog_new_with_buttons (open ? _("Open URL") : _("Add URL"),
     NULL, 0, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, open ? GTK_STOCK_OPEN :
     GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);
    gtk_widget_set_size_request (window, 300, -1);
    gtk_window_set_resizable ((GtkWindow *) window, FALSE);
    gtk_dialog_set_default_response ((GtkDialog *) window, GTK_RESPONSE_ACCEPT);

    GtkWidget * box = gtk_dialog_get_content_area ((GtkDialog *) window);

    GtkWidget * label = gtk_label_new (_("Enter URL:"));
    gtk_misc_set_alignment ((GtkMisc *) label, 0, 0);
    gtk_box_pack_start ((GtkBox *) box, label, FALSE, FALSE, 0);

    GtkWidget * combo = gtk_combo_box_text_new_with_entry ();
    gtk_box_pack_start ((GtkBox *) box, combo, FALSE, FALSE, 0);

    GtkWidget * entry = gtk_bin_get_child ((GtkBin *) combo);
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);

    const char * item;
    for (int i = 0; (item = aud_history_get (i)); i++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, item);

    g_object_set_data ((GObject *) window, "entry", entry);
    g_object_set_data ((GObject *) window, "open", GINT_TO_POINTER (open));

    g_signal_connect (window, "response", (GCallback) response_cb, NULL);
    g_signal_connect (window, "destroy", (GCallback) gtk_widget_destroyed, & window);

    gtk_widget_show_all (window);
}

void audgui_url_opener_cleanup (void)
{
    if (window)
        gtk_widget_destroy (window);
}
