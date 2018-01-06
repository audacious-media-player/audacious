/*
 * infowin.c
 * Copyright 2006-2013 William Pitcock, Tomasz Moń, Eugene Zagidullin,
 *                     John Lindgren, and Thomas Lange
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

#define AUDGUI_STATUS_TIMEOUT 3000

enum {
    CODEC_FORMAT,
    CODEC_QUALITY,
    CODEC_BITRATE,
    CODEC_ITEMS
};

static const char * codec_labels[CODEC_ITEMS] = {
    N_("Format:"),
    N_("Quality:"),
    N_("Bitrate:")
};

static struct {
    GtkWidget * location;
    GtkWidget * title;
    GtkWidget * artist;
    GtkWidget * album;
    GtkWidget * album_artist;
    GtkWidget * comment;
    GtkWidget * year;
    GtkWidget * track;
    GtkWidget * genre;
    GtkWidget * image;
    GtkWidget * codec[3];
    GtkWidget * apply;
    GtkWidget * clear;
    GtkWidget * ministatus;
} widgets;

static GtkWidget * infowin;
static Playlist current_playlist;
static int current_entry;
static String current_file;
static Tuple current_tuple;
static PluginHandle * current_decoder = nullptr;
static bool can_write = false;
static QueuedFunc ministatus_timer;

/* This is by no means intended to be a complete list.  If it is not short, it
 * is useless: scrolling through ten pages of dropdown list is more work than
 * typing out the genre. */

static const char * genre_table[] = {
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

static GtkWidget * small_label_new (const char * text)
{
    static PangoAttrList * attrs = nullptr;

    if (! attrs)
    {
        attrs = pango_attr_list_new ();
        pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_SMALL));
    }

    GtkWidget * label = gtk_label_new (text);
    gtk_label_set_attributes ((GtkLabel *) label, attrs);
    gtk_misc_set_alignment ((GtkMisc *) label, 0, 0.5);

    return label;
}

static void set_entry_str_from_field (GtkWidget * widget, const Tuple & tuple,
 Tuple::Field field, bool editable, bool clear)
{
    String text = tuple.get_str (field);
    if (! text && ! clear)
        return;

    gtk_entry_set_text ((GtkEntry *) widget, text ? text : "");
    gtk_editable_set_editable ((GtkEditable *) widget, editable);
}

static void set_entry_int_from_field (GtkWidget * widget, const Tuple & tuple,
 Tuple::Field field, bool editable, bool clear)
{
    int value = tuple.get_int (field);
    if (value <= 0 && ! clear)
        return;

    gtk_entry_set_text ((GtkEntry *) widget, (value > 0) ? (const char *) int_to_str (value) : "");
    gtk_editable_set_editable ((GtkEditable *) widget, editable);
}

static void set_field_str_from_entry (Tuple & tuple, Tuple::Field field, GtkWidget * widget)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) widget);

    if (text[0])
        tuple.set_str (field, text);
    else
        tuple.unset (field);
}

static void set_field_int_from_entry (Tuple & tuple, Tuple::Field field, GtkWidget * widget)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) widget);

    if (text[0])
        tuple.set_int (field, atoi (text));
    else
        tuple.unset (field);
}

static void entry_changed (GtkEditable * editable)
{
    if (can_write)
        gtk_widget_set_sensitive (widgets.apply, true);
}

static void ministatus_display_message (const char * text)
{
    gtk_label_set_text ((GtkLabel *) widgets.ministatus, text);
    gtk_widget_hide (widgets.clear);
    gtk_widget_show (widgets.ministatus);

    ministatus_timer.queue (AUDGUI_STATUS_TIMEOUT, [] (void *) {
        gtk_widget_hide (widgets.ministatus);
        gtk_widget_show (widgets.clear);
    }, nullptr);
}

