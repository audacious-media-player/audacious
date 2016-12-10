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

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void show_question_dialog (const char * title, const char * text,
 GtkWidget * widget, GtkWidget * action)
{
    auto cancel = audgui_button_new (_("_Cancel"), "process-stop", nullptr, nullptr);
    auto dialog = audgui_dialog_new (GTK_MESSAGE_QUESTION, title, text, action, cancel);

    audgui_dialog_add_widget (dialog, widget);
    gtk_widget_show_all (dialog);
}

static void no_confirm_cb (GtkToggleButton * toggle, void * config)
{
    aud_set_bool ("audgui", (const char *) config, gtk_toggle_button_get_active (toggle));
}

static void show_confirm_dialog (const char * title, const char * text,
 GtkWidget * action, const char * config)
{
    auto check = gtk_check_button_new_with_mnemonic (_("_Don’t ask again"));
    g_signal_connect (check, "toggled", (GCallback) no_confirm_cb, (void *) config);

    show_question_dialog (title, text, check, action);
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

    StringBuf text = str_printf (_("Do you want to permanently remove “%s”?"),
     (const char *) aud_playlist_get_title (playlist));

    int id = aud_playlist_get_unique_id (playlist);
    auto button = audgui_button_new (_("_Remove"), "edit-delete",
     confirm_delete_cb, GINT_TO_POINTER (id));

    show_confirm_dialog (_("Remove Playlist"), text, button, "no_confirm_playlist_delete");
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
    auto entry = gtk_entry_new ();
    gtk_entry_set_text ((GtkEntry *) entry, aud_playlist_get_title (playlist));
    gtk_entry_set_activates_default ((GtkEntry *) entry, true);

    int id = aud_playlist_get_unique_id (playlist);
    g_object_set_data ((GObject *) entry, "playlist-id", GINT_TO_POINTER (id));

    auto text = _("What would you like to call this playlist?");
    auto button = audgui_button_new (_("_Rename"), "insert-text", rename_cb, entry);
    show_question_dialog (_("Rename Playlist"), text, entry, button);
}

static void record_toggled (void *, void *)
{
    aud_set_bool ("audgui", "record", aud_drct_get_record_enabled ());
    hook_call ("audgui set record", nullptr);
}

void record_init ()
{
    aud_set_bool ("audgui", "record", aud_drct_get_record_enabled ());
    hook_associate ("enable record", record_toggled, nullptr);
}

void record_cleanup ()
{
    hook_dissociate ("enable record", record_toggled);
}

static void enable_record (void *)
{
    if (! aud_drct_enable_record (true))
        aud_ui_show_error (_("Failed to start recording.  Check that the "
         "FileWriter plugin is installed and is NOT selected as the "
         "primary output plugin."));
}

EXPORT void audgui_toggle_record ()
{
    if (aud_drct_get_record_enabled ())
        aud_drct_enable_record (false);
    else if (aud_get_bool ("audgui", "no_confirm_record"))
        enable_record (nullptr);
    else
    {
        auto text = _("Do you want to start recording?\n\n"
         "Recording saves (transcodes) a copy of the currently playing song or "
         "stream to your computer.  To change settings such as the output "
         "folder or encoding quality, go to Audio Settings in the Output menu.");

        auto button = audgui_button_new (_("_Record"), "media-record", enable_record, nullptr);
        show_confirm_dialog (_("Record"), text, button, "no_confirm_record");
    }

    /* reset menu item state if necessary */
    if (aud_drct_get_record_enabled () != aud_get_bool ("audgui", "record"))
        record_toggled (nullptr, nullptr);
}
