/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 * Copyright (c) 2008 Eugene Zagidullin
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

#include <gdk/gdkkeysyms.h>
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
#include "effect.h"
#include "audstrings.h"
#include "general.h"
#include "output.h"
#include "playlist-new.h"
#include "probe.h"
#include "visualization.h"

#include "main.h"
#include "util.h"
#include "tuple.h"
#include "vfs.h"

#include "build_stamp.h"
#include "ui_fileinfo.h"

#define G_FREE_CLEAR(a) if(a != NULL) { g_free(a); a = NULL; }
#define STATUS_TIMEOUT 3*1000

GtkWidget *fileinfo_win = NULL;

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
GtkWidget *label_format_name;
GtkWidget *label_quality;
GtkWidget *label_bitrate;
GtkWidget *btn_apply;
GtkWidget *label_mini_status;
GtkWidget *arrow_rawdata;
GtkWidget *treeview_rawdata;

enum {
    RAWDATA_KEY,
    RAWDATA_VALUE,
    RAWDATA_N_COLS
};

static gchar *current_file = NULL;
static InputPlugin *current_ip = NULL;
static gboolean something_changed = FALSE;

/* stolen from Audacious 1.4 vorbis plugin. --nenolod */
static const gchar *genre_table[] = {
    N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
    N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
    N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
    N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
    N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
    N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
    N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
    N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
    N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
    N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("AlternRock"),
    N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
    N_("Meditative"), N_("Instrumental Pop"),
    N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
    N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
    N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
    N_("Southern Rock"), N_("Comedy"), N_("Cult"),
    N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
    N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
    N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
    N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
    N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
    N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
    N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
    N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
    N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
    N_("Gothic Rock"), N_("Progressive Rock"),
    N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
    N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
    N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
    N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
    N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
    N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
    N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
    N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
    N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
    N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
    N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
    N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
    N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
    N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
    N_("Contemporary Christian"), N_("Christian Rock"),
    N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
    N_("Anime"), N_("JPop"), N_("Synthpop")
};

static GList *genre_list = NULL;

static void
fileinfo_entry_set_text(GtkWidget *widget, const char *text)
{
    if (widget == NULL)
        return;

    gtk_entry_set_text(GTK_ENTRY(widget), text != NULL ? text : "");
}

static void
set_entry_str_from_field(GtkWidget *widget, Tuple *tuple, gint fieldn, gboolean editable)
{
    gchar *text;

    if(widget != NULL) {
        text = (gchar*)tuple_get_string(tuple, fieldn, NULL);
        gtk_entry_set_text(GTK_ENTRY(widget), text != NULL ? text : "");
        gtk_editable_set_editable(GTK_EDITABLE(widget), editable);
    }
}

static void
set_entry_int_from_field(GtkWidget *widget, Tuple *tuple, gint fieldn, gboolean editable)
{
    gchar *text;

    if(widget == NULL) return;

    if(tuple_get_value_type(tuple, fieldn, NULL) == TUPLE_INT) {
        text = g_strdup_printf("%d", tuple_get_int(tuple, fieldn, NULL));
        gtk_entry_set_text(GTK_ENTRY(widget), text);
        gtk_editable_set_editable(GTK_EDITABLE(widget), editable);
        g_free(text);
    } else {
        gtk_entry_set_text(GTK_ENTRY(widget), "");
        gtk_editable_set_editable(GTK_EDITABLE(widget), editable);
    }
}

static void
set_field_str_from_entry(Tuple *tuple, gint fieldn, GtkWidget *widget)
{
    if(widget == NULL) return;
    tuple_associate_string(tuple, fieldn, NULL, gtk_entry_get_text(GTK_ENTRY(widget)));
}

static void
set_field_int_from_entry(Tuple *tuple, gint fieldn, GtkWidget *widget)
{
    gchar *tmp;
    if(widget == NULL) return;

    tmp = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget));
    if(*tmp != '\0')
        tuple_associate_int(tuple, fieldn, NULL, atoi(tmp));
    else
        tuple_associate_int(tuple, fieldn, NULL, -1);
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

    if (strcmp(DATA_DIR "/images/audio.png", text)) {
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

        pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height, GDK_INTERP_BILINEAR);
        g_object_unref(G_OBJECT(pixbuf));
        pixbuf = pixbuf2;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
    g_object_unref(G_OBJECT(pixbuf));
}

