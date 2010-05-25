/*
 * libaudgui/util.c
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "libaudgui.h"
#include "libaudgui-gtk.h"

static gboolean delete_hide (GtkWidget * widget, GdkEvent * event, void * unused)
{
    gtk_widget_hide (widget);
    return TRUE;
}

void audgui_hide_on_delete (GtkWidget * widget)
{
    g_signal_connect (widget, "delete-event", (GCallback) delete_hide, NULL);
}

static gboolean keypress_hide (GtkWidget * widget, GdkEventKey * event, void *
 unused)
{
    if (event->keyval == GDK_Escape)
    {
        gtk_widget_hide (widget);
        return TRUE;
    }

    return FALSE;
}

void audgui_hide_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) keypress_hide, NULL);
}

static gboolean keypress_destroy (GtkWidget * widget, GdkEventKey * event,
 void * unused)
{
    if (event->keyval == GDK_Escape)
    {
        gtk_widget_destroy (widget);
        return TRUE;
    }

    return FALSE;
}

void audgui_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) keypress_destroy,
     NULL);
}

void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const gchar * title, const gchar * text)
{
    if (* widget == NULL)
    {
        * widget = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_OK, "%s",
         text);
        gtk_window_set_title ((GtkWindow *) * widget, title);

        g_signal_connect (* widget, "response", (GCallback) gtk_widget_destroy,
         NULL);
        audgui_destroy_on_escape (* widget);
        g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed,
         widget);
    }

    gtk_window_present ((GtkWindow *) * widget);
}

GdkPixbuf * audgui_pixbuf_from_data (void * data, gint size)
{
    GdkPixbuf * pixbuf = NULL;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();

    if (gdk_pixbuf_loader_write (loader, data, size, NULL))
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

    gdk_pixbuf_loader_close (loader, NULL);
    return pixbuf;
}

void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, gint size)
{
    gint width = gdk_pixbuf_get_width (* pixbuf);
    gint height = gdk_pixbuf_get_height (* pixbuf);
    GdkPixbuf * pixbuf2;

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

    pixbuf2 = gdk_pixbuf_scale_simple (* pixbuf, width, height,
     GDK_INTERP_BILINEAR);
    g_object_unref (* pixbuf);
    * pixbuf = pixbuf2;
}
