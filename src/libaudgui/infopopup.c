/*
 * libaudgui/infopopup.c
 * Copyright 2006 William Pitcock, Tony Vroon, George Averill, Giacomo Lozito,
 *  Derek Pomery and Yoshiki Yazawa.
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

#include <gtk/gtk.h>
#include <string.h>

#include <audacious/audconfig.h>
#include <audacious/drct.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static const gchar * get_default_artwork (void)
{
    static gchar * path = NULL;
    if (! path)
        path = g_strdup_printf ("%s/images/audio.png",
         aud_get_path (AUD_PATH_DATA_DIR));
    return path;
}

#define DEFAULT_ARTWORK (get_default_artwork ())

static GtkWidget * infopopup = NULL;

static void infopopup_entry_set_text (const gchar * entry_name, const gchar *
 text)
{
    GtkWidget * widget = g_object_get_data ((GObject *) infopopup, entry_name);

    g_return_if_fail (widget != NULL);
    gtk_label_set_text ((GtkLabel *) widget, text);
}

static void infopopup_entry_set_image (const gchar * entry_name, const gchar *
 text)
{
    GtkWidget * widget = g_object_get_data ((GObject *) infopopup, entry_name);
    GdkPixbuf * pixbuf;
    gint width, height;
    gfloat aspect;

    pixbuf = gdk_pixbuf_new_from_file (text, NULL);
    g_return_if_fail (pixbuf != NULL);

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);

    if (strcmp (DEFAULT_ARTWORK, text))
    {
        GdkPixbuf * pixbuf2;

        aspect = height / (gfloat) width;

        if (aspect > 1)
        {
            height = aud_cfg->filepopup_pixelsize * aspect;
            width = aud_cfg->filepopup_pixelsize;
        }
        else
        {
            height = aud_cfg->filepopup_pixelsize;
            width = aud_cfg->filepopup_pixelsize / aspect;
        }

        pixbuf2 = gdk_pixbuf_scale_simple (pixbuf, width, height,
         GDK_INTERP_BILINEAR);
        g_object_unref (pixbuf);
        pixbuf = pixbuf2;
    }

    gtk_image_set_from_pixbuf ((GtkImage *) widget, pixbuf);
    g_object_unref (pixbuf);
}

static gboolean infopopup_progress_cb (void * unused)
{
    GtkWidget * progressbar = g_object_get_data ((GObject *) infopopup,
     "progressbar");
    gchar * tooltip_file = g_object_get_data ((GObject *) infopopup, "file");
    gint length = GPOINTER_TO_INT (g_object_get_data ((GObject *) infopopup,
     "length"));
    gint playlist, entry, time;
    const gchar * filename;
    gchar * progress_time;

    g_return_val_if_fail (tooltip_file != NULL, FALSE);
    g_return_val_if_fail (length > 0, FALSE);

    if (! aud_cfg->filepopup_showprogressbar || ! aud_drct_get_playing ())
        goto HIDE;

    playlist = aud_playlist_get_playing ();

    if (playlist == -1)
        goto HIDE;

    entry = aud_playlist_get_position (playlist);

    if (entry == -1)
        goto HIDE;

    filename = aud_playlist_entry_get_filename (playlist, entry);

    if (strcmp (filename, tooltip_file))
        goto HIDE;

    time = aud_drct_get_time ();
    gtk_progress_bar_set_fraction ((GtkProgressBar *) progressbar, time /
     (gfloat) length);
    progress_time = g_strdup_printf ("%d:%02d", time / 60000, (time / 1000) % 60);
    gtk_progress_bar_set_text ((GtkProgressBar *) progressbar, progress_time);
    g_free (progress_time);

    gtk_widget_show (progressbar);
    return TRUE;

HIDE:
    /* tooltip opened, but song is not the same, or playback is stopped */
    gtk_widget_hide (progressbar);
    return TRUE;
}

static void infopopup_progress_init (void)
{
    g_object_set_data ((GObject *) infopopup, "progress_sid", GINT_TO_POINTER
     (0));
}

static void infopopup_progress_start (void)
{
    gint sid = g_timeout_add (500, (GSourceFunc) infopopup_progress_cb, NULL);

    g_object_set_data ((GObject *) infopopup, "progress_sid", GINT_TO_POINTER
     (sid));
}

static void infopopup_progress_stop (void)
{
    gint sid = GPOINTER_TO_INT (g_object_get_data ((GObject *) infopopup,
     "progress_sid"));

    if (! sid)
        return;

    g_source_remove (sid);
    g_object_set_data ((GObject *) infopopup, "progress_sid", GINT_TO_POINTER
     (0));
}

