/*
 * confirm.c
 * Copyright 2010-2011 John Lindgren
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
#include <audacious/playlist.h>

#include "libaudgui-gtk.h"

static void no_confirm_cb (GtkToggleButton * toggle)
{
    aud_set_bool ("audgui", "no_confirm_playlist_delete", gtk_toggle_button_get_active (toggle));
}

static void confirm_delete_cb (void * data)
{
    int list = aud_playlist_by_unique_id (GPOINTER_TO_INT (data));
    if (list < 0)
        return;

    aud_playlist_delete (list);
    if (list > 0)
        aud_playlist_set_active (list - 1);
}

static void confirm_playlist_delete_response (GtkWidget * dialog, gint response, gpointer data)
{
    if (response == GTK_RESPONSE_YES)
        confirm_delete_cb (data);

    gtk_widget_destroy (dialog);
}

EXPORT void audgui_confirm_playlist_delete (int playlist)
{
    GtkWidget * dialog, * vbox, * button;
    char * message;

    if (aud_get_bool ("audgui", "no_confirm_playlist_delete"))
    {
        aud_playlist_delete (playlist);
        if (playlist > 0)
            aud_playlist_set_active (playlist - 1);
        return;
    }

    char * title = aud_playlist_get_title (playlist);
    message = g_strdup_printf (_("Are you sure you want to close %s?  If you "
     "do, any changes made since the playlist was exported will be lost."), title);
    str_unref (title);

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", message);
    g_free (message);
    gtk_window_set_title ((GtkWindow *) dialog, _("Close Playlist"));
    gtk_dialog_set_default_response ((GtkDialog *) dialog, GTK_RESPONSE_YES);

    vbox = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    button = gtk_check_button_new_with_mnemonic (_("_Don't show this message again"));

    gtk_container_add ((GtkContainer *) vbox, button);

    g_signal_connect (button, "toggled", (GCallback) no_confirm_cb, NULL);
    g_signal_connect (dialog, "response", (GCallback) confirm_playlist_delete_response,
     GINT_TO_POINTER (aud_playlist_get_unique_id (playlist)));

    gtk_widget_show_all (dialog);
}

static void rename_cb (GtkDialog * dialog, int resp, void * list)
{
    if (resp == GTK_RESPONSE_ACCEPT && GPOINTER_TO_INT (list) <
     aud_playlist_count ())
        aud_playlist_set_title (GPOINTER_TO_INT (list), gtk_entry_get_text
         ((GtkEntry *) g_object_get_data ((GObject *) dialog, "entry")));

    gtk_widget_destroy ((GtkWidget *) dialog);
}

EXPORT void audgui_show_playlist_rename (int playlist)
{
    GtkWidget * dialog = gtk_dialog_new_with_buttons (_("Rename Playlist"),
     NULL, 0, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,
     GTK_RESPONSE_REJECT, NULL);
    gtk_dialog_set_default_response ((GtkDialog *) dialog, GTK_RESPONSE_ACCEPT);

    GtkWidget * entry = gtk_entry_new ();
    char * title = aud_playlist_get_title (playlist);
    gtk_entry_set_text ((GtkEntry *) entry, title);
    str_unref (title);
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);
    gtk_box_pack_start ((GtkBox *) gtk_dialog_get_content_area ((GtkDialog *)
     dialog), entry, FALSE, FALSE, 0);
    g_object_set_data ((GObject *) dialog, "entry", entry);

    g_signal_connect (dialog, "response", (GCallback) rename_cb, GINT_TO_POINTER
     (playlist));
    gtk_widget_show_all (dialog);
}
