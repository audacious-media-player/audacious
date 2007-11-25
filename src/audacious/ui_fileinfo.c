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
#include <stdarg.h>
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

GtkWidget *image_fileicon;
GtkWidget *label_codec_name;
GtkWidget *label_quality;

static void
fileinfo_entry_set_text(GtkWidget *widget, const char *text)
{
    if (widget == NULL)
        return;

    gtk_entry_set_text(GTK_ENTRY(widget), text);
}

static void
fileinfo_label_set_text(GtkWidget *widget, const char *text)
{
    gchar *tmp;

    if (widget == NULL)
        return;

    if (text) {
        tmp = g_strdup_printf("<span size=\"small\">%s</span>", text);
        gtk_label_set_text(GTK_LABEL(widget), tmp);
        gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
        g_free(tmp);
    } else {
        gtk_label_set_text(GTK_LABEL(widget), _("<span size=\"small\">n/a</span>"));
        gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
    }
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

GdkPixbuf *
themed_icon_lookup(gint size, const gchar *name, ...) /* NULL-terminated list of icon names */
{
    GtkIconTheme *icon_theme;
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    gchar *n;
    va_list par;

    icon_theme = gtk_icon_theme_get_default ();
    //fprintf(stderr, "looking for %s\n", name);
    pixbuf = gtk_icon_theme_load_icon (icon_theme, name, size, 0, &error);
    if(pixbuf) return pixbuf;
    
    if(error != NULL) g_error_free(error);

    /* fallback */
    va_start(par, name);
    while((n = (gchar*)va_arg(par, gchar *)) != NULL) {
        //fprintf(stderr, "looking for %s\n", n);
        error = NULL;
        pixbuf = gtk_icon_theme_load_icon (icon_theme, n, size, 0, &error);
        if(pixbuf) {
            //fprintf(stderr, "%s is ok\n", n);
            va_end(par);
            return pixbuf;
        }
        if(error != NULL) g_error_free(error);
    }
    
    return NULL;
}

GdkPixbuf *
mime_icon_lookup(gint size, const gchar *mime_type) /* smart icon resolving routine :) */
{
    gchar *mime_as_is;         /* audio-x-mp3            */
    gchar *mime_gnome;         /* gnome-mime-audio-x-mp3 */
    gchar *mime_generic;       /* audio-x-generic */
    gchar *mime_gnome_generic; /* gnome-mime-audio */

    GdkPixbuf *icon = NULL;

    gchar **s = g_strsplit(mime_type, "/", 2);
    if(s[1] != NULL) {
        mime_as_is         = g_strdup_printf("%s-%s", s[0], s[1]);
        mime_gnome         = g_strdup_printf("gnome-mime-%s-%s", s[0], s[1]);
        mime_generic       = g_strdup_printf("%s-x-generic", s[0]);
        mime_gnome_generic = g_strdup_printf("gnome-mime-%s", s[0]);
        //fprintf(stderr, "will look for %s, %s, %s, %s, %s\n", mime_as_is, mime_gnome, mime_generic, mime_gnome_generic, s[0]);
        icon = themed_icon_lookup(size, mime_as_is, mime_gnome, mime_generic, mime_gnome_generic, s[0], NULL); /* s[0] is category */
        g_free(mime_gnome_generic);
        g_free(mime_generic);
        g_free(mime_gnome);
        g_free(mime_as_is);
    }
    g_strfreev(s);

    return icon;
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
    GtkWidget *label_codec;
    GtkWidget *label_codec_label;
    GtkWidget *label_quality_label;
    GtkWidget *codec_hbox;
    GtkWidget *codec_table;
    GtkWidget *table1;
    GtkWidget *bbox_close;
    GtkWidget *btn_close;
    GtkWidget *alignment;
    GtkWidget *separator;

    fileinfo_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(fileinfo_win), 6);
    gtk_window_set_title(GTK_WINDOW(fileinfo_win), _("Track Information"));
    gtk_window_set_position(GTK_WINDOW(fileinfo_win), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(fileinfo_win), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(fileinfo_win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_transient_for(GTK_WINDOW(fileinfo_win), GTK_WINDOW(mainwin));

    hbox = gtk_hbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(fileinfo_win), hbox);

    image_artwork = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox), image_artwork, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(image_artwork), 0.5, 0);
    gtk_image_set_from_file(GTK_IMAGE(image_artwork), DATA_DIR "/images/audio.png");
    separator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 0);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox1), alignment, TRUE, TRUE, 0);
    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox2);
    
    label_codec = gtk_label_new(_("<span size=\"small\">Codec</span>"));
    gtk_box_pack_start (GTK_BOX (vbox2), label_codec, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_codec), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_codec), 0, 0.5);
    
    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 0, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 0);

    codec_hbox = gtk_hbox_new(FALSE, 6);
    gtk_container_add (GTK_CONTAINER(alignment), codec_hbox);

    image_fileicon = gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (codec_hbox), image_fileicon, FALSE, FALSE, 0);
    
    codec_table = gtk_table_new(2, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE(codec_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE(codec_table), 12);
    gtk_box_pack_start (GTK_BOX (codec_hbox), codec_table, FALSE, FALSE, 0);

    label_codec_label = gtk_label_new(_("<span size=\"small\">Codec name:</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_codec_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_codec_label), 0, 0.5);
    label_quality_label = gtk_label_new(_("<span size=\"small\">Quality:</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_quality_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_quality_label), 0, 0.5);

    label_codec_name = gtk_label_new(_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_codec_name), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_codec_name), 0, 0.5);
    label_quality = gtk_label_new(_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_quality), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_quality), 0, 0.5);
    
    gtk_table_attach(GTK_TABLE(codec_table), label_codec_label, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_codec_name, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_quality_label, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_quality, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);

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
        GdkPixbuf *icon = NULL;

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

        fileinfo_label_set_text(label_codec_name, tuple_get_string(tuple, FIELD_CODEC, NULL));
        fileinfo_label_set_text(label_quality, tuple_get_string(tuple, FIELD_QUALITY, NULL));

        tmp = (gchar *)tuple_get_string(tuple, FIELD_MIMETYPE, NULL);
        icon = mime_icon_lookup(48, tmp ? tmp : "audio/x-generic");
        if (icon) {
            if (image_fileicon) gtk_image_set_from_pixbuf (GTK_IMAGE(image_fileicon), icon);
            g_object_unref(icon);
        }

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
