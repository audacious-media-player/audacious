/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "compatibility.h"

#include "main.h"
#include "playback.h"
#include "playlist-new.h"
#include "audstrings.h"
#include "ui_fileinfopopup.h"
#include "ui_fileinfo.h"

static void
filepopup_entry_set_text(GtkWidget *filepopup_win, const gchar *entry_name,
                         const gchar *text)
{
    GtkWidget *widget = g_object_get_data(G_OBJECT(filepopup_win), entry_name);
    g_return_if_fail(widget != NULL);

    gtk_label_set_text(GTK_LABEL(widget), text);
}

static void
filepopup_entry_set_image(GtkWidget *filepopup_win, const gchar *entry_name,
                          const gchar *text)
{
    GtkWidget *widget = g_object_get_data(G_OBJECT(filepopup_win), entry_name);
    GdkPixbuf *pixbuf, *pixbuf2;
    int width, height;
    double aspect;

    g_return_if_fail(widget != NULL);

    pixbuf = gdk_pixbuf_new_from_file(text, NULL);
    g_return_if_fail(pixbuf != NULL);

    width  = gdk_pixbuf_get_width(GDK_PIXBUF(pixbuf));
    height = gdk_pixbuf_get_height(GDK_PIXBUF(pixbuf));

    if (strcmp(DATA_DIR "/images/audio.png", text))
    {
        if (width == 0)
            width = 1;

        aspect = (double)height / (double)width;
        if (aspect > 1.0) {
            height = (int)(cfg.filepopup_pixelsize * aspect);
            width = cfg.filepopup_pixelsize;
        } else {
            height = cfg.filepopup_pixelsize;
            width = (int)(cfg.filepopup_pixelsize / aspect);
        }

        pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height,
                                          GDK_INTERP_BILINEAR);
        g_object_unref(G_OBJECT(pixbuf));
        pixbuf = pixbuf2;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
    g_object_unref(G_OBJECT(pixbuf));
}

static gboolean
fileinfopopup_progress_cb(gpointer filepopup_win)
{
    GtkWidget *progressbar =
        g_object_get_data(G_OBJECT(filepopup_win), "progressbar");
    gchar *tooltip_file = g_object_get_data(G_OBJECT(filepopup_win), "file");
    gchar * current_file;
    gint length =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filepopup_win), "length"));
    gint playlist, entry, time;
    const gchar * filename;

    g_return_val_if_fail(progressbar != NULL, FALSE);

    playlist = playlist_get_active ();
    entry = playlist_get_position (playlist);
    filename = playlist_entry_get_filename (playlist, entry);

    if (filename == NULL)
        return FALSE;

    current_file = g_filename_from_uri (filename, NULL, NULL);

    if (playback_get_playing() && length != -1 &&
        current_file != NULL && tooltip_file != NULL &&
        !strcmp(tooltip_file, current_file) && cfg.filepopup_showprogressbar)
    {
        time = playback_get_time();
        gchar *progress_time =
            g_strdup_printf("%d:%02d", time / 60000, (time / 1000) % 60);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar),
                                      (gdouble)time / (gdouble)length);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), progress_time);

        if (!GTK_WIDGET_VISIBLE(progressbar))
            gtk_widget_show(progressbar);

        g_free(progress_time);
    }
    else
    {
        /* tooltip opened, but song is not the same,
         * or playback is stopped, or length is not applicabile */
        if (GTK_WIDGET_VISIBLE(progressbar))
            gtk_widget_hide(progressbar);
    }

    g_free( current_file );
    return TRUE;
}

static gboolean
fileinfopopup_progress_check_active(GtkWidget *filepopup_win)
{
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filepopup_win),"progress_sid")) == 0)
        return FALSE;
    return TRUE;
}

static void
fileinfopopup_progress_init(GtkWidget *filepopup_win)
{
    g_object_set_data( G_OBJECT(filepopup_win) , "progress_sid" , GINT_TO_POINTER(0) );
}

