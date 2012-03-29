/*
 * infopopup.c
 * Copyright 2006-2011 William Pitcock, Giacomo Lozito, and John Lindgren
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

#include <gtk/gtk.h>
#include <string.h>

#include <audacious/drct.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GtkWidget * infopopup = NULL;

static void infopopup_entry_set_text (const char * entry_name, const char *
 text)
{
    GtkWidget * widget = g_object_get_data ((GObject *) infopopup, entry_name);

    g_return_if_fail (widget != NULL);
    gtk_label_set_text ((GtkLabel *) widget, text);
}

static void infopopup_entry_set_image (int playlist, int entry)
{
    GtkWidget * widget = g_object_get_data ((GObject *) infopopup, "image");
    g_return_if_fail (widget);

    GdkPixbuf * pixbuf = audgui_pixbuf_for_entry (playlist, entry);

    if (pixbuf)
    {
        audgui_pixbuf_scale_within (& pixbuf, 96);
        gtk_image_set_from_pixbuf ((GtkImage *) widget, pixbuf);
        g_object_unref (pixbuf);
    }
    else
        gtk_image_clear ((GtkImage *) widget);
}

static bool_t infopopup_progress_cb (void * unused)
{
    GtkWidget * progressbar = g_object_get_data ((GObject *) infopopup,
     "progressbar");
    char * tooltip_file = g_object_get_data ((GObject *) infopopup, "file");
    int length = GPOINTER_TO_INT (g_object_get_data ((GObject *) infopopup,
     "length"));
    int playlist, entry, time;
    char * progress_time;

    g_return_val_if_fail (tooltip_file != NULL, FALSE);
    g_return_val_if_fail (length > 0, FALSE);

    if (! aud_get_bool (NULL, "filepopup_showprogressbar") || ! aud_drct_get_playing ())
        goto HIDE;

    playlist = aud_playlist_get_playing ();

    if (playlist == -1)
        goto HIDE;

    entry = aud_playlist_get_position (playlist);

    if (entry == -1)
        goto HIDE;

    char * filename = aud_playlist_entry_get_filename (playlist, entry);
    if (strcmp (filename, tooltip_file))
    {
        str_unref (filename);
        goto HIDE;
    }
    str_unref (filename);

    time = aud_drct_get_time ();
    gtk_progress_bar_set_fraction ((GtkProgressBar *) progressbar, time /
     (float) length);
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
    int sid = g_timeout_add (500, (GSourceFunc) infopopup_progress_cb, NULL);

    g_object_set_data ((GObject *) infopopup, "progress_sid", GINT_TO_POINTER
     (sid));
}

static void infopopup_progress_stop (void)
{
    int sid = GPOINTER_TO_INT (g_object_get_data ((GObject *) infopopup,
     "progress_sid"));

    if (! sid)
        return;

    g_source_remove (sid);
    g_object_set_data ((GObject *) infopopup, "progress_sid", GINT_TO_POINTER
     (0));
}

static void infopopup_add_category (GtkWidget * infopopup_data_table,
 const char * category, const char * header_data, const char * label_data,
 int position)
{
    GtkWidget * infopopup_data_info_header = gtk_label_new ("");
    GtkWidget * infopopup_data_info_label = gtk_label_new ("");
    char * markup;

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
    g_object_set_data ((GObject *) infopopup, "image", infopopup_data_image);
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

/* calls str_unref() on <text> */
static void infopopup_update_data (char * text, const char * label_data,
 const char * header_data)
{
    if (text)
    {
        infopopup_entry_set_text (label_data, text);
        str_unref (text);

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

static void infopopup_show (int playlist, int entry, const char * filename,
 const Tuple * tuple, const char * title)
{
    int x, y, h, w;
    int length, value;
    char * tmp;

    if (infopopup == NULL)
        infopopup_create ();
    else
        infopopup_clear ();

    g_free (g_object_get_data ((GObject *) infopopup, "file"));
    g_object_set_data ((GObject *) infopopup, "file", g_strdup (filename));

    /* use title from tuple if possible */
    char * title2 = tuple_get_str (tuple, FIELD_TITLE, NULL);
    if (! title2)
        title2 = str_get (title);

    infopopup_update_data (title2, "label_title", "header_title");
    infopopup_update_data (tuple_get_str (tuple, FIELD_ARTIST, NULL), "label_artist", "header_artist");
    infopopup_update_data (tuple_get_str (tuple, FIELD_ALBUM, NULL), "label_album", "header_album");
    infopopup_update_data (tuple_get_str (tuple, FIELD_GENRE, NULL), "label_genre", "header_genre");

    length = tuple_get_int (tuple, FIELD_LENGTH, NULL);
    tmp = (length > 0) ? str_printf ("%d:%02d", length / 60000, length / 1000 % 60) : NULL;
    infopopup_update_data (tmp, "label_tracklen", "header_tracklen");

    g_object_set_data ((GObject *) infopopup, "length" , GINT_TO_POINTER (length));

    value = tuple_get_int (tuple, FIELD_YEAR, NULL);
    tmp = (value > 0) ? str_printf ("%d", value) : NULL;
    infopopup_update_data (tmp, "label_year", "header_year");

    value = tuple_get_int (tuple, FIELD_TRACK_NUMBER, NULL);
    tmp = (value > 0) ? str_printf ("%d", value) : NULL;
    infopopup_update_data (tmp, "label_tracknum", "header_tracknum");

    infopopup_entry_set_image (playlist, entry);

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

EXPORT void audgui_infopopup_show (int playlist, int entry)
{
    char * filename = aud_playlist_entry_get_filename (playlist, entry);
    char * title = aud_playlist_entry_get_title (playlist, entry, FALSE);
    Tuple * tuple = aud_playlist_entry_get_tuple (playlist, entry, FALSE);

    if (filename && title && tuple)
        infopopup_show (playlist, entry, filename, tuple, title);

    str_unref (filename);
    str_unref (title);
    if (tuple)
        tuple_unref (tuple);
}

EXPORT void audgui_infopopup_show_current (void)
{
    int playlist = aud_playlist_get_playing ();
    int position;

    if (playlist == -1)
        playlist = aud_playlist_get_active ();

    position = aud_playlist_get_position (playlist);

    if (position == -1)
        return;

    audgui_infopopup_show (playlist, position);
}

EXPORT void audgui_infopopup_hide (void)
{
    if (! infopopup)
        return;

    infopopup_progress_stop ();
    gtk_widget_hide (infopopup);
}
