/*
 * file-opener.c
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

#define AUD_GLIB_INTEGRATION
#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static Index<PlaylistAddItem> get_files (GtkWidget * chooser)
{
    Index<PlaylistAddItem> index;
    GSList * list = gtk_file_chooser_get_uris ((GtkFileChooser *) chooser);

    for (GSList * node = list; node; node = node->next)
        index.append (String ((const char *) node->data));

    g_slist_free_full (list, g_free);
    return index;
}

static void open_cb (void * data)
{
    GtkWidget * chooser = (GtkWidget *) data;
    Index<PlaylistAddItem> files = get_files (chooser);
    gboolean open = GPOINTER_TO_INT (g_object_get_data ((GObject *) chooser, "do-open"));

    if (open)
        aud_drct_pl_open_list (std::move (files));
    else
        aud_drct_pl_add_list (std::move (files), -1);

    GtkWidget * toggle = (GtkWidget *) g_object_get_data ((GObject *) chooser, "toggle-button");
    if (gtk_toggle_button_get_active ((GtkToggleButton *) toggle))
        audgui_hide_filebrowser ();
}

static void destroy_cb (GtkWidget * chooser)
{
    CharPtr path (gtk_file_chooser_get_current_folder ((GtkFileChooser *) chooser));
    if (path)
        aud_set_str ("audgui", "filesel_path", path);
}

static void toggled_cb (GtkToggleButton * toggle, void * option)
{
    aud_set_bool ("audgui", (const char *) option, gtk_toggle_button_get_active (toggle));
}

static GtkWidget * create_filebrowser (gboolean open)
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

    int dpi = audgui_get_dpi ();

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title ((GtkWindow *) window, window_title);
    gtk_window_set_default_size ((GtkWindow *) window, 7 * dpi, 5 * dpi);
    gtk_container_set_border_width ((GtkContainer *) window, 10);

    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) window, vbox);

    GtkWidget * chooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_set_local_only ((GtkFileChooser *) chooser, false);
    gtk_file_chooser_set_select_multiple ((GtkFileChooser *) chooser, true);

    String path = aud_get_str ("audgui", "filesel_path");
    if (path[0])
        gtk_file_chooser_set_current_folder ((GtkFileChooser *) chooser, path);

    gtk_box_pack_start ((GtkBox *) vbox, chooser, true, true, 3);

    GtkWidget * hbox = gtk_hbox_new (false, 0);
    gtk_box_pack_end ((GtkBox *) vbox, hbox, false, false, 3);

    GtkWidget * toggle = gtk_check_button_new_with_mnemonic (toggle_text);
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle, aud_get_bool ("audgui", option));
    g_signal_connect (toggle, "toggled", (GCallback) toggled_cb, (void *) option);
    gtk_box_pack_start ((GtkBox *) hbox, toggle, true, true, 0);

    GtkWidget * bbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout ((GtkButtonBox *) bbox, GTK_BUTTONBOX_END);
    gtk_box_set_spacing ((GtkBox *) bbox, 6);
    gtk_box_pack_end ((GtkBox *) hbox, bbox, true, true, 0);

    GtkWidget * action_button = audgui_button_new (verb, icon, open_cb, chooser);
    GtkWidget * close_button = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) audgui_hide_filebrowser, nullptr);

    gtk_container_add ((GtkContainer *) bbox, close_button);
    gtk_container_add ((GtkContainer *) bbox, action_button);

    gtk_widget_set_can_default (action_button, true);
    gtk_widget_grab_default (action_button);

    g_object_set_data ((GObject *) chooser, "toggle-button", toggle);
    g_object_set_data ((GObject *) chooser, "do-open", GINT_TO_POINTER (open));

    g_signal_connect (chooser, "file-activated", (GCallback) open_cb, nullptr);
    g_signal_connect (chooser, "destroy", (GCallback) destroy_cb, nullptr);

    audgui_destroy_on_escape (window);

    return window;
}

EXPORT void audgui_run_filebrowser (bool open)
{
    audgui_show_unique_window (AUDGUI_FILEBROWSER_WINDOW, create_filebrowser (open));
}

EXPORT void audgui_hide_filebrowser ()
{
    audgui_hide_unique_window (AUDGUI_FILEBROWSER_WINDOW);
}