static void
fileinfopopup_progress_start(GtkWidget *filepopup_win)
{
    gint sid =
        g_timeout_add(500, (GSourceFunc)fileinfopopup_progress_cb,
                      filepopup_win);
    g_object_set_data(G_OBJECT(filepopup_win), "progress_sid",
                      GINT_TO_POINTER(sid));
}

static void
fileinfopopup_progress_stop(GtkWidget *filepopup_win)
{
    gint sid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filepopup_win),
                                                 "progress_sid"));
    if (sid != 0)
    {
        g_source_remove(sid);
        g_object_set_data(G_OBJECT(filepopup_win),"progress_sid",GINT_TO_POINTER(0));
    }
}

static void
fileinfopopup_add_category(GtkWidget *filepopup_win,
                                     GtkWidget *filepopup_data_table,
                                     const gchar *category,
                                     const gchar *header_data,
                                     const gchar *label_data,
                                     const gint position)
{
    gchar *markup;

    GtkWidget *filepopup_data_info_header = gtk_label_new("");
    GtkWidget *filepopup_data_info_label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(filepopup_data_info_header), 0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(filepopup_data_info_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(filepopup_data_info_header), 0, 3);
    gtk_misc_set_padding(GTK_MISC(filepopup_data_info_label), 0, 3);

    markup =
        g_markup_printf_escaped("<span style=\"italic\">%s</span>", category);

    gtk_label_set_markup(GTK_LABEL(filepopup_data_info_header), markup);
    g_free(markup);

    g_object_set_data(G_OBJECT(filepopup_win), header_data,
                      filepopup_data_info_header);
    g_object_set_data(G_OBJECT(filepopup_win), label_data,
                      filepopup_data_info_label);
    gtk_table_attach(GTK_TABLE(filepopup_data_table),
                     filepopup_data_info_header,
                     0, 1, position, position + 1, GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(filepopup_data_table),
                     filepopup_data_info_label,
                     1, 2, position, position + 1, GTK_FILL, 0, 0, 0);
}



GtkWidget *
fileinfopopup_create(void)
{
    GtkWidget *filepopup_win;
    GtkWidget *filepopup_hbox;
    GtkWidget *filepopup_data_image;
    GtkWidget *filepopup_data_table;
    GtkWidget *filepopup_progress;

    filepopup_win = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_type_hint(GTK_WINDOW(filepopup_win),
		    		 GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_decorated(GTK_WINDOW(filepopup_win), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(filepopup_win), 6);

    filepopup_hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(filepopup_win), filepopup_hbox);

    filepopup_data_image = gtk_image_new();
    gtk_misc_set_alignment(GTK_MISC(filepopup_data_image), 0.5, 0);
    gtk_image_set_from_file(GTK_IMAGE(filepopup_data_image),
                            DATA_DIR "/images/audio.png");

    g_object_set_data(G_OBJECT(filepopup_win), "image_artwork",
                      filepopup_data_image);
    g_object_set_data(G_OBJECT(filepopup_win), "last_artwork", NULL);
    gtk_box_pack_start(GTK_BOX(filepopup_hbox), filepopup_data_image,
                       FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(filepopup_hbox), gtk_vseparator_new(),
                       FALSE, FALSE, 6);

    filepopup_data_table = gtk_table_new(8, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(filepopup_data_table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(filepopup_data_table), 6);
    gtk_box_pack_start(GTK_BOX(filepopup_hbox), filepopup_data_table,
                       TRUE, TRUE, 0);

    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Title"),
                                         "header_title", "label_title", 0);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Artist"),
                                         "header_artist", "label_artist", 1);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Album"),
                                         "header_album", "label_album", 2);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Genre"),
                                         "header_genre", "label_genre", 3);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Year"),
                                         "header_year", "label_year", 4);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Track Number"),
                                         "header_tracknum", "label_tracknum",
                                         5);
    fileinfopopup_add_category(filepopup_win, filepopup_data_table,
                                         _("Track Length"),
                                         "header_tracklen", "label_tracklen",
                                         6);

    gtk_table_set_row_spacing(GTK_TABLE(filepopup_data_table), 6, 6);

    /* track progress */
    filepopup_progress = gtk_progress_bar_new();
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(filepopup_progress),
                                     GTK_PROGRESS_LEFT_TO_RIGHT);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(filepopup_progress), "");
    gtk_table_attach(GTK_TABLE(filepopup_data_table), filepopup_progress,
                     0, 2, 7, 8, GTK_FILL, 0, 0, 0);
    g_object_set_data(G_OBJECT(filepopup_win), "file", NULL);
    g_object_set_data(G_OBJECT(filepopup_win), "progressbar",
                      filepopup_progress);
    fileinfopopup_progress_init(filepopup_win);

    /* this will realize all widgets contained in filepopup_hbox */
    gtk_widget_show_all(filepopup_hbox);

    /* do not show the track progress */
    gtk_widget_hide(filepopup_progress);

    return filepopup_win;
}