static int
fileinfo_hide(gpointer unused)
{
    if(GTK_WIDGET_VISIBLE(fileinfo_win)) gtk_widget_hide(fileinfo_win);

    /* Clear it out. */
    fileinfo_entry_set_text(entry_title, "");
    fileinfo_entry_set_text(entry_artist, "");
    fileinfo_entry_set_text(entry_album, "");
    fileinfo_entry_set_text(entry_comment, "");
    fileinfo_entry_set_text(gtk_bin_get_child(GTK_BIN(entry_genre)), "");
    fileinfo_entry_set_text(entry_year, "");
    fileinfo_entry_set_text(entry_track, "");
    fileinfo_entry_set_text(entry_location, "");

    fileinfo_label_set_text(label_format_name, NULL);
    fileinfo_label_set_text(label_quality, NULL);
    fileinfo_label_set_text(label_bitrate, NULL);

    if (label_mini_status != NULL) {
        gtk_label_set_text(GTK_LABEL(label_mini_status), "<span size=\"small\"></span>");
        gtk_label_set_use_markup(GTK_LABEL(label_mini_status), TRUE);
    }

    something_changed = FALSE;
    gtk_widget_set_sensitive(btn_apply, FALSE);

    current_ip = NULL;
    G_FREE_CLEAR(current_file);

    fileinfo_entry_set_image(image_artwork, DATA_DIR "/images/audio.png");
    return 1;
}

static void
entry_changed (GtkEditable *editable, gpointer user_data)
{
    if(current_file != NULL && current_ip != NULL && current_ip->update_song_tuple != NULL) {
        something_changed = TRUE;
        gtk_widget_set_sensitive(btn_apply, TRUE);
    }
}

static gboolean
ministatus_timeout_proc (gpointer data)
{
    GtkLabel *status = GTK_LABEL(data);
    gtk_label_set_text(status, "<span size=\"small\"></span>");
    gtk_label_set_use_markup(status, TRUE);

    return FALSE;
}

static void
ministatus_display_message(gchar *text)
{
    if(label_mini_status != NULL) {
        gchar *tmp = g_strdup_printf("<span size=\"small\">%s</span>", text);
        gtk_label_set_text(GTK_LABEL(label_mini_status), tmp);
        g_free(tmp);
        gtk_label_set_use_markup(GTK_LABEL(label_mini_status), TRUE);
        g_timeout_add (STATUS_TIMEOUT, (GSourceFunc) ministatus_timeout_proc, (gpointer) label_mini_status);
    }
}

static void
message_update_successfull()
{
    ministatus_display_message(_("Metadata updated successfully"));
}

static void
message_update_failed()
{
    ministatus_display_message(_("Metadata updating failed"));
}

static void
fileinfo_update_tuple(gpointer data)
{
    Tuple *tuple;
    VFSFile *fd;

    if (current_file != NULL && current_ip != NULL && current_ip->update_song_tuple != NULL && something_changed) {
        tuple = tuple_new();
        fd = vfs_fopen(current_file, "r+b");

        if (fd != NULL) {
            set_field_str_from_entry(tuple, FIELD_TITLE, entry_title);
            set_field_str_from_entry(tuple, FIELD_ARTIST, entry_artist);
            set_field_str_from_entry(tuple, FIELD_ALBUM, entry_album);
            set_field_str_from_entry(tuple, FIELD_COMMENT, entry_comment);
            set_field_str_from_entry(tuple, FIELD_GENRE, gtk_bin_get_child(GTK_BIN(entry_genre)));

            set_field_int_from_entry(tuple, FIELD_YEAR, entry_year);
            set_field_int_from_entry(tuple, FIELD_TRACK_NUMBER, entry_track);

            plugin_set_current((Plugin *)current_ip);
            if (current_ip->update_song_tuple(tuple, fd)) {
                message_update_successfull();
                something_changed = FALSE;
                gtk_widget_set_sensitive(btn_apply, FALSE);
            } else
                message_update_failed();

            vfs_fclose(fd);

        } else
            message_update_failed();

        mowgli_object_unref(tuple);
    }
}

/**
 * Looks up an icon from a NULL-terminated list of icon names.
 *
 * size: the requested size
 * name: the default name
 * ... : a NULL-terminated list of alternates
 */
