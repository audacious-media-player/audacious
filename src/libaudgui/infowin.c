/*
 * libaudgui/infowin.c
 * Copyright 2006 William Pitcock, Tony Vroon, George Averill, Giacomo Lozito,
 *  Derek Pomery and Yoshiki Yazawa.
 * Copyright 2008 Eugene Zagidullin
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
#include <stdarg.h>

#include <audacious/i18n.h>
#include <audacious/plugin.h>
#include <libaudcore/audstrings.h>

#include "libaudgui.h"
#include "libaudgui-gtk.h"

#define STATUS_TIMEOUT 3000

static GtkWidget * infowin = NULL;

static GtkWidget * location_text;
static GtkWidget * entry_title;
static GtkWidget * entry_artist;
static GtkWidget * entry_album;
static GtkWidget * entry_comment;
static GtkWidget * entry_year;
static GtkWidget * entry_track;
static GtkWidget * entry_genre;

static GtkWidget * image_artwork;

static GtkWidget * image_fileicon;
static GtkWidget * label_format_name;
static GtkWidget * label_quality;
static GtkWidget * label_bitrate;
static GtkWidget * btn_apply;
static GtkWidget * label_mini_status;
static GtkWidget * arrow_rawdata;
static GtkWidget * treeview_rawdata;

enum
{
    RAWDATA_KEY,
    RAWDATA_VALUE,
    RAWDATA_N_COLS
};

static gchar * current_file = NULL;
static InputPlugin * current_decoder = NULL;
static gboolean can_write = FALSE, something_changed = FALSE;

static const gchar * genre_table[] =
{
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

static void set_entry_str_from_field (GtkWidget * widget, Tuple * tuple,
 gint fieldn, gboolean editable)
{
    const gchar * text = tuple_get_string (tuple, fieldn, NULL);

    gtk_entry_set_text ((GtkEntry *) widget, text != NULL ? text : "");
    gtk_editable_set_editable ((GtkEditable *) widget, editable);
}

static void set_entry_int_from_field (GtkWidget * widget, Tuple * tuple, gint
 fieldn, gboolean editable)
{
    gchar scratch[32];

    if (tuple_get_value_type (tuple, fieldn, NULL) == TUPLE_INT)
        snprintf (scratch, sizeof scratch, "%d", tuple_get_int (tuple, fieldn,
         NULL));
    else
        scratch[0] = 0;

    gtk_entry_set_text ((GtkEntry *) widget, scratch);
    gtk_editable_set_editable ((GtkEditable *) widget, editable);
}

static void set_field_str_from_entry (Tuple * tuple, gint fieldn, GtkWidget *
 widget)
{
    const gchar * text = gtk_entry_get_text ((GtkEntry *) widget);

    if (text[0])
        tuple_associate_string (tuple, fieldn, NULL, text);
    else
        tuple_disassociate (tuple, fieldn, NULL);
}

static void set_field_int_from_entry (Tuple * tuple, gint fieldn, GtkWidget *
 widget)
{
    const gchar * text = gtk_entry_get_text ((GtkEntry *) widget);

    if (text[0])
        tuple_associate_int (tuple, fieldn, NULL, atoi (text));
    else
        tuple_disassociate (tuple, fieldn, NULL);
}

static void infowin_label_set_text (GtkWidget * widget, const gchar * text)
{
    gchar * tmp;

    if (text != NULL)
    {
        tmp = g_strdup_printf("<span size=\"small\">%s</span>", text);
        gtk_label_set_text ((GtkLabel *) widget, tmp);
        g_free (tmp);
    }
    else
        gtk_label_set_text ((GtkLabel *) widget,
         _("<span size=\"small\">n/a</span>"));

    gtk_label_set_use_markup ((GtkLabel *) widget, TRUE);
}

static void infowin_entry_set_image (GtkWidget * widget, const char * text)
{
    GdkPixbuf * pixbuf;

    pixbuf = gdk_pixbuf_new_from_file (text, NULL);
    g_return_if_fail (pixbuf != NULL);

    if (strcmp (DATA_DIR "/images/audio.png", text))
        audgui_pixbuf_scale_within (& pixbuf, aud_cfg->filepopup_pixelsize);

    gtk_image_set_from_pixbuf ((GtkImage *) widget, pixbuf);
    g_object_unref (pixbuf);
}

static void clear_infowin (void)
{
    gtk_entry_set_text ((GtkEntry *) entry_title, "");
    gtk_entry_set_text ((GtkEntry *) entry_artist, "");
    gtk_entry_set_text ((GtkEntry *) entry_album, "");
    gtk_entry_set_text ((GtkEntry *) entry_comment, "");
    gtk_entry_set_text ((GtkEntry *) gtk_bin_get_child ((GtkBin *) entry_genre),
     "");
    gtk_entry_set_text ((GtkEntry *) entry_year, "");
    gtk_entry_set_text ((GtkEntry *) entry_track, "");

    infowin_label_set_text (label_format_name, NULL);
    infowin_label_set_text (label_quality, NULL);
    infowin_label_set_text (label_bitrate, NULL);

    gtk_label_set_text ((GtkLabel *) label_mini_status,
     "<span size=\"small\"></span>");
    gtk_label_set_use_markup ((GtkLabel *) label_mini_status, TRUE);

    g_free (current_file);
    current_file = NULL;
    current_decoder = NULL;

    something_changed = FALSE;
    can_write = FALSE;
    gtk_widget_set_sensitive (btn_apply, FALSE);

    infowin_entry_set_image (image_artwork, DATA_DIR "/images/audio.png");
}

static void entry_changed (GtkEditable * editable, void * unused)
{
    if (! something_changed && can_write)
    {
        something_changed = TRUE;
        gtk_widget_set_sensitive (btn_apply, TRUE);
    }
}

static gboolean ministatus_timeout_proc (void * data)
{
    GtkLabel * status = data;

    gtk_label_set_text (status, "<span size=\"small\"></span>");
    gtk_label_set_use_markup (status, TRUE);

    return FALSE;
}

static void ministatus_display_message (const gchar * text)
{
    gchar * tmp = g_strdup_printf ("<span size=\"small\">%s</span>", text);

    gtk_label_set_text ((GtkLabel *) label_mini_status, tmp);
    gtk_label_set_use_markup ((GtkLabel *) label_mini_status, TRUE);
    g_free (tmp);

    g_timeout_add (STATUS_TIMEOUT, (GSourceFunc) ministatus_timeout_proc,
     label_mini_status);
}

static void infowin_update_tuple (void * unused)
{
    Tuple * tuple = tuple_new_from_filename (current_file);

    set_field_str_from_entry (tuple, FIELD_TITLE, entry_title);
    set_field_str_from_entry (tuple, FIELD_ARTIST, entry_artist);
    set_field_str_from_entry (tuple, FIELD_ALBUM, entry_album);
    set_field_str_from_entry (tuple, FIELD_COMMENT, entry_comment);
    set_field_str_from_entry (tuple, FIELD_GENRE, gtk_bin_get_child ((GtkBin *)
     entry_genre));
    set_field_int_from_entry (tuple, FIELD_YEAR, entry_year);
    set_field_int_from_entry (tuple, FIELD_TRACK_NUMBER, entry_track);

    if (aud_file_write_tuple (current_file, current_decoder, tuple))
    {
        ministatus_display_message (_("Metadata updated successfully"));
        something_changed = FALSE;
        gtk_widget_set_sensitive (btn_apply, FALSE);
    }
    else
        ministatus_display_message (_("Metadata updating failed"));

    mowgli_object_unref (tuple);
}

/**
 * Looks up an icon from a NULL-terminated list of icon names.
 *
 * size: the requested size
 * name: the default name
 * ... : a NULL-terminated list of alternates
 */
