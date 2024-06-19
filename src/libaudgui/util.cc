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

#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#define AUD_GLIB_INTEGRATION
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudgui-gtk.h"

#define PORTABLE_DPI 96

EXPORT int audgui_get_dpi ()
{
    static int dpi = 0;

    if (! dpi)
    {
#ifdef _WIN32
        HDC screen = GetDC (nullptr);
        dpi = (GetDeviceCaps (screen, LOGPIXELSX) + GetDeviceCaps (screen, LOGPIXELSY)) / 2;
        ReleaseDC (nullptr, screen);
#else
        GdkScreen * screen = gdk_screen_get_default ();

        /* force GTK settings to be loaded for the GDK screen */
        (void) gtk_settings_get_for_screen (screen);

        dpi = round (gdk_screen_get_resolution (screen));
#endif

        dpi = aud::max (PORTABLE_DPI, dpi);
    }

    return dpi;
}

EXPORT int audgui_to_native_dpi (int size)
{
    return aud::rescale (size, PORTABLE_DPI, audgui_get_dpi ());
}

EXPORT int audgui_to_portable_dpi (int size)
{
    return aud::rescale (size, audgui_get_dpi (), PORTABLE_DPI);
}

EXPORT int audgui_get_digit_width (GtkWidget * widget)
{
    int width;
    PangoLayout * layout = gtk_widget_create_pango_layout (widget, "0123456789");
    PangoFontDescription * desc = pango_font_description_new ();
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    pango_layout_set_font_description (layout, desc);
    pango_layout_get_pixel_size (layout, & width, nullptr);
    pango_font_description_free (desc);
    g_object_unref (layout);
    return (width + 9) / 10;
}

EXPORT void audgui_get_mouse_coords (GtkWidget * widget, int * x, int * y)
{
#ifdef USE_GTK3
    int xwin, ywin;
    GdkRectangle alloc;

    GdkWindow * window = gtk_widget_get_window (widget);
    GdkDisplay * display = gdk_window_get_display (window);
#if GTK_CHECK_VERSION(3, 20, 0)
    GdkSeat * seat = gdk_display_get_default_seat (display);
    GdkDevice * device = gdk_seat_get_pointer (seat);
#else
    GdkDeviceManager * manager = gdk_display_get_device_manager (display);
    GdkDevice * device = gdk_device_manager_get_client_pointer (manager);
#endif

    gdk_window_get_device_position (window, device, & xwin, & ywin, nullptr);
    gtk_widget_get_allocation (widget, & alloc);

    * x = xwin - alloc.x;
    * y = ywin - alloc.y;
#else
    gtk_widget_get_pointer (widget, x, y);
#endif
}

EXPORT void audgui_get_mouse_coords (GdkScreen * screen, int * x, int * y)
{
#ifdef USE_GTK3
    GdkDisplay * display = gdk_screen_get_display (screen);
#if GTK_CHECK_VERSION(3, 20, 0)
    GdkSeat * seat = gdk_display_get_default_seat (display);
    GdkDevice * device = gdk_seat_get_pointer (seat);
#else
    GdkDeviceManager * manager = gdk_display_get_device_manager (display);
    GdkDevice * device = gdk_device_manager_get_client_pointer (manager);
#endif
    gdk_device_get_position (device, nullptr, x, y);
#else
    gdk_display_get_pointer (gdk_screen_get_display (screen), nullptr, x, y, nullptr);
#endif
}

EXPORT void audgui_get_monitor_geometry (GdkScreen * screen, int x, int y, GdkRectangle * geom)
{
#if GTK_CHECK_VERSION(3, 22, 0)
    GdkDisplay * display = gdk_screen_get_display (screen);
    GdkMonitor * monitor = gdk_display_get_monitor_at_point (display, x, y);
    gdk_monitor_get_geometry (monitor, geom);
#else
    int monitors = gdk_screen_get_n_monitors (screen);

    for (int i = 0; i < monitors; i ++)
    {
        gdk_screen_get_monitor_geometry (screen, i, geom);
        if (x >= geom->x && x < geom->x + geom->width && y >= geom->y && y < geom->y + geom->height)
            return;
    }

    /* fall back to entire screen */
    geom->x = 0;
    geom->y = 0;
    geom->width = gdk_screen_get_width (screen);
    geom->height = gdk_screen_get_height (screen);
#endif
}

static gboolean escape_destroy_cb (GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_widget_destroy (widget);
        return true;
    }

    return false;
}

EXPORT void audgui_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_destroy_cb, nullptr);
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

struct FileEntryData {
    GtkFileChooserAction action;
    String title;
};

