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

#include <string.h>
#include <gtk/gtk.h>

#define AUD_GLIB_INTEGRATION
#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

struct ImportExportJob {
    bool save;
    int list_id;
    CharPtr filename;
    GtkWidget * selector = nullptr;
    GtkWidget * confirm = nullptr;

    ImportExportJob (bool save, int list_id) :
        save (save), list_id (list_id) {}
};

/* "destroy" callback; do not call directly */
static void cleanup_job (void * data)
{
    ImportExportJob * job = (ImportExportJob *) data;

    CharPtr folder (gtk_file_chooser_get_current_folder_uri ((GtkFileChooser *) job->selector));
    if (folder)
        aud_set_str ("audgui", "playlist_path", folder);

    if (job->confirm)
        gtk_widget_destroy (job->confirm);

    delete job;
}

static void finish_job (void * data)
{
    ImportExportJob * job = (ImportExportJob *) data;
    int list = aud_playlist_by_unique_id (job->list_id);

    Playlist::GetMode mode = Playlist::Wait;
    if (aud_get_bool (nullptr, "metadata_on_play"))
        mode = Playlist::NoWait;

    if (list >= 0)
    {
        aud_playlist_set_filename (list, job->filename);

        if (job->save)
            aud_playlist_save (list, job->filename, mode);
        else
        {
            aud_playlist_entry_delete (list, 0, aud_playlist_entry_count (list));
            aud_playlist_entry_insert (list, 0, job->filename, Tuple (), false);
        }
    }

    gtk_widget_destroy (job->selector);
}

static void confirm_overwrite (ImportExportJob * job)
{
    if (job->confirm)
        gtk_widget_destroy (job->confirm);

    GtkWidget * button1 = audgui_button_new (_("_Overwrite"), "document-save", finish_job, job);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", nullptr, nullptr);

    job->confirm = audgui_dialog_new (GTK_MESSAGE_QUESTION,
     _("Confirm Overwrite"), str_printf (_("Overwrite %s?"), (const char *) job->filename),
     button1, button2);

    g_signal_connect (job->confirm, "destroy", (GCallback) gtk_widget_destroyed, & job->confirm);

    gtk_widget_show_all (job->confirm);
}

static void check_overwrite (void * data)
{
    ImportExportJob * job = (ImportExportJob *) data;

    job->filename = CharPtr (gtk_file_chooser_get_uri ((GtkFileChooser *) job->selector));
    if (! job->filename)
        return;

    if (job->save && ! strchr (job->filename, '.'))
    {
        const char * default_ext = nullptr;
        auto filter = gtk_file_chooser_get_filter ((GtkFileChooser *) job->selector);

        if (filter)
            default_ext = (const char *) g_object_get_data ((GObject *) filter, "default-ext");

        if (! default_ext)
        {
            aud_ui_show_error (_("Please type a filename extension or select a "
             "format from the drop-down list."));
            return;
        }

        job->filename.capture (g_strconcat (job->filename, ".", default_ext, nullptr));
    }

    if (job->save && VFSFile::test_file (job->filename, VFS_EXISTS))
        confirm_overwrite (job);
    else
        finish_job (data);
}

static void set_format_filters (GtkWidget * selector)
{
    GtkFileFilter * filter;

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Select Format by Extension"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter ((GtkFileChooser *) selector, filter);

    for (auto & format : aud_playlist_save_formats ())
    {
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, format.name);

        for (auto & ext : format.exts)
            gtk_file_filter_add_pattern (filter, str_concat ({"*.", ext}));

        if (format.exts.len ())
            g_object_set_data_full ((GObject *) filter, "default-ext",
             g_strdup (format.exts[0]), g_free);

        gtk_file_chooser_add_filter ((GtkFileChooser *) selector, filter);
    }
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

    job->selector = gtk_file_chooser_dialog_new (title, nullptr, action, nullptr, nullptr);
    gtk_file_chooser_set_local_only ((GtkFileChooser *) job->selector, false);

    if (filename)
        gtk_file_chooser_set_uri ((GtkFileChooser *) job->selector, filename);
    else if (folder)
        gtk_file_chooser_set_current_folder_uri ((GtkFileChooser *) job->selector, folder);

    GtkWidget * button1 = audgui_button_new (verb, icon, check_overwrite, job);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop",
     (AudguiCallback) gtk_widget_destroy, job->selector);

    gtk_dialog_add_action_widget ((GtkDialog *) job->selector, button2, GTK_RESPONSE_NONE);
    gtk_dialog_add_action_widget ((GtkDialog *) job->selector, button1, GTK_RESPONSE_NONE);

    gtk_widget_set_can_default (button1, true);
    gtk_widget_grab_default (button1);

    if (job->save)
        set_format_filters (job->selector);

    g_signal_connect_swapped (job->selector, "destroy", (GCallback) cleanup_job, job);

    gtk_widget_show_all (job->selector);
}

static GtkWidget * start_job (bool save)
{
    int list = aud_playlist_get_active ();

    String filename = aud_playlist_get_filename (list);
    String folder = aud_get_str ("audgui", "playlist_path");

    ImportExportJob * job = new ImportExportJob (save, aud_playlist_get_unique_id (list));
    create_selector (job, filename, folder[0] ? (const char *) folder : nullptr);

    return job->selector;
}

EXPORT void audgui_import_playlist ()
{
    audgui_show_unique_window (AUDGUI_PLAYLIST_IMPORT_WINDOW, start_job (false));
}

EXPORT void audgui_export_playlist ()
{
    audgui_show_unique_window (AUDGUI_PLAYLIST_EXPORT_WINDOW, start_job (true));
}
