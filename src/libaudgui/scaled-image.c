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
    GdkPixbuf * unscaled = g_object_get_data ((GObject *) widget, "pixbuf-unscaled");

    if (! unscaled)
        return NULL;

    int width = gdk_pixbuf_get_width (unscaled);
    int height = gdk_pixbuf_get_height (unscaled);

    if (width > maxwidth || height > maxheight)
    {
        if (width * maxheight > height * maxwidth)
        {
            height = height * maxwidth / width;
            width = maxwidth;
        }
        else
        {
            width = width * maxheight / height;
            height = maxheight;
        }
    }

    GdkPixbuf * scaled = g_object_get_data ((GObject *) widget, "pixbuf-scaled");

    if (scaled)
    {
        if (gdk_pixbuf_get_width (scaled) == width && gdk_pixbuf_get_height (scaled) == height)
            return scaled;

        g_object_unref (scaled);
    }

    scaled = gdk_pixbuf_scale_simple (unscaled, width, height, GDK_INTERP_BILINEAR);
    g_object_set_data ((GObject *) widget, "pixbuf-scaled", scaled);

    return scaled;
}

static bool_t draw_cb (GtkWidget * widget, cairo_t * cr)
{
    GdkRectangle rect;
    gtk_widget_get_allocation (widget, & rect);

    GdkPixbuf * scaled = get_scaled (widget, rect.width, rect.height);

    if (scaled)
    {
        int x = (rect.width - gdk_pixbuf_get_width (scaled)) / 2;
        int y = (rect.height - gdk_pixbuf_get_height (scaled)) / 2;
        gdk_cairo_set_source_pixbuf (cr, scaled, x, y);
        cairo_paint (cr);
    }

    return TRUE;
}

EXPORT void audgui_scaled_image_set (GtkWidget * widget, GdkPixbuf * pixbuf)
{
    GdkPixbuf * old;

    if ((old = g_object_get_data ((GObject *) widget, "pixbuf-unscaled")))
        g_object_unref (old);
    if ((old = g_object_get_data ((GObject *) widget, "pixbuf-scaled")))
        g_object_unref (old);

    if (pixbuf)
        g_object_ref (pixbuf);

    g_object_set_data ((GObject *) widget, "pixbuf-unscaled", pixbuf);
    g_object_set_data ((GObject *) widget, "pixbuf-scaled", NULL);

    if (! gtk_widget_in_destruction (widget))
        gtk_widget_queue_draw (widget);
}

static void destroy_cb (GtkWidget * widget)
{
    audgui_scaled_image_set (widget, NULL);
}

EXPORT GtkWidget * audgui_scaled_image_new (GdkPixbuf * pixbuf)
{
    GtkWidget * widget = gtk_drawing_area_new ();

    g_signal_connect (widget, "draw", (GCallback) draw_cb, NULL);
    g_signal_connect (widget, "destroy", (GCallback) destroy_cb, NULL);

    audgui_scaled_image_set (widget, pixbuf);

    return widget;
}
