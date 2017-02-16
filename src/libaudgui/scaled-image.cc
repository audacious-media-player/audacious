/*
 * scaled-image.c
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

#include <libaudgui/libaudgui-gtk.h>

static GdkPixbuf * get_scaled (GtkWidget * widget, int maxwidth, int maxheight)
{
    GdkPixbuf * unscaled = (GdkPixbuf *) g_object_get_data ((GObject *) widget, "pixbuf-unscaled");

    if (! unscaled)
        return nullptr;

    int width = gdk_pixbuf_get_width (unscaled);
    int height = gdk_pixbuf_get_height (unscaled);

    if (width > maxwidth || height > maxheight)
    {
        if (width * maxheight > height * maxwidth)
        {
            height = aud::rescale (height, width, maxwidth);
            width = maxwidth;
        }
        else
        {
            width = aud::rescale (width, height, maxheight);
            height = maxheight;
        }
    }

    GdkPixbuf * scaled = (GdkPixbuf *) g_object_get_data ((GObject *) widget, "pixbuf-scaled");

    if (scaled)
    {
        if (gdk_pixbuf_get_width (scaled) == width && gdk_pixbuf_get_height (scaled) == height)
            return scaled;
    }

    scaled = gdk_pixbuf_scale_simple (unscaled, width, height, GDK_INTERP_BILINEAR);
    g_object_set_data_full ((GObject *) widget, "pixbuf-scaled", scaled, g_object_unref);

    return scaled;
}

static int expose_cb (GtkWidget * widget, GdkEventExpose * event)
{
    GdkRectangle rect;
    gtk_widget_get_allocation (widget, & rect);

    GdkPixbuf * scaled = get_scaled (widget, rect.width, rect.height);

    if (scaled)
    {
        int x = (rect.width - gdk_pixbuf_get_width (scaled)) / 2;
        int y = (rect.height - gdk_pixbuf_get_height (scaled)) / 2;

        cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));
        gdk_cairo_set_source_pixbuf (cr, scaled, x, y);
        cairo_paint (cr);
        cairo_destroy (cr);
    }

    return true;
}

EXPORT void audgui_scaled_image_set (GtkWidget * widget, GdkPixbuf * pixbuf)
{
    if (pixbuf)
        g_object_ref (pixbuf);

    g_object_set_data_full ((GObject *) widget, "pixbuf-unscaled", pixbuf, g_object_unref);
    g_object_set_data_full ((GObject *) widget, "pixbuf-scaled", nullptr, g_object_unref);

    gtk_widget_queue_draw (widget);
}

EXPORT GtkWidget * audgui_scaled_image_new (GdkPixbuf * pixbuf)
{
    GtkWidget * widget = gtk_drawing_area_new ();

    g_signal_connect (widget, "expose-event", (GCallback) expose_cb, nullptr);

    audgui_scaled_image_set (widget, pixbuf);

    return widget;
}