static void infowin_update_tuple ()
{
    set_field_str_from_entry (current_tuple, Tuple::Title, widgets.title);
    set_field_str_from_entry (current_tuple, Tuple::Artist, widgets.artist);
    set_field_str_from_entry (current_tuple, Tuple::Album, widgets.album);
    set_field_str_from_entry (current_tuple, Tuple::AlbumArtist, widgets.album_artist);
    set_field_str_from_entry (current_tuple, Tuple::Comment, widgets.comment);
    set_field_str_from_entry (current_tuple, Tuple::Genre,
     gtk_bin_get_child ((GtkBin *) widgets.genre));
    set_field_int_from_entry (current_tuple, Tuple::Year, widgets.year);
    set_field_int_from_entry (current_tuple, Tuple::Track, widgets.track);

    if (aud_file_write_tuple (current_file, current_decoder, current_tuple))
    {
        ministatus_display_message (_("Save successful"));
        gtk_widget_set_sensitive (widgets.apply, false);
    }
    else
        ministatus_display_message (_("Save error"));
}

static void infowin_next ()
{
    int entry = current_entry + 1;

    if (entry < current_playlist.n_entries ())
    {
        current_playlist.select_all (false);
        current_playlist.select_entry (entry, true);
        current_playlist.set_focus (entry);
        audgui_infowin_show (current_playlist, entry);
    }
    else
        audgui_infowin_hide ();
}

static void genre_fill (GtkWidget * combo)
{
    GList * list = nullptr;
    GList * node;

    for (const char * genre : genre_table)
        list = g_list_prepend (list, _(genre));

    list = g_list_sort (list, (GCompareFunc) strcmp);

    for (node = list; node != nullptr; node = node->next)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, (const char *) node->data);

    g_list_free (list);
}

static void clear_toggled (GtkToggleButton * toggle)
{
    aud_set_bool ("audgui", "clear_song_fields", gtk_toggle_button_get_active (toggle));
}

static void infowin_display_image (const char * filename)
{
    if (! current_file || strcmp (filename, current_file))
        return;

    AudguiPixbuf pb = audgui_pixbuf_request (filename);
    if (! pb)
        pb = audgui_pixbuf_fallback ();

    if (pb)
        audgui_scaled_image_set (widgets.image, pb.get ());
}

static void infowin_destroyed ()
{
    hook_dissociate ("art ready", (HookFunction) infowin_display_image);

    ministatus_timer.stop ();

    memset (& widgets, 0, sizeof widgets);

    infowin = nullptr;
    current_file = String ();
    current_tuple = Tuple ();
    current_decoder = nullptr;
}