static GdkPixbuf * themed_icon_lookup (gint size, const gchar * name, ...)
{
    GtkIconTheme * icon_theme;
    GdkPixbuf * pixbuf;
    const gchar * n;
    va_list par;

    icon_theme = gtk_icon_theme_get_default ();
    pixbuf = gtk_icon_theme_load_icon (icon_theme, name, size, 0, NULL);

    if (pixbuf != NULL)
        return pixbuf;

    /* fallback */
    va_start (par, name);

    while ((n = va_arg (par, const gchar *)) != NULL)
    {
        pixbuf = gtk_icon_theme_load_icon (icon_theme, n, size, 0, NULL);

        if (pixbuf)
        {
            va_end (par);
            return pixbuf;
        }
    }

    va_end (par);
    return NULL;
}

/**
 * Intelligently looks up an icon for a mimetype. Supports
 * HIDEOUSLY BROKEN gnome icon naming scheme too.
 *
 * size     : the requested size
 * mime_type: the mime type.
 */
static GdkPixbuf * mime_icon_lookup (gint size, const gchar * mime_type)
{
    gchar * mime_as_is;         /* audio-x-mp3 */
    gchar * mime_gnome;         /* gnome-mime-audio-x-mp3 */
    gchar * mime_generic;       /* audio-x-generic */
    gchar * mime_gnome_generic; /* gnome-mime-audio */
    GdkPixbuf * icon = NULL;
    gchar * * s = g_strsplit (mime_type, "/", 2);

    if (s[1] != NULL)
    {
        mime_as_is = g_strdup_printf ("%s-%s", s[0], s[1]);
        mime_gnome = g_strdup_printf ("gnome-mime-%s-%s", s[0], s[1]);
        mime_generic = g_strdup_printf ("%s-x-generic", s[0]);
        mime_gnome_generic = g_strdup_printf ("gnome-mime-%s", s[0]);

        icon = themed_icon_lookup (size, mime_as_is, mime_gnome, mime_generic,
         mime_gnome_generic, s[0], NULL); /* s[0] is category */

        g_free (mime_gnome_generic);
        g_free (mime_generic);
        g_free (mime_gnome);
        g_free (mime_as_is);
    }

    g_strfreev (s);
    return icon;
}

