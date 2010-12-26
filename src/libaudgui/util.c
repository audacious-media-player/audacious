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

#include <audacious/debug.h>
#include <audacious/playlist.h>
#include <audacious/plugin.h>
#include <audacious/misc.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

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
    if (* widget != NULL)
        gtk_widget_destroy (* widget);

    * widget = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_OK, "%s", text);
    gtk_window_set_title ((GtkWindow *) * widget, title);

    g_signal_connect (* widget, "response", (GCallback) gtk_widget_destroy, NULL);
    audgui_destroy_on_escape (* widget);
    g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed,
     widget);

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

GdkPixbuf * audgui_pixbuf_for_entry (gint list, gint entry)
{
    const gchar * name = aud_playlist_entry_get_filename (list, entry);
    g_return_val_if_fail (name, NULL);

    /* Don't get album art for network files -- too slow. */
    if (! strncmp (name, "http://", 7) || ! strncmp (name, "https://", 8) ||
     ! strncmp (name, "mms://", 6))
        goto FALLBACK;

    AUDDBG ("Trying to load pixbuf for %s.\n", name);
    PluginHandle * decoder = aud_playlist_entry_get_decoder (list, entry, FALSE);
    if (! decoder)
        goto FALLBACK;

    void * data;
    gint size;

    if (aud_file_read_image (name, decoder, & data, & size))
    {
        GdkPixbuf * p = audgui_pixbuf_from_data (data, size);
        g_free (data);
        if (p)
            return p;
    }

    gchar * assoc = aud_get_associated_image_file (name);

    if (assoc)
    {
        GdkPixbuf * p = gdk_pixbuf_new_from_file (assoc, NULL);
        g_free (assoc);
        if (p)
            return p;
    }

FALLBACK:;
    AUDDBG ("Using fallback pixbuf.\n");
    static GdkPixbuf * fallback = NULL;
    if (! fallback)
    {
        gchar * path = g_strdup_printf ("%s/images/album.png",
         aud_get_path (AUD_PATH_DATA_DIR));
        fallback = gdk_pixbuf_new_from_file (path, NULL);
        g_free (path);
    }
    if (fallback)
        g_object_ref ((GObject *) fallback);
    return fallback;
}


static void clear_cached_pixbuf (void * list, GdkPixbuf * * pixbuf)
{
    if (GPOINTER_TO_INT (list) != aud_playlist_get_playing () || ! * pixbuf)
        return;

    AUDDBG ("Clearing cached pixbuf.\n");
    g_object_unref ((GObject *) * pixbuf);
    * pixbuf = NULL;
}

GdkPixbuf * audgui_pixbuf_for_current (void)
{
    static GdkPixbuf * pixbuf = NULL;
    static gboolean hooked = FALSE;

    if (! hooked)
    {
        hook_associate ("playlist position", (HookFunction) clear_cached_pixbuf,
         & pixbuf);
        hooked = TRUE;
    }

    if (! pixbuf)
    {
        gint list = aud_playlist_get_playing ();
        pixbuf = audgui_pixbuf_for_entry (list, aud_playlist_get_position (list));
    }

    if (pixbuf)
        g_object_ref ((GObject *) pixbuf);
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

gchar * skip_top_folders (gchar * name)
{
    const gchar * home = getenv ("HOME");
    if (! home)
        goto NO_HOME;

    gint len = strlen (home);
    if (len > 0 && home[len - 1] == '/')
        len --;

    if (! strncmp (name, home, len) && name[len] == '/')
        return name + len + 1;

NO_HOME:
    return name[0] == '/' ? name + 1 : name;
}

void split_filename (gchar * name, gchar * * base, gchar * * first, gchar * *
 second)
{
    * first = * second = NULL;

    gchar * c;

    if ((c = strrchr (name, '/')))
    {
        * base = c + 1;
        * c = 0;
    }
    else
    {
        * base = name;
        goto DONE;
    }

    if ((c = strrchr (name, '/')))
    {
        * first = c + 1;
        * c = 0;
    }
    else
    {
        * first = name;
        goto DONE;
    }

    if ((c = strrchr (name, '/')))
        * second = c + 1;
    else
        * second = name;

DONE:
    if ((c = strrchr (* base, '.')))
        * c = 0;
}

gchar * stream_name (const gchar * name)
{
    if (! strncmp (name, "http://", 7))
        name += 7;
    else if (! strncmp (name, "https://", 8))
        name += 8;
    else if (! strncmp (name, "mms://", 6))
        name += 6;
    else
        return NULL;

    gchar * s = g_strdup (name);
    gchar * c;

    if ((c = strchr (s, '/')))
        * c = 0;
    if ((c = strchr (s, ':')))
        * c = 0;
    if ((c = strchr (s, '?')))
        * c = 0;

    return s;
}

void audgui_three_strings (gint list, gint entry, gchar * * title, gchar * *
 artist, gchar * * album)
{
    * title = * artist = * album = NULL;

    const gchar * name = aud_playlist_entry_get_filename (list, entry);
    const Tuple * tuple = aud_playlist_entry_get_tuple (list, entry, FALSE);
    g_return_if_fail (name && tuple);

    const gchar * _title = tuple_get_string (tuple, FIELD_TITLE, NULL);
    const gchar * _artist = tuple_get_string (tuple, FIELD_ARTIST, NULL);
    const gchar * _album = tuple_get_string (tuple, FIELD_ALBUM, NULL);

    * title = (_title && _title[0]) ? g_strdup (_title) : NULL;
    * artist = (_artist && _artist[0]) ? g_strdup (_artist) : NULL;
    * album = (_album && _album[0]) ? g_strdup (_album) : NULL;

    gchar * copy = uri_to_display (name);

    if (copy[0] == '/')
    {
        gchar * base, * first, * second;
        split_filename (skip_top_folders (copy), & base, & first,
         & second);

        if (! * title)
            * title = g_strdup (base);

        if (first)
        {
            if (second && ! * artist && ! * album)
            {
                * artist = g_strdup (second);
                * album = g_strdup (first);
            }
            else if (! * artist)
                * artist = g_strdup (first);
            else if (! * album)
                * album = g_strdup (first);
        }
    }
    else
    {
        if (! * title)
            * title = stream_name (copy);
        else if (! * artist)
            * artist = stream_name (copy);
        else if (! * album)
            * album = stream_name (copy);
    }

    g_free (copy);
}
