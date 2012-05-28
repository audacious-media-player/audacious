/*
 * plugin-preferences.c
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

#include "i18n.h"
#include "misc.h"
#include "preferences.h"
#include "ui_preferences.h"

static void response_cb (GtkWidget * window, int response, PluginPreferences * p)
{
    if (response == GTK_RESPONSE_OK && p->apply)
        p->apply ();
    if (response == GTK_RESPONSE_CANCEL && p->cancel)
        p->cancel ();

    gtk_widget_destroy (window);
}

static void destroy_cb (GtkWidget * window, PluginPreferences * p)
{
    p->data = NULL;

    if (p->cleanup)
        p->cleanup ();
}

void plugin_preferences_show (PluginPreferences * p)
{
    if (p->data)
    {
        gtk_window_present ((GtkWindow *) p->data);
        return;
    }

    if (p->init)
        p->init ();

    const char * title = dgettext (p->domain, p->title);
    GtkWidget * window = p->apply ? gtk_dialog_new_with_buttons (title, NULL, 0,
     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL)
     : gtk_dialog_new_with_buttons (title, NULL, 0, GTK_STOCK_CLOSE,
     GTK_RESPONSE_CLOSE, NULL);

    GtkWidget * content = gtk_dialog_get_content_area ((GtkDialog *) window);
    GtkWidget * box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    create_widgets_with_domain (box, p->prefs, p->n_prefs, p->domain);
    gtk_box_pack_start ((GtkBox *) content, box, TRUE, TRUE, 0);

    g_signal_connect (window, "response", (GCallback) response_cb, p);
    g_signal_connect (window, "destroy", (GCallback) destroy_cb, p);

    p->data = window;

    gtk_widget_show_all (window);
}

void plugin_preferences_cleanup (PluginPreferences * p)
{
    if (p->data)
        gtk_widget_destroy (p->data);
}
