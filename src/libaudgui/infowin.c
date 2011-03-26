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

#include <audacious/audconfig.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

#define AUDGUI_STATUS_TIMEOUT 3000

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

static GtkWidget * label_format_name;
static GtkWidget * label_quality;
static GtkWidget * label_bitrate;
static GtkWidget * btn_apply;
static GtkWidget * label_mini_status;

enum
{
    RAWDATA_KEY,
    RAWDATA_VALUE,
    RAWDATA_N_COLS
};

static gchar * current_file = NULL;
static PluginHandle * current_decoder = NULL;
static gboolean can_write = FALSE, something_changed = FALSE;

/* This is by no means intended to be a complete list.  If it is not short, it
 * is useless: scrolling through ten pages of dropdown list is more work than
 * typing out the genre. */

static const gchar * genre_table[] = {
 N_("Acid Jazz"),
 N_("Acid Rock"),
 N_("Ambient"),
 N_("Bebop"),
 N_("Bluegrass"),
 N_("Blues"),
 N_("Chamber Music"),
 N_("Classical"),
 N_("Country"),
 N_("Death Metal"),
 N_("Disco"),
 N_("Easy Listening"),
 N_("Folk"),
 N_("Funk"),
 N_("Gangsta Rap"),
 N_("Gospel"),
 N_("Grunge"),
 N_("Hard Rock"),
 N_("Heavy Metal"),
 N_("Hip-hop"),
 N_("House"),
 N_("Jazz"),
 N_("Jungle"),
 N_("Metal"),
 N_("New Age"),
 N_("New Wave"),
 N_("Noise"),
 N_("Pop"),
 N_("Punk Rock"),
 N_("Rap"),
 N_("Reggae"),
 N_("Rock"),
 N_("Rock and Roll"),
 N_("Rhythm and Blues"),
 N_("Ska"),
 N_("Soul"),
 N_("Swing"),
 N_("Techno"),
 N_("Trip-hop")};

static void set_entry_str_from_field (GtkWidget * widget, const Tuple * tuple,
 gint fieldn, gboolean editable)
{
    const gchar * text = tuple_get_string (tuple, fieldn, NULL);

    gtk_entry_set_text ((GtkEntry *) widget, text != NULL ? text : "");
    gtk_editable_set_editable ((GtkEditable *) widget, editable);
}

static void set_entry_int_from_field (GtkWidget * widget, const Tuple * tuple,
 gint fieldn, gboolean editable)
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

static void infowin_entry_set_image (GtkWidget * widget, gint list, gint entry)
{
    GdkPixbuf * p = audgui_pixbuf_for_entry (list, entry);
    g_return_if_fail (p);

    audgui_pixbuf_scale_within (& p, aud_cfg->filepopup_pixelsize);
    gtk_image_set_from_pixbuf ((GtkImage *) widget, p);
    g_object_unref ((GObject *) p);
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
    gtk_image_clear ((GtkImage *) image_artwork);
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

    g_timeout_add (AUDGUI_STATUS_TIMEOUT, (GSourceFunc) ministatus_timeout_proc,
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

gboolean genre_fill (GtkWidget * combo)
{
    GList * list = NULL;
    GList * node;
    gint i;

    for (i = 0; i < G_N_ELEMENTS (genre_table); i ++)
        list = g_list_prepend (list, _(genre_table[i]));

    list = g_list_sort (list, (GCompareFunc) strcmp);

    for (node = list; node != NULL; node = node->next)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, node->data);

    g_list_free (list);
    return FALSE;
}

void create_infowin (void)
{
    GtkWidget * hbox;
    GtkWidget * hbox_status_and_bbox;
    GtkWidget * vbox0;
    GtkWidget * vbox2;
    GtkWidget * label_title;
    GtkWidget * label_artist;
    GtkWidget * label_album;
    GtkWidget * label_comment;
    GtkWidget * label_genre;
    GtkWidget * label_year;
    GtkWidget * label_track;
    GtkWidget * label_format;
    GtkWidget * label_quality_label;
    GtkWidget * label_bitrate_label;
    GtkWidget * codec_hbox;
    GtkWidget * codec_table;
    GtkWidget * table1;
    GtkWidget * bbox_close;
    GtkWidget * btn_close;
    GtkWidget * alignment;

    infowin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width ((GtkContainer *) infowin, 6);
    gtk_window_set_title ((GtkWindow *) infowin, _("Track Information"));
    gtk_window_set_type_hint ((GtkWindow *) infowin,
     GDK_WINDOW_TYPE_HINT_DIALOG);

    vbox0 = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) infowin, vbox0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox0, hbox, TRUE, TRUE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) hbox, vbox2, TRUE, TRUE, 0);

    image_artwork = gtk_image_new ();
    gtk_box_pack_start ((GtkBox *) vbox2, image_artwork, TRUE, TRUE, 0);

    location_text = gtk_label_new ("");
    gtk_widget_set_size_request (location_text, 200, -1);
    gtk_label_set_line_wrap ((GtkLabel *) location_text, TRUE);
    gtk_label_set_line_wrap_mode ((GtkLabel *) location_text,
     PANGO_WRAP_WORD_CHAR);
    gtk_label_set_selectable ((GtkLabel *) location_text, TRUE);
    gtk_box_pack_start ((GtkBox *) vbox2, location_text, FALSE, FALSE, 0);

    codec_hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox2, codec_hbox, FALSE, FALSE, 0);

    codec_table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings ((GtkTable *) codec_table, 3);
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

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start ((GtkBox *) hbox, vbox2, TRUE, TRUE, 0);

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

    entry_genre = gtk_combo_box_text_new_with_entry ();
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
    g_signal_connect_swapped (btn_close, "clicked", (GCallback) gtk_widget_hide,
     infowin);

    audgui_hide_on_delete (infowin);
    audgui_hide_on_escape (infowin);

    gtk_widget_show_all (vbox0);
    gtk_widget_grab_focus (entry_title);
}

static void infowin_show (gint list, gint entry, const gchar * filename,
 const Tuple * tuple, PluginHandle * decoder, gboolean updating_enabled)
{
    gchar * tmp;

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

    tmp = uri_to_display (filename);
    gtk_label_set_text ((GtkLabel *) location_text, tmp);
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

    infowin_entry_set_image (image_artwork, list, entry);

    gtk_window_present ((GtkWindow *) infowin);
}

void audgui_infowin_show (gint playlist, gint entry)
{
    const gchar * filename = aud_playlist_entry_get_filename (playlist, entry);
    g_return_if_fail (filename != NULL);

    PluginHandle * decoder = aud_playlist_entry_get_decoder (playlist, entry,
     FALSE);
    if (decoder == NULL)
        return;

    if (aud_custom_infowin (filename, decoder))
        return;

    const Tuple * tuple = aud_playlist_entry_get_tuple (playlist, entry, FALSE);

    if (tuple == NULL)
    {
        gchar * message = g_strdup_printf (_("No info available for %s.\n"),
         filename);
        hook_call ("interface show error", message);
        g_free (message);
        return;
    }

    infowin_show (playlist, entry, filename, tuple, decoder,
     aud_file_can_write_tuple (filename, decoder));
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