gboolean genre_fill (GtkWidget * combo)
{
    GList * list = NULL;
    GList * node;
    gint i;

    for (i = 0; i < G_N_ELEMENTS (genre_table); i ++)
        list = g_list_prepend (list, _(genre_table[i]));

    list = g_list_sort (list, (GCompareFunc) strcmp);

    for (node = list; node != NULL; node = node->next)
        gtk_combo_box_append_text ((GtkComboBox *) combo, node->data);

    g_list_free (list);
    return FALSE;
}

void create_infowin (void)
{
    GtkWidget * hbox;
    GtkWidget * hbox_status_and_bbox;
    GtkWidget * vbox0;
    GtkWidget * vbox1;
    GtkWidget * vbox2;
    GtkWidget * vbox3;
    GtkWidget * label_title;
    GtkWidget * label_artist;
    GtkWidget * label_album;
    GtkWidget * label_comment;
    GtkWidget * label_genre;
    GtkWidget * label_year;
    GtkWidget * label_track;
    GtkWidget * label_location;
    GtkWidget * label_general;
    GtkWidget * label_format;
    GtkWidget * label_quality_label;
    GtkWidget * label_bitrate_label;
    GtkWidget * codec_hbox;
    GtkWidget * codec_table;
    GtkWidget * table1;
    GtkWidget * bbox_close;
    GtkWidget * btn_close;
    GtkWidget * alignment;
    GtkWidget * separator;
    GtkWidget * scrolledwindow;
    GtkTreeViewColumn * column;
    GtkCellRenderer * renderer;

    infowin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width ((GtkContainer *) infowin, 6);
    gtk_window_set_title ((GtkWindow *) infowin, _("Track Information"));
    gtk_window_set_type_hint ((GtkWindow *) infowin,
     GDK_WINDOW_TYPE_HINT_DIALOG);

    vbox0 = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) infowin, vbox0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox0, hbox, TRUE, TRUE, 0);

    image_artwork = gtk_image_new ();
    gtk_box_pack_start ((GtkBox *) hbox, image_artwork, FALSE, FALSE, 0);
    gtk_misc_set_alignment ((GtkMisc *) image_artwork, 0.5, 0);
    gtk_image_set_from_file ((GtkImage *) image_artwork, DATA_DIR
     "/images/audio.png");
    separator = gtk_vseparator_new ();
    gtk_box_pack_start ((GtkBox *) hbox, separator, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start ((GtkBox *) hbox, vbox1, TRUE, TRUE, 0);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox1, alignment, TRUE, TRUE, 0);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) alignment, vbox2);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox1, alignment, TRUE, TRUE, 0);

    vbox3 = gtk_vbox_new(FALSE, 0);
    gtk_container_add ((GtkContainer *) alignment, vbox3);

    label_general = gtk_label_new (_("<span size=\"small\">General</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_general, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_general, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_general, 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 6, 0, 0);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);

    codec_hbox = gtk_hbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) alignment, codec_hbox);

    image_fileicon = gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE,
     GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start ((GtkBox *) codec_hbox, image_fileicon, FALSE, FALSE, 0);

    codec_table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings ((GtkTable *) codec_table, 6);
    gtk_table_set_col_spacings ((GtkTable *) codec_table, 12);
    gtk_box_pack_start ((GtkBox *) codec_hbox, codec_table, FALSE, FALSE, 0);

    label_format = gtk_label_new (_("<span size=\"small\">Format:</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_format, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_format, 0, 0.5);
    label_quality_label = gtk_label_new
     (_("<span size=\"small\">Quality:</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_quality_label, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_quality_label, 0, 0.5);
    label_bitrate_label = gtk_label_new (_("<span size=\"small\">Bitrate:</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_bitrate_label, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_bitrate_label, 0, 0.5);

    label_format_name = gtk_label_new (_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_format_name, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_format_name, 0, 0.5);
    label_quality = gtk_label_new (_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_quality, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_quality, 0, 0.5);
    label_bitrate = gtk_label_new (_("<span size=\"small\">n/a</span>"));
    gtk_label_set_use_markup ((GtkLabel *) label_bitrate, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_bitrate, 0, 0.5);

    gtk_table_attach ((GtkTable *) codec_table, label_format, 0, 1, 0, 1,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) codec_table, label_format_name, 1, 2, 0, 1,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) codec_table, label_quality_label, 0, 1, 1, 2,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) codec_table, label_quality, 1, 2, 1, 2,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) codec_table, label_bitrate_label, 0, 1, 2, 3,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach ((GtkTable *) codec_table, label_bitrate, 1, 2, 2, 3,
     GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label_title = gtk_label_new (_("<span size=\"small\">Title</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_title, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_title, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_title, 0, 0);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    entry_title = gtk_entry_new ();
    gtk_container_add ((GtkContainer *) alignment, entry_title);
    g_signal_connect (entry_title, "changed", (GCallback) entry_changed, NULL);

    label_artist = gtk_label_new (_("<span size=\"small\">Artist</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_artist, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_artist, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_artist, 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    entry_artist = gtk_entry_new ();
    gtk_container_add ((GtkContainer *) alignment, entry_artist);
    g_signal_connect (entry_artist, "changed", (GCallback) entry_changed, NULL);

    label_album = gtk_label_new (_("<span size=\"small\">Album</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_album, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_album, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_album, 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    entry_album = gtk_entry_new ();
    gtk_container_add ((GtkContainer *) alignment, entry_album);
    g_signal_connect (entry_album, "changed", (GCallback) entry_changed, NULL);

    label_comment = gtk_label_new (_("<span size=\"small\">Comment</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_comment, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_comment, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_comment, 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    entry_comment = gtk_entry_new ();
    gtk_container_add ((GtkContainer *) alignment, entry_comment);
    g_signal_connect (entry_comment, "changed", (GCallback) entry_changed, NULL);

    label_genre = gtk_label_new (_("<span size=\"small\">Genre</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_genre, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_genre, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_genre, 0, 0.5);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    entry_genre = gtk_combo_box_entry_new_text ();

    gtk_container_add ((GtkContainer *) alignment, entry_genre);
    g_signal_connect (entry_genre, "changed", (GCallback) entry_changed, NULL);
    g_idle_add ((GSourceFunc) genre_fill, entry_genre);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 6, 0, 0);
    table1 = gtk_table_new (2, 2, FALSE);
    gtk_container_add ((GtkContainer *) alignment, table1);
    gtk_table_set_col_spacings ((GtkTable *) table1, 6);

    label_year = gtk_label_new (_("<span size=\"small\">Year</span>"));
    gtk_table_attach ((GtkTable *) table1, label_year, 0, 1, 0, 1, GTK_FILL, 0,
     0, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_year, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_year, 0, 0.5);

    entry_year = gtk_entry_new ();
    gtk_table_attach ((GtkTable *) table1, entry_year, 0, 1, 1, 2, GTK_EXPAND |
     GTK_FILL, 0, 0, 0);
    g_signal_connect (entry_year, "changed", (GCallback) entry_changed, NULL);

    label_track = gtk_label_new (_("<span size=\"small\">Track Number</span>"));
    gtk_table_attach ((GtkTable *) table1, label_track, 1, 2, 0, 1, GTK_FILL, 0,
     0, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_track, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_track, 0, 0.5);

    entry_track = gtk_entry_new ();
    gtk_table_attach ((GtkTable *) table1, entry_track, 1, 2, 1, 2, GTK_EXPAND |
     GTK_FILL, 0, 0, 0);
    g_signal_connect (entry_track, "changed", (GCallback) entry_changed, NULL);

    label_location = gtk_label_new (_("<span size=\"small\">Location</span>"));
    gtk_box_pack_start ((GtkBox *) vbox2, label_location, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) label_location, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_location, 0, 0.5);

    alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 3, 6, 25, 0);
    gtk_box_pack_start ((GtkBox *) vbox2, alignment, FALSE, FALSE, 0);

    location_text = gtk_label_new ("");
    gtk_widget_set_size_request (location_text, 375, -1);
    gtk_label_set_line_wrap ((GtkLabel *) location_text, TRUE);
#if GTK_CHECK_VERSION (2, 10, 0)
    gtk_label_set_line_wrap_mode ((GtkLabel *) location_text,
     PANGO_WRAP_WORD_CHAR);
#endif
    gtk_label_set_selectable ((GtkLabel *) location_text, TRUE);
    gtk_container_add ((GtkContainer *) alignment, location_text);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) alignment, hbox);
    gtk_box_pack_start ((GtkBox *) vbox3, alignment, TRUE, TRUE, 0);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_alignment_set_padding ((GtkAlignment *) (alignment), 0, 6, 0, 0);
    arrow_rawdata = gtk_expander_new
     (_("<span size=\"small\">Raw Metadata</span>"));
    gtk_expander_set_use_markup ((GtkExpander *) arrow_rawdata, TRUE);
    gtk_container_add ((GtkContainer *) alignment, arrow_rawdata);
    gtk_box_pack_start ((GtkBox *) hbox, alignment, TRUE, TRUE, 0);

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolledwindow,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolledwindow,
     GTK_SHADOW_IN);
    gtk_container_add ((GtkContainer *) arrow_rawdata, scrolledwindow);

    treeview_rawdata = gtk_tree_view_new ();
    gtk_container_add ((GtkContainer *) scrolledwindow, treeview_rawdata);
    gtk_tree_view_set_rules_hint ((GtkTreeView *) treeview_rawdata, TRUE);
    gtk_tree_view_set_reorderable ((GtkTreeView *) treeview_rawdata, TRUE);
    gtk_widget_set_size_request (treeview_rawdata, -1, 130);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Key"));
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing (column, 4);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_column_set_fixed_width (column, 50);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", RAWDATA_KEY,
     NULL);
    gtk_tree_view_append_column ((GtkTreeView *) treeview_rawdata, column);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Value"));
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing (column, 4);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_column_set_fixed_width (column, 50);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text",
     RAWDATA_VALUE, NULL);
    gtk_tree_view_append_column ((GtkTreeView *) treeview_rawdata, column);

    hbox_status_and_bbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start ((GtkBox *) vbox0, hbox_status_and_bbox, FALSE, FALSE, 0);

    label_mini_status = gtk_label_new ("<span size=\"small\"></span>");
    gtk_label_set_use_markup ((GtkLabel *) label_mini_status, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label_mini_status, 0, 0.5);
    gtk_box_pack_start ((GtkBox *) hbox_status_and_bbox, label_mini_status,
     TRUE, TRUE, 0);

    bbox_close = gtk_hbutton_box_new ();
    gtk_box_set_spacing ((GtkBox *) bbox_close, 6);
    gtk_box_pack_start ((GtkBox *) hbox_status_and_bbox, bbox_close, FALSE,
     FALSE, 0);
    gtk_button_box_set_layout ((GtkButtonBox *) bbox_close, GTK_BUTTONBOX_END);

    btn_apply = gtk_button_new_from_stock (GTK_STOCK_SAVE);
    gtk_container_add ((GtkContainer *) bbox_close, btn_apply);
    g_signal_connect (btn_apply, "clicked", (GCallback) infowin_update_tuple,
     NULL);
    gtk_widget_set_sensitive (btn_apply, FALSE);

    btn_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_container_add ((GtkContainer *) bbox_close, btn_close);
    GTK_WIDGET_SET_FLAGS (btn_close, GTK_CAN_DEFAULT);
    g_signal_connect_swapped (btn_close, "clicked", (GCallback) gtk_widget_hide,
     infowin);

    audgui_hide_on_delete (infowin);
    audgui_hide_on_escape (infowin);

    gtk_widget_show_all (vbox0);
}

/*  Converts filenames (in place) for easy reading, thus:
 *
 *  file:///home/me/Music/My song.ogg  ->  Music
 *                                         My song.ogg
 *
 *  file:///media/disk/My song.ogg  ->  /
 *                                      media
 *                                      disk
 *                                      My song.ogg
 */
static gchar * easy_read_filename (gchar * file)
{
    const gchar * home;
    gint len;

    if (strncmp (file, "file:///", 8))
        return file;

    home = getenv ("HOME");
    len = (home == NULL) ? 0 : strlen (home);
    len = (len > 0 && home[len - 1] == '/') ? len - 1 : len;

    if (len > 0 && ! strncmp (file + 7, home, len) && file[len + 7] == '/')
    {
        string_replace_char (file + len + 8, '/', '\n');
        return file + len + 8;
    }

    string_replace_char (file + 7, '/', '\n');
    return file + 6;
}

static gboolean set_image_from_album_art (const gchar * filename, InputPlugin *
 decoder)
{
    GdkPixbuf * pixbuf = NULL;
    void * data;
    gint size;

    if (aud_file_read_image (filename, decoder, & data, & size))
    {
        pixbuf = audgui_pixbuf_from_data (data, size);
        g_free (data);
    }

    if (pixbuf == NULL)
        return FALSE;

    audgui_pixbuf_scale_within (& pixbuf, aud_cfg->filepopup_pixelsize);
    gtk_image_set_from_pixbuf ((GtkImage *) image_artwork, pixbuf);
    g_object_unref (pixbuf);
    return TRUE;
}

static void infowin_show (const gchar * filename, Tuple * tuple, InputPlugin *
 decoder, gboolean updating_enabled)
{
    const gchar * string;
    gchar * tmp;
    GdkPixbuf * icon;
    GtkTreeIter iter;
    GtkListStore * store;
    mowgli_dictionary_iteration_state_t state;
    TupleValue * tvalue;
    gint i;

    if (infowin == NULL)
        create_infowin ();
    else
        clear_infowin ();

    current_file = g_strdup (filename);
    current_decoder = decoder;
    can_write = updating_enabled;

    set_entry_str_from_field (entry_title, tuple, FIELD_TITLE, updating_enabled);
    set_entry_str_from_field (entry_artist, tuple, FIELD_ARTIST,
     updating_enabled);
    set_entry_str_from_field (entry_album, tuple, FIELD_ALBUM, updating_enabled);
    set_entry_str_from_field (entry_comment, tuple, FIELD_COMMENT,
     updating_enabled);
    set_entry_str_from_field (gtk_bin_get_child ((GtkBin *) entry_genre), tuple,
     FIELD_GENRE, updating_enabled);

    tmp = g_strdup (filename);
    string_decode_percent (tmp);
    gtk_label_set_text ((GtkLabel *) location_text, easy_read_filename (tmp));
    g_free (tmp);

    set_entry_int_from_field (entry_year, tuple, FIELD_YEAR, updating_enabled);
    set_entry_int_from_field (entry_track, tuple, FIELD_TRACK_NUMBER,
     updating_enabled);

    infowin_label_set_text (label_format_name, tuple_get_string (tuple,
     FIELD_CODEC, NULL));
    infowin_label_set_text (label_quality, tuple_get_string (tuple,
     FIELD_QUALITY, NULL));

    if (tuple_get_value_type (tuple, FIELD_BITRATE, NULL) == TUPLE_INT)
    {
        tmp = g_strdup_printf (_("%d kb/s"), tuple_get_int (tuple,
         FIELD_BITRATE, NULL));
        infowin_label_set_text (label_bitrate, tmp);
        g_free (tmp);
    }
    else
        infowin_label_set_text (label_bitrate, NULL);

    string = tuple_get_string (tuple, FIELD_MIMETYPE, NULL);
    icon = mime_icon_lookup (48, string != NULL ? string : "audio/x-generic");

    if (icon != NULL)
    {
        gtk_image_set_from_pixbuf ((GtkImage *) image_fileicon, icon);
        g_object_unref (icon);
    }

    if (! set_image_from_album_art (filename, decoder))
    {
        tmp = aud_get_associated_image_file (filename);

        if (tmp != NULL)
        {
            infowin_entry_set_image (image_artwork, tmp);
            g_free (tmp);
        }
    }

    store = gtk_list_store_new (RAWDATA_N_COLS, G_TYPE_STRING, G_TYPE_STRING);

    for (i = 0; i < FIELD_LAST; i ++)
    {
        gchar * value;

        if (tuple->values[i] == NULL)
            continue;

        if (tuple->values[i]->type == TUPLE_INT)
            value = g_strdup_printf ("%d", tuple->values[i]->value.integer);
        else if (tuple->values[i]->value.string != NULL)
            value = g_strdup (tuple->values[i]->value.string);
        else
            continue;

        gtk_list_store_append (store, & iter);
        gtk_list_store_set (store, & iter, RAWDATA_KEY, tuple_fields[i].name,
         RAWDATA_VALUE, value, -1);
        g_free (value);
    }

    /* non-standard values are stored in a dictionary. */
    MOWGLI_DICTIONARY_FOREACH (tvalue, & state, tuple->dict)
    {
        gchar * value;

        if (tvalue->type == TUPLE_INT)
            value = g_strdup_printf ("%d", tvalue->value.integer);
        else if (tvalue->value.string != NULL)
            value = g_strdup (tvalue->value.string);
        else
            continue;

        gtk_list_store_append (store, & iter);
        gtk_list_store_set (store, & iter, RAWDATA_KEY, state.cur->key,
         RAWDATA_VALUE, value, -1);
        g_free (value);
    }

    gtk_tree_view_set_model ((GtkTreeView *) treeview_rawdata, (GtkTreeModel *)
     store);
    g_object_unref (store);

    gtk_window_present ((GtkWindow *) infowin);
}

void audgui_infowin_show (gint playlist, gint entry)
{
    const gchar * filename = aud_playlist_entry_get_filename (playlist, entry);
    InputPlugin * decoder = aud_playlist_entry_get_decoder (playlist, entry);
    Tuple * tuple = (Tuple *) aud_playlist_entry_get_tuple (playlist, entry);

    g_return_if_fail (filename != NULL);

    if (decoder == NULL)
    {
        decoder = aud_file_find_decoder (filename, FALSE);

        if (decoder == NULL)
            return;
    }

    if (aud_custom_infowin (filename, decoder))
        return;

    if (tuple == NULL)
    {
        tuple = aud_file_read_tuple (filename, decoder);

        if (tuple == NULL)
        {
            gchar * message = g_strdup_printf (_("No info available for %s.\n"),
             filename);

            aud_hook_call ("interface show error", message);
            g_free (message);
            return;
        }

        aud_playlist_entry_set_tuple (playlist, entry, tuple);
    }

    infowin_show (filename, tuple, decoder, aud_file_can_write_tuple (filename,
     decoder));
}

void audgui_infowin_show_current (void)
{
    gint playlist = aud_playlist_get_playing ();
    gint position;

    if (playlist == -1)
        playlist = aud_playlist_get_active ();

    position = aud_playlist_get_position (playlist);

    if (position == -1)
        return;

    audgui_infowin_show (playlist, position);
}
