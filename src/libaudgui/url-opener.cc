/*
 * url-opener.c
 * Copyright 2012-2013 John Lindgren
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

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void open_cb (void * entry)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) entry);
    gboolean open = GPOINTER_TO_INT (g_object_get_data ((GObject *) entry, "open"));

    if (open)
        aud_drct_pl_open (text);
    else
        aud_drct_pl_add (text, -1);

    if (aud_get_bool (nullptr, "save_url_history"))
        aud_history_add (text);
}

static void clear_cb (void * combo)
{
    /* no gtk_combo_box_text_clear()? */
    gtk_list_store_clear ((GtkListStore *) gtk_combo_box_get_model ((GtkComboBox *) combo));
    aud_history_clear ();
}

static GtkWidget * create_url_opener (bool open)
{
    static const PreferencesWidget widgets[] = {
        WidgetCheck (N_("_Save to history"),
            WidgetBool (0, "save_url_history"))
    };

    const char * title, * verb, * icon;

    if (open)
    {
        title = _("Open URL");
        verb = _("_Open");
        icon = "document-open";
    }
    else
    {
        title = _("Add URL");
        verb = _("_Add");
        icon = "list-add";
    }

    GtkWidget * combo = gtk_combo_box_text_new_with_entry ();
    GtkWidget * entry = gtk_bin_get_child ((GtkBin *) combo);
    gtk_entry_set_activates_default ((GtkEntry *) entry, true);

    for (int i = 0;; i++)
    {
        String item = aud_history_get (i);
        if (! item)
            break;

        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, item);
    }

    g_object_set_data ((GObject *) entry, "open", GINT_TO_POINTER (open));

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    audgui_create_widgets (hbox, widgets);

    GtkWidget * clear_button = audgui_button_new (_("C_lear history"),
     "edit-clear", clear_cb, combo);
    gtk_box_pack_end ((GtkBox *) hbox, clear_button, false, false, 0);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, combo, false, false, 0);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    GtkWidget * button1 = audgui_button_new (verb, icon, open_cb, entry);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", nullptr, nullptr);

    GtkWidget * dialog = audgui_dialog_new (GTK_MESSAGE_OTHER, title,
     _("Enter URL:"), button1, button2);
    gtk_widget_set_size_request (dialog, 4 * audgui_get_dpi (), -1);
    audgui_dialog_add_widget (dialog, vbox);

    return dialog;
}

EXPORT void audgui_show_add_url_window (bool open)
{
    audgui_show_unique_window (AUDGUI_URL_OPENER_WINDOW, create_url_opener (open));
}
