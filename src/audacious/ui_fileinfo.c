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
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "strings.h"
#include "general.h"
#include "output.h"
#include "visualization.h"

#include "main.h"
#include "util.h"
#include "dnd.h"
#include "tuple.h"

#include "playlist.h"

#include "ui_main.h"
#include "ui_playlist.h"
#include "build_stamp.h"
#include "ui_fileinfo.h"
#include "ui_playlist.h"

GtkWidget *fileinfo_win;

GtkWidget *entry_location;
GtkWidget *entry_title;
GtkWidget *entry_artist;
GtkWidget *entry_album;
GtkWidget *entry_comment;
GtkWidget *entry_year;
GtkWidget *entry_track;
GtkWidget *entry_genre;

GtkWidget *image_artwork;

static void
fileinfo_entry_set_text(GtkWidget *widget, const char *text)
{
    if (widget == NULL)
        return;

    gtk_entry_set_text(GTK_ENTRY(widget), text);
}

static void
fileinfo_entry_set_text_free(GtkWidget *widget, char *text)
{
    if (widget == NULL)
        return;

    gtk_entry_set_text(GTK_ENTRY(widget), text);

    g_free(text);
}

static void
fileinfo_entry_set_image(GtkWidget *widget, const char *text)
{
	GdkPixbuf *pixbuf;
	int width, height;
	double aspect;
	GdkPixbuf *pixbuf2;

	if (widget == NULL)
		return;

	pixbuf = gdk_pixbuf_new_from_file(text, NULL);

	if (pixbuf == NULL)
		return;

	width  = gdk_pixbuf_get_width(GDK_PIXBUF(pixbuf));
	height = gdk_pixbuf_get_height(GDK_PIXBUF(pixbuf));

	if(strcmp(DATA_DIR "/images/audio.png", text))
	{
		if(width == 0)
			width = 1;
		aspect = (double)height / (double)width;
		if(aspect > 1.0) {
			height = (int)(cfg.filepopup_pixelsize * aspect);
			width = cfg.filepopup_pixelsize;
		} else {
			height = cfg.filepopup_pixelsize;
			width = (int)(cfg.filepopup_pixelsize / aspect);
		}
		pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height, GDK_INTERP_BILINEAR);
		g_object_unref(G_OBJECT(pixbuf));
		pixbuf = pixbuf2;
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
	g_object_unref(G_OBJECT(pixbuf));
}

void fileinfo_hide(gpointer unused)
{
    gtk_widget_hide(fileinfo_win);

    /* Clear it out. */
    fileinfo_entry_set_text(entry_title, "");
    fileinfo_entry_set_text(entry_artist, "");
    fileinfo_entry_set_text(entry_album, "");
    fileinfo_entry_set_text(entry_comment, "");
    fileinfo_entry_set_text(entry_genre, "");
    fileinfo_entry_set_text(entry_year, "");
    fileinfo_entry_set_text(entry_track, "");
    fileinfo_entry_set_text(entry_location, "");

    fileinfo_entry_set_image(image_artwork, DATA_DIR "/images/audio.png");
}