static void infopopup_add_category (GtkWidget * infopopup_data_table,
 const gchar * category, const gchar * header_data, const gchar * label_data,
 gint position)
{
    GtkWidget * infopopup_data_info_header = gtk_label_new ("");
    GtkWidget * infopopup_data_info_label = gtk_label_new ("");
    gchar * markup;

    gtk_misc_set_alignment ((GtkMisc *) infopopup_data_info_header, 0, 0.5);
    gtk_misc_set_alignment ((GtkMisc *) infopopup_data_info_label, 0, 0.5);
    gtk_misc_set_padding ((GtkMisc *) infopopup_data_info_header, 0, 3);
    gtk_misc_set_padding ((GtkMisc *) infopopup_data_info_label, 0, 3);

    markup = g_markup_printf_escaped ("<span style=\"italic\">%s</span>",
     category);
    gtk_label_set_markup ((GtkLabel *) infopopup_data_info_header, markup);
    g_free (markup);

    g_object_set_data ((GObject *) infopopup, header_data,
     infopopup_data_info_header);
    g_object_set_data ((GObject *) infopopup, label_data,
     infopopup_data_info_label);
    gtk_table_attach ((GtkTable *) infopopup_data_table,
     infopopup_data_info_header, 0, 1, position, position + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) infopopup_data_table,
     infopopup_data_info_label, 1, 2, position, position + 1, GTK_FILL, 0, 0, 0);
}

static void infopopup_create (void)
{
    GtkWidget * infopopup_hbox;
    GtkWidget * infopopup_data_image;
    GtkWidget * infopopup_data_table;
    GtkWidget * infopopup_progress;

    infopopup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_type_hint ((GtkWindow *) infopopup,
     GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_decorated ((GtkWindow *) infopopup, FALSE);
    gtk_container_set_border_width ((GtkContainer *) infopopup, 6);

    infopopup_hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) infopopup, infopopup_hbox);

    infopopup_data_image = gtk_image_new ();
    gtk_misc_set_alignment ((GtkMisc *) infopopup_data_image, 0.5, 0);
    gtk_image_set_from_file ((GtkImage *) infopopup_data_image, DEFAULT_ARTWORK);

    g_object_set_data ((GObject *) infopopup, "image_artwork",
     infopopup_data_image);
    g_object_set_data ((GObject *) infopopup, "last_artwork", NULL);
    gtk_box_pack_start ((GtkBox *) infopopup_hbox, infopopup_data_image, FALSE,
     FALSE, 0);

    gtk_box_pack_start ((GtkBox *) infopopup_hbox, gtk_vseparator_new (), FALSE,
     FALSE, 6);

    infopopup_data_table = gtk_table_new (8, 2, FALSE);
    gtk_table_set_row_spacings ((GtkTable *) infopopup_data_table, 0);
    gtk_table_set_col_spacings ((GtkTable *) infopopup_data_table, 6);
    gtk_box_pack_start ((GtkBox *) infopopup_hbox, infopopup_data_table, TRUE,
     TRUE, 0);

    infopopup_add_category (infopopup_data_table, _("Title"), "header_title",
     "label_title", 0);
    infopopup_add_category (infopopup_data_table, _("Artist"), "header_artist",
     "label_artist", 1);
    infopopup_add_category (infopopup_data_table, _("Album"), "header_album",
     "label_album", 2);
    infopopup_add_category (infopopup_data_table, _("Genre"), "header_genre",
     "label_genre", 3);
    infopopup_add_category (infopopup_data_table, _("Year"), "header_year",
     "label_year", 4);
    infopopup_add_category (infopopup_data_table, _("Track Number"),
     "header_tracknum", "label_tracknum", 5);
    infopopup_add_category (infopopup_data_table, _("Track Length"),
     "header_tracklen", "label_tracklen", 6);

    gtk_table_set_row_spacing ((GtkTable *) infopopup_data_table, 6, 6);

    /* track progress */
    infopopup_progress = gtk_progress_bar_new ();
    gtk_progress_bar_set_text ((GtkProgressBar *) infopopup_progress, "");
    gtk_table_attach ((GtkTable *) infopopup_data_table, infopopup_progress, 0,
     2, 7, 8, GTK_FILL, 0, 0, 0);
    g_object_set_data ((GObject *) infopopup, "file", NULL);
    g_object_set_data ((GObject *) infopopup, "progressbar", infopopup_progress);
    infopopup_progress_init ();

    /* do not show the track progress */
    gtk_widget_set_no_show_all (infopopup_progress, TRUE);
    gtk_widget_show_all (infopopup_hbox);
}

#if 0
static void infopopup_destroy (void)
{
    infopopup_progress_stop (infopopup);
    g_free (g_object_get_data ((GObject *) infopopup, "last_artwork"));
    gtk_widget_destroy (infopopup);
}
#endif

static void infopopup_update_data (const gchar * text, const gchar * label_data,
 const gchar * header_data)
{
    if (text != NULL)
    {
        infopopup_entry_set_text (label_data, text);
        gtk_widget_show ((GtkWidget *) g_object_get_data ((GObject *) infopopup,
         header_data));
        gtk_widget_show ((GtkWidget *) g_object_get_data ((GObject *) infopopup,
         label_data));
    }
    else
    {
        gtk_widget_hide ((GtkWidget *) g_object_get_data ((GObject *) infopopup,
         header_data));
        gtk_widget_hide ((GtkWidget *) g_object_get_data ((GObject *) infopopup,
         label_data));
    }
}

