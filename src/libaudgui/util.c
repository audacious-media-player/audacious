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
#include <audacious/misc.h>
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
    g_object_unref (layout);
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

EXPORT GtkWidget * audgui_button_new (const char * text, const char * icon,
 AudguiCallback callback, void * data)
{
    GtkWidget * button = gtk_button_new_with_mnemonic (text);

    if (icon)
    {
        GtkWidget * image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU);
        gtk_button_set_image ((GtkButton *) button, image);
    }

    if (callback)
        g_signal_connect_swapped (button, "clicked", (GCallback) callback, data);

    return button;
}

EXPORT GtkWidget * audgui_dialog_new (GtkMessageType type, const char * title,
 const char * text, GtkWidget * button1, GtkWidget * button2)
{
    GtkWidget * dialog = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_NONE, "%s", text);
    gtk_window_set_title ((GtkWindow *) dialog, title);

    if (button2)
    {
        gtk_dialog_add_action_widget ((GtkDialog *) dialog, button2, GTK_RESPONSE_NONE);
        g_signal_connect_swapped (button2, "clicked", (GCallback) gtk_widget_destroy, dialog);
    }

    gtk_dialog_add_action_widget ((GtkDialog *) dialog, button1, GTK_RESPONSE_NONE);
    g_signal_connect_swapped (button1, "clicked", (GCallback) gtk_widget_destroy, dialog);

    gtk_widget_set_can_default (button1, TRUE);
    gtk_widget_grab_default (button1);

    return dialog;
}

EXPORT void audgui_dialog_add_widget (GtkWidget * dialog, GtkWidget * widget)
{
    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    gtk_box_pack_start ((GtkBox *) box, widget, FALSE, FALSE, 0);
}

EXPORT void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text)
{
    AUDDBG ("%s\n", text);

    if (* widget)
    {
        const char * old = NULL;
        g_object_get ((GObject *) * widget, "text", & old, NULL);
        g_return_if_fail (old);

        int messages = GPOINTER_TO_INT (g_object_get_data ((GObject *) * widget, "messages"));
        if (messages > 10)
            text = _("\n(Further messages have been hidden.)");

        if (! strstr (old, text))
        {
            SCONCAT3 (both, old, "\n", text);
            g_object_set ((GObject *) * widget, "text", both, NULL);
            g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (messages + 1));
        }

        gtk_window_present ((GtkWindow *) * widget);
    }
    else
    {
        GtkWidget * button = audgui_button_new (_("_Close"), "window-close", NULL, NULL);
        * widget = audgui_dialog_new (type, title, text, button, NULL);

        g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (1));
        g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed, widget);

        gtk_widget_show_all (* widget);
    }
}

EXPORT void audgui_format_time (char * buf, int bufsize, int64_t milliseconds)
{
    int hours = milliseconds / 3600000;
    int minutes = (milliseconds / 60000) % 60;
    int seconds = (milliseconds / 1000) % 60;

    if (hours)
        snprintf (buf, bufsize, "%d:%02d:%02d", hours, minutes, seconds);
    else
    {
        bool_t zero = aud_get_bool (NULL, "leading_zero");
        snprintf (buf, bufsize, zero ? "%02d:%02d" : "%d:%02d", minutes, seconds);
    }
}