GdkPixbuf *
themed_icon_lookup(gint size, const gchar *name, ...)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    gchar *n;
    va_list par;

    icon_theme = gtk_icon_theme_get_default ();
    pixbuf = gtk_icon_theme_load_icon (icon_theme, name, size, 0, &error);

    if (pixbuf != NULL)
        return pixbuf;

    if (error != NULL)
        g_error_free(error);

    /* fallback */
    va_start(par, name);
    while((n = (gchar*)va_arg(par, gchar *)) != NULL) {
        error = NULL;
        pixbuf = gtk_icon_theme_load_icon (icon_theme, n, size, 0, &error);

        if (pixbuf) {
            va_end(par);
            return pixbuf;
        }

        if (error != NULL)
            g_error_free(error);
    }

    return NULL;
}

/**
 * Intelligently looks up an icon for a mimetype. Supports
 * HIDEOUSLY BROKEN gnome icon naming scheme too.
 *
 * size     : the requested size
 * mime_type: the mime type.
 */
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
        icon = themed_icon_lookup(size, mime_as_is, mime_gnome, mime_generic, mime_gnome_generic, s[0], NULL); /* s[0] is category */
        g_free(mime_gnome_generic);
        g_free(mime_generic);
        g_free(mime_gnome);
        g_free(mime_as_is);
    }
    g_strfreev(s);

    return icon;
}

static gboolean fileinfo_keypress (GtkWidget * widget, GdkEventKey * event,
 void * unused)
{
    if (event->keyval == GDK_Escape)
    {
        fileinfo_hide (NULL);
        return TRUE;
    }

    return FALSE;
}

