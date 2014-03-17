/*
 * playlists.c
 * Copyright 2013 John Lindgren
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
#include <libaudcore/vfs.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

typedef struct
{
    bool_t save;
    int list_id;
    char * filename;
    GtkWidget * selector, * confirm;
}
ImportExportJob;

/* "destroy" callback; do not call directly */
static void cleanup_job (void * data)
{
    ImportExportJob * job = data;

    char * folder = gtk_file_chooser_get_current_folder_uri ((GtkFileChooser *) job->selector);

    if (folder)
        aud_set_str ("audgui", "playlist_path", folder);

    g_free (folder);

    if (job->confirm)
        gtk_widget_destroy (job->confirm);

    g_free (job->filename);
    g_slice_free (ImportExportJob, job);
}

static void finish_job (void * data)
{
    ImportExportJob * job = data;
    int list = aud_playlist_by_unique_id (job->list_id);

    if (list >= 0)
    {
        aud_playlist_set_filename (list, job->filename);

        if (job->save)
            aud_playlist_save (list, job->filename);
        else
        {
            aud_playlist_entry_delete (list, 0, aud_playlist_entry_count (list));
            aud_playlist_entry_insert (list, 0, job->filename, NULL, FALSE);
        }
    }

    gtk_widget_destroy (job->selector);
}

static void confirm_overwrite (ImportExportJob * job)
{
    if (job->confirm)
        gtk_widget_destroy (job->confirm);

    SPRINTF (message, _("Overwrite %s?"), job->filename);

    GtkWidget * button1 = audgui_button_new (_("_Overwrite"), "document-save", finish_job, job);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", NULL, NULL);

    job->confirm = audgui_dialog_new (GTK_MESSAGE_QUESTION,
     _("Confirm Overwrite"), message, button1, button2);

    g_signal_connect (job->confirm, "destroy", (GCallback) gtk_widget_destroyed, & job->confirm);

    gtk_widget_show_all (job->confirm);
}

static void check_overwrite (void * data)
{
    ImportExportJob * job = data;

    job->filename = gtk_file_chooser_get_uri ((GtkFileChooser *) job->selector);

    if (! job->filename)
        return;

    if (job->save && vfs_file_test (job->filename, G_FILE_TEST_EXISTS))
        confirm_overwrite (job);
    else
        finish_job (data);
}

static void create_selector (ImportExportJob * job, const char * filename, const char * folder)
{
    const char * title, * verb, * icon;
    GtkFileChooserAction action;

    if (job->save)
    {
        title = _("Export Playlist");
        verb = _("_Export");
        icon = "document-save";
        action = GTK_FILE_CHOOSER_ACTION_SAVE;
    }
    else
    {
        title = _("Import Playlist");
        verb = _("_Import");
        icon = "document-open";
        action = GTK_FILE_CHOOSER_ACTION_OPEN;
    }

    job->selector = gtk_file_chooser_dialog_new (title, NULL, action, NULL, NULL);

    if (filename)
        gtk_file_chooser_set_uri ((GtkFileChooser *) job->selector, filename);
    else if (folder)
        gtk_file_chooser_set_current_folder_uri ((GtkFileChooser *) job->selector, folder);

    GtkWidget * button1 = audgui_button_new (verb, icon, check_overwrite, job);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop",
     (AudguiCallback) gtk_widget_destroy, job->selector);

    gtk_dialog_add_action_widget ((GtkDialog *) job->selector, button2, GTK_RESPONSE_NONE);
    gtk_dialog_add_action_widget ((GtkDialog *) job->selector, button1, GTK_RESPONSE_NONE);

    gtk_widget_set_can_default (button1, TRUE);
    gtk_widget_grab_default (button1);

    g_signal_connect_swapped (job->selector, "destroy", (GCallback) cleanup_job, job);

    gtk_widget_show_all (job->selector);
}

static GtkWidget * start_job (bool_t save)
{
    int list = aud_playlist_get_active ();

    char * filename = aud_playlist_get_filename (list);
    char * folder = aud_get_str ("audgui", "playlist_path");

    ImportExportJob * job = g_slice_new0 (ImportExportJob);

    job->save = save;
    job->list_id = aud_playlist_get_unique_id (list);

    create_selector (job, filename, folder[0] ? folder : NULL);

    str_unref (filename);
    str_unref (folder);

    return job->selector;
}

EXPORT void audgui_import_playlist (void)
{
    audgui_show_unique_window (AUDGUI_PLAYLIST_IMPORT_WINDOW, start_job (FALSE));
}

EXPORT void audgui_export_playlist (void)
{
    audgui_show_unique_window (AUDGUI_PLAYLIST_EXPORT_WINDOW, start_job (TRUE));
}