static void entry_response_cb (GtkWidget * dialog, int response, GtkWidget * entry)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        CharPtr uri (gtk_file_chooser_get_uri ((GtkFileChooser *) dialog));
        if (uri)
            audgui_file_entry_set_uri (entry, uri);
    }

    gtk_widget_destroy (dialog);
}

static void entry_browse_cb (GtkWidget * entry, GtkEntryIconPosition pos,
 GdkEvent * event, const FileEntryData * data)
{
    GtkWidget * dialog = gtk_file_chooser_dialog_new (data->title, nullptr,
     data->action, _("Open"), GTK_RESPONSE_ACCEPT, _("Cancel"),
     GTK_RESPONSE_REJECT, nullptr);

    gtk_file_chooser_set_local_only ((GtkFileChooser *) dialog, false);

    String uri = audgui_file_entry_get_uri (entry);
    if (uri)
        gtk_file_chooser_set_uri ((GtkFileChooser *) dialog, uri);

    g_signal_connect (dialog, "response", (GCallback) entry_response_cb, entry);
    g_signal_connect_object (entry, "destroy", (GCallback) gtk_widget_destroy,
     dialog, G_CONNECT_SWAPPED);

    gtk_widget_show (dialog);
}

EXPORT GtkWidget * audgui_file_entry_new (GtkFileChooserAction action, const char * title)
{
    GtkWidget * entry = gtk_entry_new ();

    auto data = new FileEntryData {action, String (title)};
    g_object_set_data_full ((GObject *) entry, "file-entry-data", data,
     aud::delete_obj<FileEntryData>);

    gtk_entry_set_icon_from_icon_name ((GtkEntry *) entry,
     GTK_ENTRY_ICON_SECONDARY, "document-open");
    g_signal_connect (entry, "icon-press", (GCallback) entry_browse_cb, data);

    return entry;
}

EXPORT String audgui_file_entry_get_uri (GtkWidget * entry)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) entry);

    if (! text[0])
        return String ();
    else if (strstr (text, "://"))
        return String (text);
    else
        return String (filename_to_uri (filename_normalize (filename_expand (str_copy (text)))));
}

EXPORT void audgui_file_entry_set_uri (GtkWidget * entry, const char * uri)
{
    if (! uri || ! uri[0])
    {
        gtk_entry_set_text ((GtkEntry *) entry, "");
        return;
    }

    StringBuf path = uri_to_filename (uri, false);
    gtk_entry_set_text ((GtkEntry *) entry, path ? filename_contract (std::move (path)) : uri);
    gtk_editable_set_position ((GtkEditable *) entry, -1);
}

static void set_label_properties (GtkWidget * label, void * label_selectable)
{
    if (GTK_IS_LABEL (label))
    {
        gtk_label_set_selectable ((GtkLabel *) label, GPOINTER_TO_INT (label_selectable));
        gtk_label_set_line_wrap_mode ((GtkLabel *) label, PANGO_WRAP_WORD_CHAR);
    }
}

#ifdef USE_GTK3
static const char * icon_for_message_type (GtkMessageType type)
{
    switch (type)
    {
        case GTK_MESSAGE_INFO: return "dialog-information";
        case GTK_MESSAGE_WARNING: return "dialog-warning";
        case GTK_MESSAGE_QUESTION: return "dialog-question";
        case GTK_MESSAGE_ERROR: return "dialog-error";
        default: return nullptr;
    }
}
#endif

EXPORT GtkWidget * audgui_dialog_new (GtkMessageType type, const char * title,
 const char * text, GtkWidget * button1, GtkWidget * button2)
{
    GtkWidget * dialog = gtk_message_dialog_new (nullptr, (GtkDialogFlags) 0, type,
     GTK_BUTTONS_NONE, "%s", text);
    gtk_window_set_title ((GtkWindow *) dialog, title);
    gtk_window_set_role ((GtkWindow *) dialog, "message");

    bool label_selectable = (type != GTK_MESSAGE_OTHER);
    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    gtk_container_foreach ((GtkContainer *) box, set_label_properties,
     GINT_TO_POINTER (label_selectable));

#ifdef USE_GTK3
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    const char * icon = icon_for_message_type (type);
    if (icon)
    {
        GtkWidget * image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_DIALOG);
        gtk_message_dialog_set_image ((GtkMessageDialog *) dialog, image);
    }