static void add_entry (GtkWidget * grid, const char * title, GtkWidget * entry,
 int x, int y, int span)
{
    GtkWidget * label = small_label_new (title);

    gtk_table_attach ((GtkTable *) grid, label, x, x + span, y, y + 1,
     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach ((GtkTable *) grid, entry, x, x + span, y + 1, y + 2,
     GTK_FILL, GTK_FILL, 0, 0);

    g_signal_connect (entry, "changed", (GCallback) entry_changed, nullptr);
}

static void create_infowin ()
{
    int dpi = audgui_get_dpi ();

    infowin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width ((GtkContainer *) infowin, 6);
    gtk_window_set_title ((GtkWindow *) infowin, _("Song Info"));
    gtk_window_set_type_hint ((GtkWindow *) infowin,
     GDK_WINDOW_TYPE_HINT_DIALOG);

    GtkWidget * main_grid = gtk_table_new (0, 0, false);
    gtk_table_set_col_spacings ((GtkTable *) main_grid, 6);
    gtk_table_set_row_spacings ((GtkTable *) main_grid, 6);
    gtk_container_add ((GtkContainer *) infowin, main_grid);

    widgets.image = audgui_scaled_image_new (nullptr);
    gtk_table_attach_defaults ((GtkTable *) main_grid, widgets.image, 0, 1, 0, 1);

    widgets.location = gtk_label_new ("");
    gtk_widget_set_size_request (widgets.location, 2 * dpi, -1);
    gtk_label_set_line_wrap ((GtkLabel *) widgets.location, true);
    gtk_label_set_line_wrap_mode ((GtkLabel *) widgets.location, PANGO_WRAP_WORD_CHAR);
    gtk_label_set_selectable ((GtkLabel *) widgets.location, true);
    gtk_table_attach ((GtkTable *) main_grid, widgets.location, 0, 1, 1, 2,
     GTK_FILL, GTK_FILL, 0, 0);

    GtkWidget * codec_grid = gtk_table_new (0, 0, false);
    gtk_table_set_row_spacings ((GtkTable *) codec_grid, 2);
    gtk_table_set_col_spacings ((GtkTable *) codec_grid, 12);
    gtk_table_attach ((GtkTable *) main_grid, codec_grid, 0, 1, 2, 3,
     GTK_FILL, GTK_FILL, 0, 0);

    for (int row = 0; row < CODEC_ITEMS; row ++)
    {
        GtkWidget * label = small_label_new (_(codec_labels[row]));
        gtk_table_attach ((GtkTable *) codec_grid, label, 0, 1, row, row + 1,
         GTK_FILL, GTK_FILL, 0, 0);

        widgets.codec[row] = small_label_new (nullptr);
        gtk_table_attach ((GtkTable *) codec_grid, widgets.codec[row], 1, 2, row, row + 1,
         GTK_FILL, GTK_FILL, 0, 0);
    }

    GtkWidget * grid = gtk_table_new (0, 0, false);
    gtk_table_set_row_spacings ((GtkTable *) grid, 2);
    gtk_table_set_col_spacings ((GtkTable *) grid, 6);
    gtk_table_attach ((GtkTable *) main_grid, grid, 1, 2, 0, 3,
     GTK_FILL, GTK_FILL, 0, 0);

    widgets.title = gtk_entry_new ();
    gtk_widget_set_size_request (widgets.title, 3 * dpi, -1);
    add_entry (grid, _("Title"), widgets.title, 0, 0, 2);

    widgets.artist = gtk_entry_new ();
    add_entry (grid, _("Artist"), widgets.artist, 0, 2, 2);

    widgets.album = gtk_entry_new ();
    add_entry (grid, _("Album"), widgets.album, 0, 4, 2);

    widgets.album_artist = gtk_entry_new ();
    add_entry (grid, _("Album Artist"), widgets.album_artist, 0, 6, 2);

    widgets.comment = gtk_entry_new ();
    add_entry (grid, _("Comment"), widgets.comment, 0, 8, 2);

    widgets.genre = gtk_combo_box_text_new_with_entry ();
    genre_fill (widgets.genre);
    add_entry (grid, _("Genre"), widgets.genre, 0, 10, 2);

    widgets.year = gtk_entry_new ();
    add_entry (grid, _("Year"), widgets.year, 0, 12, 1);

    widgets.track = gtk_entry_new ();
    add_entry (grid, _("Track Number"), widgets.track, 1, 12, 1);

    GtkWidget * bottom_hbox = gtk_hbox_new (false, 6);
    gtk_table_attach ((GtkTable *) main_grid, bottom_hbox, 0, 2, 3, 4,
     GTK_FILL, GTK_FILL, 0, 0);

    widgets.clear = gtk_check_button_new_with_mnemonic
     (_("Clea_r fields when moving to next song"));

    gtk_toggle_button_set_active ((GtkToggleButton *) widgets.clear,
     aud_get_bool ("audgui", "clear_song_fields"));
    g_signal_connect (widgets.clear, "toggled", (GCallback) clear_toggled, nullptr);

    gtk_widget_set_no_show_all (widgets.clear, true);
    gtk_widget_show (widgets.clear);
    gtk_box_pack_start ((GtkBox *) bottom_hbox, widgets.clear, false, false, 0);

    widgets.ministatus = small_label_new (nullptr);
    gtk_widget_set_no_show_all (widgets.ministatus, true);
    gtk_box_pack_start ((GtkBox *) bottom_hbox, widgets.ministatus, true, true, 0);

    widgets.apply = audgui_button_new (_("_Save"), "document-save",
     (AudguiCallback) infowin_update_tuple, nullptr);

    GtkWidget * close_button = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) audgui_infowin_hide, nullptr);

    GtkWidget * next_button = audgui_button_new (_("_Next"), "go-next",
     (AudguiCallback) infowin_next, nullptr);

    gtk_box_pack_end ((GtkBox *) bottom_hbox, close_button, false, false, 0);
    gtk_box_pack_end ((GtkBox *) bottom_hbox, next_button, false, false, 0);
    gtk_box_pack_end ((GtkBox *) bottom_hbox, widgets.apply, false, false, 0);

    audgui_destroy_on_escape (infowin);
    g_signal_connect (infowin, "destroy", (GCallback) infowin_destroyed, nullptr);

    hook_associate ("art ready", (HookFunction) infowin_display_image, nullptr);
}

