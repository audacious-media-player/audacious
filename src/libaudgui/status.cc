/*
 * status.c
 * Copyright 2014 John Lindgren
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

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>

#include "internal.h"
#include "libaudgui-gtk.h"

static GtkWidget * progress_window;
static GtkWidget * progress_label, * progress_label_2;
static GtkWidget * error_window, * info_window;

static void create_progress_window ()
{
    progress_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) progress_window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title ((GtkWindow *) progress_window, _("Working ..."));
    gtk_window_set_resizable ((GtkWindow *) progress_window, false);
    gtk_container_set_border_width ((GtkContainer *) progress_window, 6);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_add ((GtkContainer *) progress_window, vbox);

    progress_label = gtk_label_new (nullptr);
    gtk_label_set_width_chars ((GtkLabel *) progress_label, 40);
    gtk_label_set_max_width_chars ((GtkLabel *) progress_label, 40);
    gtk_label_set_ellipsize ((GtkLabel *) progress_label, PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_pack_start ((GtkBox *) vbox, progress_label, false, false, 0);

    progress_label_2 = gtk_label_new (nullptr);
    gtk_label_set_width_chars ((GtkLabel *) progress_label_2, 40);
    gtk_label_set_max_width_chars ((GtkLabel *) progress_label_2, 40);
    gtk_label_set_ellipsize ((GtkLabel *) progress_label, PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_pack_start ((GtkBox *) vbox, progress_label_2, false, false, 0);

    gtk_widget_show_all (progress_window);

    g_signal_connect (progress_window, "destroy",
     (GCallback) gtk_widget_destroyed, & progress_window);
}

static void show_progress (void * data, void * user)
{
    if (! progress_window)
        create_progress_window ();

    gtk_label_set_text ((GtkLabel *) progress_label, (const char *) data);
}

static void show_progress_2 (void * data, void * user)
{
    if (! progress_window)
        create_progress_window ();

    gtk_label_set_text ((GtkLabel *) progress_label_2, (const char *) data);
}

static void hide_progress (void * data, void * user)
{
    if (progress_window)
        gtk_widget_destroy (progress_window);
}

static void show_error (void * data, void * user)
{
    audgui_simple_message (& error_window, GTK_MESSAGE_ERROR, _("Error"), (const char *) data);
}

static void show_info (void * data, void * user)
{
    audgui_simple_message (& info_window, GTK_MESSAGE_INFO, _("Information"), (const char *) data);
}

void status_init ()
{
    hook_associate ("ui show progress", show_progress, nullptr);
    hook_associate ("ui show progress 2", show_progress_2, nullptr);
    hook_associate ("ui hide progress", hide_progress, nullptr);
    hook_associate ("ui show error", show_error, nullptr);
    hook_associate ("ui show info", show_info, nullptr);
}

void status_cleanup ()
{
    hook_dissociate ("ui show progress", show_progress);
    hook_dissociate ("ui show progress 2", show_progress_2);
    hook_dissociate ("ui hide progress", hide_progress);
    hook_dissociate ("ui show error", show_error);
    hook_dissociate ("ui show info", show_info);

    if (progress_window)
        gtk_widget_destroy (progress_window);
    if (error_window)
        gtk_widget_destroy (error_window);
    if (info_window)
        gtk_widget_destroy (info_window);
}