G_GNUC_END_IGNORE_DEPRECATIONS
#endif

    if (button2)
    {
        gtk_dialog_add_action_widget ((GtkDialog *) dialog, button2, GTK_RESPONSE_NONE);
        g_signal_connect_swapped (button2, "clicked", (GCallback) gtk_widget_destroy, dialog);
    }

    gtk_dialog_add_action_widget ((GtkDialog *) dialog, button1, GTK_RESPONSE_NONE);
    g_signal_connect_swapped (button1, "clicked", (GCallback) gtk_widget_destroy, dialog);

    gtk_widget_set_can_default (button1, true);
    gtk_widget_grab_default (button1);

    return dialog;
}

EXPORT void audgui_dialog_add_widget (GtkWidget * dialog, GtkWidget * widget)
{
    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    gtk_box_pack_start ((GtkBox *) box, widget, false, false, 0);
}

EXPORT void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text)
{
    if (type == GTK_MESSAGE_ERROR)
        AUDERR ("%s\n", text);
    else if (type == GTK_MESSAGE_WARNING)
        AUDWARN ("%s\n", text);
    else if (type == GTK_MESSAGE_INFO)
        AUDINFO ("%s\n", text);

    if (* widget)
    {
        char * old = nullptr;
        g_object_get ((GObject *) * widget, "text", & old, nullptr);
        g_return_if_fail (old);

        int messages = GPOINTER_TO_INT (g_object_get_data ((GObject *) * widget, "messages"));
        if (messages > 10)
            text = _("\n(Further messages have been hidden.)");

        if (! strstr (old, text))
        {
            StringBuf both = str_concat ({old, "\n", text});
            g_object_set ((GObject *) * widget, "text", (const char *) both, nullptr);
            g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (messages + 1));
        }

        g_free (old);
        gtk_window_present ((GtkWindow *) * widget);
    }
    else
    {
        GtkWidget * button = audgui_button_new (_("_Close"), "window-close", nullptr, nullptr);
        * widget = audgui_dialog_new (type, title, text, button, nullptr);

        g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (1));
        g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed, widget);

        gtk_widget_show_all (* widget);
    }
}

EXPORT cairo_pattern_t * audgui_dark_bg_gradient (const GdkColor & base, int height)
{
    float r = 1, g = 1, b = 1;

    /* in a dark theme, try to match the tone of the base color */
    int v = aud::max (aud::max (base.red, base.green), base.blue);

    if (v >= 10*256 && v < 80*256)
    {
        r = (float) base.red / v;
        g = (float) base.green / v;
        b = (float) base.blue / v;
    }

    cairo_pattern_t * gradient = cairo_pattern_create_linear (0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb (gradient, 0, 0.16 * r, 0.16 * g, 0.16 * b);
    cairo_pattern_add_color_stop_rgb (gradient, 0.45, 0.11 * r, 0.11 * g, 0.11 * b);
    cairo_pattern_add_color_stop_rgb (gradient, 0.55, 0.06 * r, 0.06 * g, 0.06 * b);
    cairo_pattern_add_color_stop_rgb (gradient, 1, 0.09 * r, 0.09 * g, 0.09 * b);
    return gradient;
}

static void rgb_to_hsv (float r, float g, float b, float * h, float * s, float * v)
{
    float max = aud::max (aud::max (r, g), b);
    float min = aud::min (aud::min (r, g), b);

    * v = max;

    if (max == min)
    {
        * h = 0;
        * s = 0;
        return;
    }

    if (r == max)
        * h = 1 + (g - b) / (max - min);
    else if (g == max)
        * h = 3 + (b - r) / (max - min);
    else
        * h = 5 + (r - g) / (max - min);

    * s = (max - min) / max;
}

static void hsv_to_rgb (float h, float s, float v, float * r, float * g, float * b)
{
    for (; h >= 2; h -= 2)
    {
        float * p = r;
        r = g;
        g = b;
        b = p;
    }

    if (h < 1)
    {
        * r = 1;
        * g = 0;
        * b = 1 - h;
    }
    else
    {
        * r = 1;
        * g = h - 1;
        * b = 0;
    }

    * r = v * (1 - s * (1 - * r));
    * g = v * (1 - s * (1 - * g));
    * b = v * (1 - s * (1 - * b));
}

EXPORT void audgui_vis_bar_color (const GdkColor & hue, int bar, int n_bars,
                                  float & r, float & g, float & b)
{
    float h, s, v;
    rgb_to_hsv (hue.red / 65535.0, hue.green / 65535.0, hue.blue / 65535.0, & h, & s, & v);

    if (s < 0.1) /* monochrome theme? use blue instead */
        h = 4.6;

    s = 1 - 0.9 * bar / (n_bars - 1);
    v = 0.75 + 0.25 * bar / (n_bars - 1);

    hsv_to_rgb (h, s, v, & r, & g, & b);
}
