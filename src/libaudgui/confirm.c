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
#include <libaudcore/audstrings.h>

#include "libaudgui-gtk.h"

static void no_confirm_cb (GtkToggleButton * toggle)
{
    aud_set_bool ("audgui", "no_confirm_playlist_delete", gtk_toggle_button_get_active (toggle));
}

static void confirm_delete_cb (void * data)
{
    int list = aud_playlist_by_unique_id (GPOINTER_TO_INT (data));

    if (list >= 0)
        aud_playlist_delete (list);
}

EXPORT void audgui_confirm_playlist_delete (int playlist)
{
    if (aud_get_bool ("audgui", "no_confirm_playlist_delete"))
    {
        aud_playlist_delete (playlist);
        return;
    }

    char * title = aud_playlist_get_title (playlist);
    SPRINTF (message, _("Do you want to permanently remove “%s”?"), title);
    str_unref (title);

    int id = aud_playlist_get_unique_id (playlist);
    GtkWidget * button1 = audgui_button_new (_("_Remove"), "edit-delete",
     confirm_delete_cb, GINT_TO_POINTER (id));
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", NULL, NULL);

    GtkWidget * dialog = audgui_dialog_new (GTK_MESSAGE_QUESTION,
     _("Remove Playlist"), message, button1, button2);

    GtkWidget * check = gtk_check_button_new_with_mnemonic (_("_Don’t ask again"));
    g_signal_connect (check, "toggled", (GCallback) no_confirm_cb, NULL);
    audgui_dialog_add_widget (dialog, check);

    gtk_widget_show_all (dialog);
}

static void rename_cb (void * entry)
{
    void * data = g_object_get_data ((GObject *) entry, "playlist-id");
    int list = aud_playlist_by_unique_id (GPOINTER_TO_INT (data));

    if (list >= 0)
        aud_playlist_set_title (list, gtk_entry_get_text ((GtkEntry *) entry));
}

EXPORT void audgui_show_playlist_rename (int playlist)
{
    char * title = aud_playlist_get_title (playlist);

    GtkWidget * entry = gtk_entry_new ();
    gtk_entry_set_text ((GtkEntry *) entry, title);
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);

    int id = aud_playlist_get_unique_id (playlist);
    g_object_set_data ((GObject *) entry, "playlist-id", GINT_TO_POINTER (id));

    GtkWidget * button1 = audgui_button_new (_("_Rename"), "insert-text", rename_cb, entry);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", NULL, NULL);

    GtkWidget * dialog = audgui_dialog_new (GTK_MESSAGE_QUESTION,
     _("Rename Playlist"), _("What would you like to call this playlist?"),
     button1, button2);

    audgui_dialog_add_widget (dialog, entry);

    gtk_widget_show_all (dialog);

    str_unref (title);
}
