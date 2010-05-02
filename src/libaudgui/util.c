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
