/*
 * util.c
 * Copyright 2010-2012 John Lindgren and Michał Lipski
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <audacious/debug.h>
#include <audacious/i18n.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

EXPORT int audgui_get_digit_width (GtkWidget * widget)
{
    int width;
    PangoLayout * layout = gtk_widget_create_pango_layout (widget, "0123456789");
    PangoFontDescription * desc = pango_font_description_new ();
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    pango_layout_set_font_description (layout, desc);
    pango_layout_get_pixel_size (layout, & width, NULL);
    pango_font_description_free (desc);
    return (width + 9) / 10;
}

EXPORT void audgui_get_mouse_coords (GtkWidget * widget, int * x, int * y)
{
    if (widget)
    {
        int xwin, ywin;
        GdkRectangle alloc;

        GdkWindow * window = gtk_widget_get_window (widget);
        GdkDisplay * display = gdk_window_get_display (window);
        GdkDeviceManager * manager = gdk_display_get_device_manager (display);
        GdkDevice * device = gdk_device_manager_get_client_pointer (manager);

        gdk_window_get_device_position (window, device, & xwin, & ywin, NULL);
        gtk_widget_get_allocation (widget, & alloc);

        * x = xwin - alloc.x;
        * y = ywin - alloc.y;
    }
    else
    {
        GdkDisplay * display = gdk_display_get_default ();
        GdkDeviceManager * manager = gdk_display_get_device_manager (display);
        GdkDevice * device = gdk_device_manager_get_client_pointer (manager);
        gdk_device_get_position (device, NULL, x, y);
    }
}

EXPORT void audgui_hide_on_delete (GtkWidget * widget)
{
    g_signal_connect (widget, "delete-event", (GCallback)
     gtk_widget_hide_on_delete, NULL);
}

static bool_t escape_hide_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_widget_hide (widget);
        return TRUE;
    }

    return FALSE;
}

EXPORT void audgui_hide_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_hide_cb, NULL);
}

static bool_t escape_destroy_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_widget_destroy (widget);
        return TRUE;
    }

    return FALSE;
}

EXPORT void audgui_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_destroy_cb, NULL);
}

static void toggle_cb (GtkToggleButton * toggle, bool_t * setting)
{
    * setting = gtk_toggle_button_get_active (toggle);
}

EXPORT void audgui_connect_check_box (GtkWidget * box, bool_t * setting)
{
    gtk_toggle_button_set_active ((GtkToggleButton *) box, * setting);
    g_signal_connect ((GObject *) box, "toggled", (GCallback) toggle_cb, setting);
}

EXPORT GtkWidget * audgui_button_new (const char * text, const char * icon)
{
    GtkWidget * button = gtk_button_new_with_mnemonic (text);
    GtkWidget * image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU);
    gtk_button_set_image ((GtkButton *) button, image);

#if GTK_CHECK_VERSION (3, 6, 0)
    gtk_button_set_always_show_image ((GtkButton *) button, TRUE);
#endif

    return button;
}

EXPORT void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text)
{
    AUDDBG ("%s\n", text);

    if (* widget != NULL)
    {
        const char * old = NULL;
        g_object_get ((GObject *) * widget, "text", & old, NULL);
        g_return_if_fail (old);

        int messages = GPOINTER_TO_INT (g_object_get_data ((GObject *)
         * widget, "messages"));

        if (messages > 10)
            text = _("\n(Further messages have been hidden.)");

        if (strstr (old, text))
            goto CREATED;

        char both[strlen (old) + strlen (text) + 2];
        snprintf (both, sizeof both, "%s\n%s", old, text);
        g_object_set ((GObject *) * widget, "text", both, NULL);

        g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER
         (messages + 1));

        goto CREATED;
    }

    * widget = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_OK, "%s", text);
    gtk_window_set_title ((GtkWindow *) * widget, title);

    g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (1));

    g_signal_connect (* widget, "response", (GCallback) gtk_widget_destroy, NULL);
    audgui_destroy_on_escape (* widget);
    g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed,
     widget);

CREATED:
    gtk_window_present ((GtkWindow *) * widget);
}

EXPORT void audgui_set_default_icon (void)
{
#ifndef _WIN32
    gtk_window_set_default_icon_name ("audacious");
#endif
}