void
create_fileinfo_window(void)
{
    GtkWidget *hbox;
    GtkWidget *vbox1;
    GtkWidget *vbox2;
    GtkWidget *label_title;
    GtkWidget *label_artist;
    GtkWidget *label_album;
    GtkWidget *label_comment;
    GtkWidget *label_genre;
    GtkWidget *label_year;
    GtkWidget *label_track;
    GtkWidget *label_location;
    GtkWidget *table1;
    GtkWidget *bbox_close;
    GtkWidget *btn_close;
    GtkWidget *alignment;

    fileinfo_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(fileinfo_win), 6);
    gtk_window_set_title(GTK_WINDOW(fileinfo_win), _("Track Information"));
    gtk_window_set_position(GTK_WINDOW(fileinfo_win), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(fileinfo_win), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(fileinfo_win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_transient_for(GTK_WINDOW(fileinfo_win), GTK_WINDOW(mainwin));

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(fileinfo_win), hbox);

    image_artwork = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox), image_artwork, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(image_artwork), 0.5, 0);
    gtk_image_set_from_file(GTK_IMAGE(image_artwork), DATA_DIR "/images/audio.png");

    vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 0);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox1), alignment, TRUE, TRUE, 0);
    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox2);

    label_title = gtk_label_new(_("<span size=\"small\">Title</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_title, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_title), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_title), 0, 0);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_title = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_title);

    label_artist = gtk_label_new(_("<span size=\"small\">Artist</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_artist, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_artist), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_artist), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_artist = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_artist);

    label_album = gtk_label_new(_("<span size=\"small\">Album</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_album, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_album), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_album), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_album = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_album);

    label_comment = gtk_label_new(_("<span size=\"small\">Comment</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_comment, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_comment), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_comment), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_comment = gtk_entry_new();
    gtk_container_add (GTK_CONTAINER(alignment), entry_comment);

    label_genre = gtk_label_new(_("<span size=\"small\">Genre</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_genre, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_genre), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_genre), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_genre = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_genre);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    table1 = gtk_table_new(2, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(alignment), table1);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 6);

    label_year = gtk_label_new(_("<span size=\"small\">Year</span>"));
    gtk_table_attach(GTK_TABLE(table1), label_year, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_year), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_year), 0, 0.5);

    entry_year = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table1), entry_year, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);

    label_track = gtk_label_new(_("<span size=\"small\">Track Number</span>"));
    gtk_table_attach(GTK_TABLE(table1), label_track, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_track), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_track), 0, 0.5);

    entry_track = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table1), entry_track, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);

    label_location = gtk_label_new(_("<span size=\"small\">Location</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_location, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_location), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_location), 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 6, 0, 0);

    entry_location = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_location);

    bbox_close = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox1), bbox_close, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox_close), GTK_BUTTONBOX_END);

    btn_close = gtk_button_new_from_stock("gtk-close");
    gtk_container_add(GTK_CONTAINER(bbox_close), btn_close);
    GTK_WIDGET_SET_FLAGS(btn_close, GTK_CAN_DEFAULT);
    g_signal_connect(G_OBJECT(btn_close), "clicked", (GCallback) fileinfo_hide, NULL);

    gtk_widget_show_all (hbox);
}

void
fileinfo_show_for_tuple(Tuple *tuple)
{
	gchar *tmp = NULL;

	if (tuple == NULL)
		return;

	gtk_widget_realize(fileinfo_win);

	fileinfo_entry_set_text(entry_title, tuple_get_string(tuple, FIELD_TITLE, NULL));
	fileinfo_entry_set_text(entry_artist, tuple_get_string(tuple, FIELD_ARTIST, NULL));
	fileinfo_entry_set_text(entry_album, tuple_get_string(tuple, FIELD_ALBUM, NULL));
	fileinfo_entry_set_text(entry_comment, tuple_get_string(tuple, FIELD_COMMENT, NULL));
	fileinfo_entry_set_text(entry_genre, tuple_get_string(tuple, FIELD_GENRE, NULL));

	tmp = g_strdup_printf("%s/%s",
		tuple_get_string(tuple, FIELD_FILE_PATH, NULL),
		tuple_get_string(tuple, FIELD_FILE_NAME, NULL));
	if(tmp){
		fileinfo_entry_set_text_free(entry_location, str_to_utf8(tmp));
		g_free(tmp);
		tmp = NULL;
	}

	if (tuple_get_int(tuple, FIELD_YEAR, NULL))
		fileinfo_entry_set_text_free(entry_year,
			g_strdup_printf("%d", tuple_get_int(tuple, FIELD_YEAR, NULL)));

	if (tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL))
		fileinfo_entry_set_text_free(entry_track,
			g_strdup_printf("%d", tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL)));

	tmp = fileinfo_recursive_get_image(
		tuple_get_string(tuple, FIELD_FILE_PATH, NULL),
		tuple_get_string(tuple, FIELD_FILE_NAME, NULL), 0);
	
	if(tmp)
	{
		fileinfo_entry_set_image(image_artwork, tmp);
		g_free(tmp);
	}
	
	gtk_widget_show(fileinfo_win);
}

void
fileinfo_show_for_path(gchar *path)
{
	Tuple *tuple = input_get_song_tuple(path);

	if (tuple == NULL)
		return input_file_info_box(path);

	fileinfo_show_for_tuple(tuple);

	mowgli_object_unref(tuple);
}
