/*
 * confirm.c
 * Copyright 2010-2013 John Lindgren and Thomas Lange
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

static void confirm_delete_cb (GtkWidget * dialog, int response, void * data)
{
    int list = aud_playlist_by_unique_id (GPOINTER_TO_INT (data));

    if (list >= 0 && response == GTK_RESPONSE_YES)
        aud_playlist_delete (list);

    gtk_widget_destroy (dialog);
}

EXPORT void audgui_confirm_playlist_delete (int playlist)
{
    if (aud_get_bool ("audgui", "no_confirm_playlist_delete"))
    {
        aud_playlist_delete (playlist);
        return;
    }

    char * title = aud_playlist_get_title (playlist);

    GtkWidget * dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_YES_NO, _("Do you want to close “%s”?\nOnce closed, the "
      "playlist cannot be recovered."), title);

    gtk_window_set_title ((GtkWindow *) dialog, _("Close Playlist"));
    gtk_dialog_set_default_response ((GtkDialog *) dialog, GTK_RESPONSE_YES);

    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    GtkWidget * button = gtk_check_button_new_with_mnemonic (_("_Don’t ask again"));
    gtk_box_pack_start ((GtkBox *) box, button, FALSE, FALSE, 0);

    int id = aud_playlist_get_unique_id (playlist);
    g_signal_connect (button, "toggled", (GCallback) no_confirm_cb, NULL);
    g_signal_connect (dialog, "response", (GCallback) confirm_delete_cb, GINT_TO_POINTER (id));

    gtk_widget_show_all (dialog);

    str_unref (title);
}

static void rename_cb (GtkDialog * dialog, int response, void * data)
{
    int list = aud_playlist_by_unique_id (GPOINTER_TO_INT (data));

    if (list >= 0 && response == GTK_RESPONSE_OK)
    {
        GtkWidget * entry = g_object_get_data ((GObject *) dialog, "entry");
        const char * name = gtk_entry_get_text ((GtkEntry *) entry);
        aud_playlist_set_title (list, name);
    }

    gtk_widget_destroy ((GtkWidget *) dialog);
}

EXPORT void audgui_show_playlist_rename (int playlist)
{
    char * title = aud_playlist_get_title (playlist);

    GtkWidget * dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_OK_CANCEL, _("What would you like to call this playlist?"));

    gtk_window_set_title ((GtkWindow *) dialog, _("Rename Playlist"));
    gtk_dialog_set_default_response ((GtkDialog *) dialog, GTK_RESPONSE_OK);

    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    GtkWidget * entry = gtk_entry_new ();

    gtk_entry_set_text ((GtkEntry *) entry, title);
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);
    gtk_box_pack_start ((GtkBox *) box, entry, FALSE, FALSE, 0);
    g_object_set_data ((GObject *) dialog, "entry", entry);

    int id = aud_playlist_get_unique_id (playlist);
    g_signal_connect (dialog, "response", (GCallback) rename_cb, GINT_TO_POINTER (id));

    gtk_widget_show_all (dialog);

    str_unref (title);
}