void
create_fileinfo_window(void)
{
    GtkWidget *hbox;
    GtkWidget *hbox_status_and_bbox;
    GtkWidget *vbox0;
    GtkWidget *vbox1;
    GtkWidget *vbox2;
    GtkWidget *vbox3;
    GtkWidget *label_title;
    GtkWidget *label_artist;
    GtkWidget *label_album;
    GtkWidget *label_comment;
    GtkWidget *label_genre;
    GtkWidget *label_year;
    GtkWidget *label_track;
    GtkWidget *label_location;
    GtkWidget *label_general;
    GtkWidget *label_format;
    GtkWidget *label_quality_label;
    GtkWidget *label_bitrate_label;
    GtkWidget *codec_hbox;
    GtkWidget *codec_table;
    GtkWidget *table1;
    GtkWidget *bbox_close;
    GtkWidget *btn_close;
    GtkWidget *alignment;
    GtkWidget *separator;
    GtkWidget *scrolledwindow;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint i;

    fileinfo_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(fileinfo_win), 6);
    gtk_window_set_title(GTK_WINDOW(fileinfo_win), _("Track Information"));
    gtk_window_set_position(GTK_WINDOW(fileinfo_win), GTK_WIN_POS_CENTER);
    gtk_window_set_type_hint(GTK_WINDOW(fileinfo_win), GDK_WINDOW_TYPE_HINT_DIALOG);

    vbox0 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(fileinfo_win), vbox0);

    hbox = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox0), hbox, TRUE, TRUE, 0);

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

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox1), alignment, TRUE, TRUE, 0);

    vbox3 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox3);

    label_general = gtk_label_new(_("<span size=\"small\">General</span>"));
    gtk_box_pack_start (GTK_BOX (vbox2), label_general, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_general), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_general), 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 0, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 0);

    codec_hbox = gtk_hbox_new(FALSE, 6);
    gtk_container_add (GTK_CONTAINER(alignment), codec_hbox);

    image_fileicon = gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (codec_hbox), image_fileicon, FALSE, FALSE, 0);

    codec_table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE(codec_table), 6);
    gtk_table_set_col_spacings (GTK_TABLE(codec_table), 12);
    gtk_box_pack_start (GTK_BOX (codec_hbox), codec_table, FALSE, FALSE, 0);

    label_format = gtk_label_new(_("<span size=\"small\">Format:</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_format), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_format), 0, 0.5);
    label_quality_label = gtk_label_new(_("<span size=\"small\">Quality:</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_quality_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_quality_label), 0, 0.5);
    label_bitrate_label = gtk_label_new(_("<span size=\"small\">Bitrate:</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_bitrate_label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_bitrate_label), 0, 0.5);

    label_format_name = gtk_label_new(_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_format_name), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_format_name), 0, 0.5);
    label_quality = gtk_label_new(_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_quality), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_quality), 0, 0.5);
    label_bitrate = gtk_label_new(_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup(GTK_LABEL(label_bitrate), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_bitrate), 0, 0.5);

    gtk_table_attach(GTK_TABLE(codec_table), label_format, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_format_name, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_quality_label, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_quality, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_bitrate_label, 0, 1, 2, 3,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach(GTK_TABLE(codec_table), label_bitrate, 1, 2, 2, 3,
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
    g_signal_connect(G_OBJECT(entry_title), "changed", (GCallback) entry_changed, NULL);

    label_artist = gtk_label_new(_("<span size=\"small\">Artist</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_artist, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_artist), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_artist), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_artist = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_artist);
    g_signal_connect(G_OBJECT(entry_artist), "changed", (GCallback) entry_changed, NULL);

    label_album = gtk_label_new(_("<span size=\"small\">Album</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_album, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_album), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_album), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_album = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_album);
    g_signal_connect(G_OBJECT(entry_album), "changed", (GCallback) entry_changed, NULL);

    label_comment = gtk_label_new(_("<span size=\"small\">Comment</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_comment, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_comment), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_comment), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_comment = gtk_entry_new();
    gtk_container_add (GTK_CONTAINER(alignment), entry_comment);
    g_signal_connect(G_OBJECT(entry_comment), "changed", (GCallback) entry_changed, NULL);

    label_genre = gtk_label_new(_("<span size=\"small\">Genre</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_genre, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_genre), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_genre), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 0, 0);
    entry_genre = gtk_combo_box_entry_new_text();

    if (!genre_list) {
        GList *iter;

        for (i = 0; i < G_N_ELEMENTS(genre_table); i++)
            genre_list = g_list_prepend(genre_list, _(genre_table[i]));
        genre_list = g_list_sort(genre_list, (GCompareFunc) g_utf8_collate);

        MOWGLI_ITER_FOREACH(iter, genre_list)
            gtk_combo_box_append_text(GTK_COMBO_BOX(entry_genre), iter->data);
    }

    gtk_container_add(GTK_CONTAINER(alignment), entry_genre);
    g_signal_connect(G_OBJECT(entry_genre), "changed", (GCallback) entry_changed, NULL);

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
    g_signal_connect(G_OBJECT(entry_year), "changed", (GCallback) entry_changed, NULL);

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
    g_signal_connect(G_OBJECT(entry_track), "changed", (GCallback) entry_changed, NULL);

    label_location = gtk_label_new(_("<span size=\"small\">Location</span>"));
    gtk_box_pack_start(GTK_BOX(vbox2), label_location, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_location), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_location), 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 6, 0, 0);

    entry_location = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(alignment), entry_location);
    gtk_editable_set_editable(GTK_EDITABLE(entry_location), FALSE);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_box_pack_start(GTK_BOX(vbox3), alignment, TRUE, TRUE, 0);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 6, 0, 0);
    arrow_rawdata = gtk_expander_new(_("<span size=\"small\">Raw Metadata</span>"));
    gtk_expander_set_use_markup(GTK_EXPANDER(arrow_rawdata), TRUE);
    gtk_container_add(GTK_CONTAINER(alignment), arrow_rawdata);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(arrow_rawdata), scrolledwindow);

    treeview_rawdata = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolledwindow), treeview_rawdata);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_rawdata), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview_rawdata), TRUE);
    gtk_widget_set_size_request(treeview_rawdata, -1, 130);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Key"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", RAWDATA_KEY, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_rawdata), column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Value"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", RAWDATA_VALUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_rawdata), column);

    hbox_status_and_bbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox0), hbox_status_and_bbox, FALSE, FALSE, 0);

    label_mini_status = gtk_label_new("<span size=\"small\"></span>");
    gtk_label_set_use_markup(GTK_LABEL(label_mini_status), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_mini_status), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox_status_and_bbox), label_mini_status, TRUE, TRUE, 0);

    bbox_close = gtk_hbutton_box_new();
    gtk_box_set_spacing(GTK_BOX(bbox_close), 6);
    gtk_box_pack_start(GTK_BOX(hbox_status_and_bbox), bbox_close, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox_close), GTK_BUTTONBOX_END);

    btn_apply = gtk_button_new_from_stock("gtk-save");
    gtk_container_add(GTK_CONTAINER(bbox_close), btn_apply);
    g_signal_connect(G_OBJECT(btn_apply), "clicked", (GCallback) fileinfo_update_tuple, NULL);
    gtk_widget_set_sensitive(btn_apply, FALSE);

    btn_close = gtk_button_new_from_stock("gtk-close");
    gtk_container_add(GTK_CONTAINER(bbox_close), btn_close);
    GTK_WIDGET_SET_FLAGS(btn_close, GTK_CAN_DEFAULT);
    g_signal_connect(G_OBJECT(btn_close), "clicked", (GCallback) fileinfo_hide, NULL);
    g_signal_connect ((GObject *) fileinfo_win, "delete-event", (GCallback)
     fileinfo_hide, NULL);
    g_signal_connect ((GObject *) fileinfo_win, "key-press-event", (GCallback)
     fileinfo_keypress, NULL);

    gtk_widget_show_all (vbox0);
}