static void infopopup_clear (void)
{
    infopopup_progress_stop ();

    infopopup_entry_set_text ("label_title", "");
    infopopup_entry_set_text ("label_artist", "");
    infopopup_entry_set_text ("label_album", "");
    infopopup_entry_set_text ("label_genre", "");
    infopopup_entry_set_text ("label_tracknum", "");
    infopopup_entry_set_text ("label_year", "");
    infopopup_entry_set_text ("label_tracklen", "");

    gtk_window_resize ((GtkWindow *) infopopup, 1, 1);
}

static void infopopup_show (const gchar * filename, const Tuple * tuple,
 const gchar * title)
{
    gint x, y, h, w;
    gchar * last_artwork;
    gint length, value;
    gchar * tmp;
    const gchar * title2;

    if (infopopup == NULL)
        infopopup_create ();
    else
        infopopup_clear ();

    g_free (g_object_get_data ((GObject *) infopopup, "file"));
    g_object_set_data ((GObject *) infopopup, "file", g_strdup (filename));

    /* use title from tuple if possible */
    if ((title2 = tuple_get_string (tuple, FIELD_TITLE, NULL)))
        title = title2;

    infopopup_update_data (title, "label_title", "header_title");
    infopopup_update_data (tuple_get_string (tuple, FIELD_ARTIST, NULL),
     "label_artist", "header_artist");
    infopopup_update_data (tuple_get_string (tuple, FIELD_ALBUM, NULL),
     "label_album", "header_album");
    infopopup_update_data (tuple_get_string (tuple, FIELD_GENRE, NULL),
     "label_genre", "header_genre");

    length = tuple_get_int (tuple, FIELD_LENGTH, NULL);
    tmp = (length > 0) ? g_strdup_printf ("%d:%02d", length / 60000, length /
     1000 % 60) : NULL;
    infopopup_update_data (tmp, "label_tracklen", "header_tracklen");
    g_free (tmp);

    g_object_set_data ((GObject *) infopopup, "length" , GINT_TO_POINTER (length));

    value = tuple_get_int (tuple, FIELD_YEAR, NULL);
    tmp = (value > 0) ? g_strdup_printf ("%d", value) : NULL;
    infopopup_update_data (tmp, "label_year", "header_year");
    g_free (tmp);

    value = tuple_get_int (tuple, FIELD_TRACK_NUMBER, NULL);
    tmp = (value > 0) ? g_strdup_printf ("%d", value) : NULL;
    infopopup_update_data (tmp, "label_tracknum", "header_tracknum");
    g_free (tmp);

    last_artwork = g_object_get_data ((GObject *) infopopup, "last_artwork");
    tmp = aud_get_associated_image_file (filename);

    if (tmp == NULL)
        tmp = g_strdup (DEFAULT_ARTWORK);

    if (last_artwork == NULL || strcmp (tmp, last_artwork))
    {
        infopopup_entry_set_image ("image_artwork", tmp);
        g_free (last_artwork);
        g_object_set_data ((GObject *) infopopup, "last_artwork", tmp);
    }
    else
        g_free (tmp);

    /* start a timer that updates a progress bar if the tooltip
       is shown for the song that is being currently played */
    if (length > 0)
    {
        infopopup_progress_start ();
        /* immediately run the callback once to update progressbar status */
        infopopup_progress_cb (NULL);
    }

    gdk_window_get_pointer (gdk_get_default_root_window (), & x, & y, NULL);
    gtk_window_get_size ((GtkWindow *) infopopup, & w, & h);

    /* If we show the popup right under the cursor, the underlying window gets
     * a "leave-notify-event" and immediately hides the popup again.  So, we
     * offset the popup slightly. */
    if (x + w > gdk_screen_width ())
        x -= w + 3;
    else
        x += 3;

    if (y + h > gdk_screen_height ())
        y -= h + 3;
    else
        y += 3;

    gtk_window_move ((GtkWindow *) infopopup, x, y);
    gtk_widget_show (infopopup);
}

void audgui_infopopup_show (gint playlist, gint entry)
{
    const gchar * filename = aud_playlist_entry_get_filename (playlist, entry);
    const Tuple * tuple = aud_playlist_entry_get_tuple (playlist, entry, FALSE);

    g_return_if_fail (filename != NULL);

    if (tuple == NULL) /* FIXME: show an error popup if this happens */
        return;

    infopopup_show (filename, tuple, aud_playlist_entry_get_title (playlist,
     entry, FALSE));
}

void audgui_infopopup_show_current (void)
{
    gint playlist = aud_playlist_get_playing ();
    gint position;

    if (playlist == -1)
        playlist = aud_playlist_get_active ();

    position = aud_playlist_get_position (playlist);

    if (position == -1)
        return;

    audgui_infopopup_show (playlist, position);
}

void audgui_infopopup_hide (void)
{
    infopopup_progress_stop ();
    gtk_widget_hide (infopopup);
}
