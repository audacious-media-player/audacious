/*
 * util.c
 * Copyright 2010-2012 John Lindgren
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include <audacious/debug.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <audacious/misc.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GdkPixbuf * current_pixbuf;

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

EXPORT GdkPixbuf * audgui_pixbuf_from_data (const void * data, int64_t size)
{
    GdkPixbuf * pixbuf = NULL;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();
    GError * error = NULL;

    if (gdk_pixbuf_loader_write (loader, data, size, & error) &&
     gdk_pixbuf_loader_close (loader, & error))
    {
        if ((pixbuf = gdk_pixbuf_loader_get_pixbuf (loader)))
            g_object_ref (pixbuf);
    }
    else
    {
        AUDDBG ("error while loading pixbuf: %s\n", error->message);
        g_error_free (error);
    }

    g_object_unref (loader);
    return pixbuf;
}

/* deprecated */
EXPORT GdkPixbuf * audgui_pixbuf_for_entry (int list, int entry)
{
    char * name = aud_playlist_entry_get_filename (list, entry);
    g_return_val_if_fail (name, NULL);

    const void * data;
    int64_t size;

    aud_art_get_data (name, & data, & size);

    if (data)
    {
        GdkPixbuf * p = audgui_pixbuf_from_data (data, size);
        aud_art_unref (name);

        if (p)
        {
            str_unref (name);
            return p;
        }
    }

    str_unref (name);
    return audgui_pixbuf_fallback ();
}

EXPORT GdkPixbuf * audgui_pixbuf_fallback (void)
{
    static GdkPixbuf * fallback = NULL;

    if (! fallback)
    {
        SPRINTF (path, "%s/images/album.png", aud_get_path (AUD_PATH_DATA_DIR));
        fallback = gdk_pixbuf_new_from_file (path, NULL);
    }

    if (fallback)
        g_object_ref ((GObject *) fallback);

    return fallback;
}

void audgui_pixbuf_uncache (void)
{
    if (current_pixbuf)
    {
        g_object_unref ((GObject *) current_pixbuf);
        current_pixbuf = NULL;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/* deprecated */
EXPORT GdkPixbuf * audgui_pixbuf_for_current (void)
{
    if (! current_pixbuf)
    {
        int list = aud_playlist_get_playing ();
        current_pixbuf = audgui_pixbuf_for_entry (list, aud_playlist_get_position (list));
    }

    if (current_pixbuf)
        g_object_ref ((GObject *) current_pixbuf);

    return current_pixbuf;
}

#pragma GCC diagnostic pop

EXPORT void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size)
{
    int width = gdk_pixbuf_get_width (* pixbuf);
    int height = gdk_pixbuf_get_height (* pixbuf);

    if (width <= size && height <= size)
        return;

    if (width > height)
    {
        height = size * height / width;
        width = size;
    }
    else
    {
        width = size * width / height;
        height = size;
    }

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    GdkPixbuf * pixbuf2 = gdk_pixbuf_scale_simple (* pixbuf, width, height,
     GDK_INTERP_BILINEAR);
    g_object_unref (* pixbuf);
    * pixbuf = pixbuf2;
}

EXPORT GdkPixbuf * audgui_pixbuf_request (const char * filename)
{
    const void * data;
    int64_t size;

    aud_art_request_data (filename, & data, & size);
    if (! data)
        return NULL;

    GdkPixbuf * p = audgui_pixbuf_from_data (data, size);

    aud_art_unref (filename);
    return p;
}

EXPORT GdkPixbuf * audgui_pixbuf_request_current (void)
{
    if (! current_pixbuf)
    {
        int list = aud_playlist_get_playing ();
        int entry = aud_playlist_get_position (list);
        if (entry < 0)
            return NULL;

        char * filename = aud_playlist_entry_get_filename (list, entry);
        current_pixbuf = audgui_pixbuf_request (filename);
        str_unref (filename);
    }

    if (current_pixbuf)
        g_object_ref ((GObject *) current_pixbuf);

    return current_pixbuf;
}

EXPORT void audgui_set_default_icon (void)
{
#ifndef _WIN32
    gtk_window_set_default_icon_name ("audacious");
#endif
}