static void
fileinfo_show_for_tuple(Tuple *tuple, gboolean updating_enabled)
{
    gchar *tmp = NULL;
    GdkPixbuf *icon = NULL;
    GtkTreeIter iter;
    GtkListStore *store;
    mowgli_dictionary_iteration_state_t state;
    TupleValue *tvalue;
    gint i;

    if (tuple == NULL)
        return;

    if(!updating_enabled) {
        current_ip = NULL;
        G_FREE_CLEAR(current_file);
    }

    something_changed = FALSE;

    if (fileinfo_win == NULL)
        create_fileinfo_window();

    if (!GTK_WIDGET_REALIZED(fileinfo_win))
        gtk_widget_realize(fileinfo_win);

    set_entry_str_from_field(entry_title, tuple, FIELD_TITLE, updating_enabled);
    set_entry_str_from_field(entry_artist, tuple, FIELD_ARTIST, updating_enabled);
    set_entry_str_from_field(entry_album, tuple, FIELD_ALBUM, updating_enabled);
    set_entry_str_from_field(entry_comment, tuple, FIELD_COMMENT, updating_enabled);
    set_entry_str_from_field(gtk_bin_get_child(GTK_BIN(entry_genre)), tuple, FIELD_GENRE, updating_enabled);

    tmp = g_strdup_printf ("%s%s",
            tuple_get_string(tuple, FIELD_FILE_PATH, NULL),
            tuple_get_string(tuple, FIELD_FILE_NAME, NULL));

    if (tmp) {
        fileinfo_entry_set_text(entry_location, tmp);
        g_free(tmp);
    }

    /* set empty string if field not availaible. --eugene */
    set_entry_int_from_field(entry_year, tuple, FIELD_YEAR, updating_enabled);
    set_entry_int_from_field(entry_track, tuple, FIELD_TRACK_NUMBER, updating_enabled);

    fileinfo_label_set_text(label_format_name, tuple_get_string(tuple, FIELD_CODEC, NULL));
    fileinfo_label_set_text(label_quality, tuple_get_string(tuple, FIELD_QUALITY, NULL));

    if (tuple_get_value_type(tuple, FIELD_BITRATE, NULL) == TUPLE_INT) {
        tmp = g_strdup_printf(_("%d kb/s"), tuple_get_int(tuple, FIELD_BITRATE, NULL));
        fileinfo_label_set_text(label_bitrate, tmp);
        g_free(tmp);
    } else
        fileinfo_label_set_text(label_bitrate, NULL);

    tmp = (gchar *)tuple_get_string(tuple, FIELD_MIMETYPE, NULL);
    icon = mime_icon_lookup(48, tmp ? tmp : "audio/x-generic");
    if (icon) {
        if (image_fileicon) gtk_image_set_from_pixbuf (GTK_IMAGE(image_fileicon), icon);
        g_object_unref(icon);
    }

    tmp = fileinfo_recursive_get_image(
            tuple_get_string(tuple, FIELD_FILE_PATH, NULL),
            tuple_get_string(tuple, FIELD_FILE_NAME, NULL), 0);

    if (tmp) {
        fileinfo_entry_set_image(image_artwork, tmp);
        g_free(tmp);
    }

    gtk_widget_set_sensitive(btn_apply, FALSE);

    if (label_mini_status != NULL) {
        gtk_label_set_text(GTK_LABEL(label_mini_status), "<span size=\"small\"></span>");
        gtk_label_set_use_markup(GTK_LABEL(label_mini_status), TRUE);
    }

    store = gtk_list_store_new(RAWDATA_N_COLS, G_TYPE_STRING, G_TYPE_STRING);

    for (i = 0; i < FIELD_LAST; i++) {
         gchar *key, *value;

         if (!tuple->values[i])
             continue;

         if (tuple->values[i]->type != TUPLE_INT && tuple->values[i]->value.string)
             value = g_strdup(tuple->values[i]->value.string);
         else if (tuple->values[i]->type == TUPLE_INT)
             value = g_strdup_printf("%d", tuple->values[i]->value.integer);
         else
             continue;

         key = g_strdup(tuple_fields[i].name);

         gtk_list_store_append(store, &iter);
         gtk_list_store_set(store, &iter,
                            RAWDATA_KEY, key,
                            RAWDATA_VALUE, value, -1);

         g_free(key);
         g_free(value);
    }

    /* non-standard values are stored in a dictionary. */
    MOWGLI_DICTIONARY_FOREACH(tvalue, &state, tuple->dict) {
         gchar *key, *value;

         if (tvalue->type != TUPLE_INT && tvalue->value.string)
             value = g_strdup(tvalue->value.string);
         else if (tvalue->type == TUPLE_INT)
             value = g_strdup_printf("%d", tvalue->value.integer);
         else
             continue;

         key = g_strdup(state.cur->key);

         gtk_list_store_append(store, &iter);
         gtk_list_store_set(store, &iter,
                            RAWDATA_KEY, key,
                            RAWDATA_VALUE, value, -1);

         g_free(key);
         g_free(value);
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_rawdata), GTK_TREE_MODEL(store));
    g_object_unref(store);

    if (!GTK_WIDGET_VISIBLE(fileinfo_win))
        gtk_widget_show(fileinfo_win);
}