static void infowin_show (Playlist list, int entry, const String & filename,
 const Tuple & tuple, PluginHandle * decoder, bool writable)
{
    if (! infowin)
        create_infowin ();

    current_playlist = list;
    current_entry = entry;
    current_file = filename;
    current_tuple = tuple.ref ();
    current_decoder = decoder;
    can_write = writable;

    bool clear = aud_get_bool ("audgui", "clear_song_fields");

    set_entry_str_from_field (widgets.title, tuple, Tuple::Title, writable, clear);
    set_entry_str_from_field (widgets.artist, tuple, Tuple::Artist, writable, clear);
    set_entry_str_from_field (widgets.album, tuple, Tuple::Album, writable, clear);
    set_entry_str_from_field (widgets.album_artist, tuple, Tuple::AlbumArtist, writable, clear);
    set_entry_str_from_field (widgets.comment, tuple, Tuple::Comment, writable, clear);
    set_entry_str_from_field (gtk_bin_get_child ((GtkBin *) widgets.genre),
     tuple, Tuple::Genre, writable, clear);

    gtk_label_set_text ((GtkLabel *) widgets.location, uri_to_display (filename));

    set_entry_int_from_field (widgets.year, tuple, Tuple::Year, writable, clear);
    set_entry_int_from_field (widgets.track, tuple, Tuple::Track, writable, clear);

    String codec_values[CODEC_ITEMS];

    codec_values[CODEC_FORMAT] = tuple.get_str (Tuple::Codec);
    codec_values[CODEC_QUALITY] = tuple.get_str (Tuple::Quality);

    if (tuple.get_value_type (Tuple::Bitrate) == Tuple::Int)
        codec_values[CODEC_BITRATE] = String (str_printf (_("%d kb/s"),
         tuple.get_int (Tuple::Bitrate)));

    for (int row = 0; row < CODEC_ITEMS; row ++)
    {
        const char * text = codec_values[row] ? (const char *) codec_values[row] : _("N/A");
        gtk_label_set_text ((GtkLabel *) widgets.codec[row], text);
    }

    infowin_display_image (filename);

    /* nothing has been changed yet */
    gtk_widget_set_sensitive (widgets.apply, false);

    gtk_widget_grab_focus (widgets.title);

    if (! audgui_reshow_unique_window (AUDGUI_INFO_WINDOW))
        audgui_show_unique_window (AUDGUI_INFO_WINDOW, infowin);
}

EXPORT void audgui_infowin_show (Playlist playlist, int entry)
{
    String filename = playlist.entry_filename (entry);
    g_return_if_fail (filename != nullptr);

    String error;
    PluginHandle * decoder = playlist.entry_decoder (entry, Playlist::Wait, & error);
    Tuple tuple = decoder ? playlist.entry_tuple (entry, Playlist::Wait, & error) : Tuple ();

    if (decoder && tuple.valid () && ! aud_custom_infowin (filename, decoder))
    {
        /* cuesheet entries cannot be updated */
        bool can_write = aud_file_can_write_tuple (filename, decoder) &&
         ! tuple.is_set (Tuple::StartTime);

        tuple.delete_fallbacks ();
        infowin_show (playlist, entry, filename, tuple, decoder, can_write);
    }
    else
        audgui_infowin_hide ();

    if (error)
        aud_ui_show_error (str_printf (_("Error opening %s:\n%s"),
         (const char *) filename, (const char *) error));
}

EXPORT void audgui_infowin_show_current ()
{
    auto playlist = Playlist::playing_playlist ();
    if (playlist == Playlist ())
        playlist = Playlist::active_playlist ();

    int position = playlist.get_position ();
    if (position < 0)
        return;

    audgui_infowin_show (playlist, position);
}

EXPORT void audgui_infowin_hide ()
{
    audgui_hide_unique_window (AUDGUI_INFO_WINDOW);
}
