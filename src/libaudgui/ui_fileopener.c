/*
 * ui_fileopener.c
 * Copyright 2007-2013 Michael Färber and John Lindgren
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
#include <audacious/drct.h>
#include <audacious/misc.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static Index * get_files (GtkWidget * chooser)
{
    Index * index = index_new ();
    GSList * list = gtk_file_chooser_get_uris ((GtkFileChooser *) chooser);

    for (GSList * node = list; node; node = node->next)
        index_insert (index, -1, str_get (node->data));

    g_slist_free_full (list, g_free);
    return index;
}

static void open_cb (void * data)
{
    GtkWidget * chooser = data;
    Index * files = get_files (chooser);
    bool_t open = GPOINTER_TO_INT (g_object_get_data ((GObject *) chooser, "do-open"));

    if (open)
        aud_drct_pl_open_list (files);
    else
        aud_drct_pl_add_list (files, -1);

    GtkWidget * toggle = g_object_get_data ((GObject *) chooser, "toggle-button");
    if (gtk_toggle_button_get_active ((GtkToggleButton *) toggle))
        audgui_hide_filebrowser ();
}

static void destroy_cb (GtkWidget * chooser)
{
    char * path = gtk_file_chooser_get_current_folder ((GtkFileChooser *) chooser);
    if (path)
    {
        aud_set_str ("audgui", "filesel_path", path);
        g_free (path);
    }
}

static void toggled_cb (GtkToggleButton * toggle, void * option)
{
    aud_set_bool ("audgui", (const char *) option, gtk_toggle_button_get_active (toggle));
}

static GtkWidget * create_filebrowser (bool_t open)
{
    const char * window_title, * verb, * icon, * toggle_text, * option;

    if (open)
    {
        window_title = _("Open Files");
        verb = _("_Open");
        icon = "document-open";
        toggle_text = _("Close _dialog on open");
        option = "close_dialog_open";
    }
    else
    {
        window_title = _("Add Files");
        verb = _("_Add");
        icon = "list-add";
        toggle_text = _("Close _dialog on add");
        option = "close_dialog_add";
    }

    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 450);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget * chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

    char * path = aud_get_str ("audgui", "filesel_path");
    if (path[0])
        gtk_file_chooser_set_current_folder ((GtkFileChooser *) chooser, path);
    str_unref (path);

    gtk_box_pack_start(GTK_BOX(vbox), chooser, TRUE, TRUE, 3);

    GtkWidget * hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    GtkWidget * toggle = gtk_check_button_new_with_mnemonic (toggle_text);
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle, aud_get_bool ("audgui", option));
    g_signal_connect (toggle, "toggled", (GCallback) toggled_cb, (void *) option);
    gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 3);

    GtkWidget * bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 6);
    gtk_box_pack_end(GTK_BOX(hbox), bbox, TRUE, TRUE, 3);

    GtkWidget * action_button = audgui_button_new (verb, icon, open_cb, chooser);
    GtkWidget * close_button = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) audgui_hide_filebrowser, NULL);

    gtk_container_add(GTK_CONTAINER(bbox), close_button);
    gtk_container_add(GTK_CONTAINER(bbox), action_button);

    gtk_widget_set_can_default (action_button, TRUE);
    gtk_widget_grab_default (action_button);

    g_object_set_data ((GObject *) chooser, "toggle-button", toggle);
    g_object_set_data ((GObject *) chooser, "do-open", GINT_TO_POINTER (open));

    g_signal_connect (chooser, "file-activated", (GCallback) open_cb, NULL);
    g_signal_connect (chooser, "destroy", (GCallback) destroy_cb, NULL);

    audgui_destroy_on_escape (window);

    return window;
}

EXPORT void audgui_run_filebrowser (bool_t open)
{
    audgui_show_unique_window (AUDGUI_FILEBROWSER_WINDOW, create_filebrowser (open));
}

EXPORT void audgui_hide_filebrowser (void)
{
    audgui_hide_unique_window (AUDGUI_FILEBROWSER_WINDOW);
}