static void fileinfo_show_editor_for_path (const gchar * path, InputPlugin * ip)
{
    G_FREE_CLEAR(current_file);
    current_file = g_strdup(path);
    current_ip = ip;

    Tuple *tuple = input_get_song_tuple(path);

    if (tuple == NULL) {
        input_file_info_box(path);
        return;
    }

    fileinfo_show_for_tuple(tuple, TRUE);

    mowgli_object_unref(tuple);
}

void ui_fileinfo_show (gint playlist, gint entry)
{
    const gchar * filename = playlist_entry_get_filename (playlist, entry);
    InputPlugin * decoder = playlist_entry_get_decoder (playlist, entry);
    Tuple * tuple;

    g_return_if_fail (filename != NULL);

    if (decoder == NULL)
        decoder = file_probe (filename, FALSE);

    if (decoder == NULL)
    {
        gchar * message = g_strdup_printf ("No decoder found for %s.\n",
         filename);

        hook_call ("interface show error", message);
        g_free (message);
        return;
    }

    if (decoder->file_info_box != NULL)
        decoder->file_info_box (filename);
    else if (decoder->update_song_tuple != NULL && ! vfs_is_remote (filename))
        fileinfo_show_editor_for_path (filename, decoder);
    else if ((tuple = (Tuple *) playlist_entry_get_tuple (playlist, entry)) !=
     NULL)
        fileinfo_show_for_tuple (tuple, FALSE);
    else if (decoder->get_song_tuple != NULL && (tuple = decoder->get_song_tuple
     (filename)) != NULL)
    {
        fileinfo_show_for_tuple (tuple, FALSE);
        playlist_entry_set_tuple (playlist, entry, tuple);
    }
    else
    {
        gchar * message = g_strdup_printf ("No info available for "
         "%s.\n", filename);

        hook_call ("interface show error", message);
        g_free (message);
    }
}

void ui_fileinfo_show_current (void)
{
    gint playlist = playlist_get_playing ();

    if (playlist == -1)
        playlist = playlist_get_active ();

    ui_fileinfo_show (playlist, playlist_get_position (playlist));
}
