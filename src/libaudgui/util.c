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

#define DEBUG
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include <audacious/debug.h>

#include "libaudgui.h"
#include "libaudgui-gtk.h"

void audgui_hide_on_delete (GtkWidget * widget)
{
    g_signal_connect (widget, "delete-event", (GCallback)
     gtk_widget_hide_on_delete, NULL);
}

static gboolean escape_cb (GtkWidget * widget, GdkEventKey * event, void
 (* action) (GtkWidget * widget))
{
    if (event->keyval == GDK_Escape)
    {
        action (widget);
        return TRUE;
    }

    return FALSE;
}

void audgui_hide_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_cb,
     gtk_widget_hide);
}

void audgui_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_cb,
     gtk_widget_destroy);
}

static void toggle_cb (GtkToggleButton * toggle, gboolean * setting)
{
    * setting = gtk_toggle_button_get_active (toggle);
}

void audgui_connect_check_box (GtkWidget * box, gboolean * setting)
{
    gtk_toggle_button_set_active ((GtkToggleButton *) box, * setting);
    g_signal_connect ((GObject *) box, "toggled", (GCallback) toggle_cb, setting);
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
    GError * error = NULL;

    if (gdk_pixbuf_loader_write (loader, data, size, &error))
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    else
        AUDDBG("error while loading pixbuf: %s\n", error->message);

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