void
fileinfopopup_destroy(GtkWidget *filepopup_win)
{
    gchar *last_artwork;
    fileinfopopup_progress_stop(filepopup_win);

    last_artwork =
        g_object_get_data(G_OBJECT(filepopup_win), "last_artwork");
    if (last_artwork != NULL)
        g_free(last_artwork);

    gtk_widget_destroy(filepopup_win);
}

static void
fileinfopupup_update_data(GtkWidget *filepopup_win,
                                    const gchar *text,
                                    const gchar *label_data,
                                    const gchar *header_data)
{
    if (text != NULL)
    {
        filepopup_entry_set_text(filepopup_win, label_data, text);
        gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win), header_data)));
        gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win), label_data)));
    }
    else
    {
        gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win), header_data)));
        gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win), label_data)));
    }
}

void
fileinfopopup_show_from_tuple(GtkWidget *filepopup_win,
                                        Tuple *tuple)
{
    gchar *tmp = NULL;
    gint x, y, x_off = 3, y_off = 3, h, w;
    gchar *length_string, *year_string, *track_string;
    gchar *last_artwork;
    const static gchar default_artwork[] = DATA_DIR "/images/audio.png";
    gint length;
    const gchar * path, * name;

    last_artwork =
        g_object_get_data(G_OBJECT(filepopup_win), "last_artwork");

    g_return_if_fail(tuple != NULL);

    tmp = g_object_get_data(G_OBJECT(filepopup_win), "file");
    if (tmp != NULL) {
        g_free(tmp);
        tmp = NULL;
        g_object_set_data(G_OBJECT(filepopup_win), "file", NULL);
    }

    path = tuple_get_string (tuple, FIELD_FILE_PATH, NULL);

    if (!path || strncmp (path, "file://", 7)) /* remote files not handled */
        return;

    path += 7;
    name = tuple_get_string (tuple, FIELD_FILE_NAME, NULL);

    g_object_set_data ((GObject *) filepopup_win, "file", g_build_filename
     (path, name, NULL));

    gtk_widget_realize(filepopup_win);

    if (tuple_get_string(tuple, FIELD_TITLE, NULL))
    {
        gchar *markup =
            g_markup_printf_escaped("<span style=\"italic\">%s</span>", _("Title"));
        gtk_label_set_markup(GTK_LABEL(g_object_get_data(G_OBJECT(filepopup_win), "header_title")), markup);
        g_free(markup);
        filepopup_entry_set_text(filepopup_win, "label_title", tuple_get_string(tuple, FIELD_TITLE, NULL));
    }
    else
    {
        /* display filename if track_name is not available */
        gchar *markup =
            g_markup_printf_escaped("<span style=\"italic\">%s</span>", _("Filename"));
        gtk_label_set_markup(GTK_LABEL(g_object_get_data(G_OBJECT(filepopup_win), "header_title")), markup);
        g_free(markup);
        filepopup_entry_set_text (filepopup_win, "label_title", name);
    }

    fileinfopupup_update_data(filepopup_win, tuple_get_string(tuple, FIELD_ARTIST, NULL),
                                        "label_artist", "header_artist");
    fileinfopupup_update_data(filepopup_win, tuple_get_string(tuple, FIELD_ALBUM, NULL),
                                        "label_album", "header_album");
    fileinfopupup_update_data(filepopup_win, tuple_get_string(tuple, FIELD_GENRE, NULL),
                                        "label_genre", "header_genre");

    length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
    length_string = (length > 0) ?
        g_strdup_printf("%d:%02d", length / 60000, (length / 1000) % 60) : NULL;
    fileinfopupup_update_data(filepopup_win, length_string,
                                        "label_tracklen", "header_tracklen");
    g_free(length_string);

    if ( length > 0 )
      g_object_set_data( G_OBJECT(filepopup_win), "length" , GINT_TO_POINTER(length) );
    else
      g_object_set_data( G_OBJECT(filepopup_win), "length" , GINT_TO_POINTER(-1) );

    year_string = (tuple_get_int(tuple, FIELD_YEAR, NULL) == 0) ? NULL : g_strdup_printf("%d", tuple_get_int(tuple, FIELD_YEAR, NULL));
    fileinfopupup_update_data(filepopup_win, year_string,
                                        "label_year", "header_year");
    g_free(year_string);

    track_string = (tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL) == 0) ? NULL : g_strdup_printf("%d", tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL));
    fileinfopupup_update_data(filepopup_win, track_string,
                                        "label_tracknum", "header_tracknum");
    g_free(track_string);

    tmp = fileinfo_recursive_get_image (path, name, 0);

    /* { */
        if (tmp) { // picture found
            if (!last_artwork || strcmp(last_artwork, tmp)) { // new picture
                filepopup_entry_set_image(filepopup_win, "image_artwork", tmp);
                if (last_artwork) g_free(last_artwork);
                last_artwork = tmp;
                g_object_set_data(G_OBJECT(filepopup_win), "last_artwork", last_artwork);
            }
            else { // same picture
            }
        }
        else { // no picture found
            if (!last_artwork || strcmp(last_artwork, default_artwork)) {
                filepopup_entry_set_image(filepopup_win, "image_artwork", default_artwork);
                if (last_artwork) g_free(last_artwork);
                last_artwork = g_strdup(default_artwork);
                g_object_set_data(G_OBJECT(filepopup_win), "last_artwork", last_artwork);
            }
            else {
            }
        }
    /* } */

    /* start a timer that updates a progress bar if the tooltip
       is shown for the song that is being currently played */
    if (fileinfopopup_progress_check_active(filepopup_win) == FALSE)
    {
        fileinfopopup_progress_start(filepopup_win);
        /* immediately run the callback once to update progressbar status */
        fileinfopopup_progress_cb(filepopup_win);
    }

    gdk_window_get_pointer(gdk_get_default_root_window(), &x, &y, NULL);
    gtk_window_get_size(GTK_WINDOW(filepopup_win), &w, &h);
    if (gdk_screen_width()-(w+3) < x) x_off = (w*-1)-3;
    if (gdk_screen_height()-(h+3) < y) y_off = (h*-1)-3;
    gtk_window_move(GTK_WINDOW(filepopup_win), x + x_off, y + y_off);

    gtk_widget_show(filepopup_win);
}

void
fileinfopopup_show_from_title(GtkWidget *filepopup_win, gchar *title)
{
    Tuple * tuple = tuple_new();
    tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    fileinfopopup_show_from_tuple(filepopup_win, tuple);
    mowgli_object_unref(tuple);
    return;
}

void
fileinfopopup_hide(GtkWidget *filepopup_win, gpointer unused)
{
    if (GTK_WIDGET_VISIBLE(filepopup_win))
    {
        fileinfopopup_progress_stop(filepopup_win);

        gtk_widget_hide(filepopup_win);

        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_title", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_artist", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_album", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_genre", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_tracknum", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_year", "");
        filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_tracklen", "");

        gtk_window_resize(GTK_WINDOW(filepopup_win), 1, 1);
    }
}
