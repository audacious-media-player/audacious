/*
 * infopopup.c
 * Copyright 2006-2012 William Pitcock, Giacomo Lozito, John Lindgren, and
 *                     Thomas Lange
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

#define AUD_GLIB_INTEGRATION
#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void infopopup_move_to_mouse (GtkWidget * infopopup);

static const GdkColor gray = {0, 40960, 40960, 40960};
static const GdkColor white = {0, 65535, 65535, 65535};

static struct {
    GtkWidget * title_header, * title_label;
    GtkWidget * artist_header, * artist_label;
    GtkWidget * album_header, * album_label;
    GtkWidget * genre_header, * genre_label;
    GtkWidget * year_header, * year_label;
    GtkWidget * track_header, * track_label;
    GtkWidget * length_header, * length_label;
    GtkWidget * image;
    GtkWidget * progress;
} widgets;

static String current_file;
static GtkWidget * infopopup_queued;

/* returns false if album art fetch was queued */
static bool infopopup_display_image (const char * filename)
{
    bool queued;
    AudguiPixbuf pb = audgui_pixbuf_request (filename, & queued);
    if (! pb)
        return ! queued;

    audgui_pixbuf_scale_within (pb, audgui_get_dpi ());
    gtk_image_set_from_pixbuf ((GtkImage *) widgets.image, pb.get ());
    gtk_widget_show (widgets.image);

    return true;
}

static void infopopup_art_ready (const char * filename)
{
    if (! infopopup_queued || strcmp (filename, current_file))
        return;

    infopopup_display_image (filename);
    audgui_show_unique_window (AUDGUI_INFOPOPUP_WINDOW, infopopup_queued);
    infopopup_queued = nullptr;
}

static void infopopup_progress_cb (void *)
{
    String filename;
    int length = 0, time = 0;

    if (aud_drct_get_playing ())
    {
        filename = aud_drct_get_filename ();
        length = aud_drct_get_length ();
        time = aud_drct_get_time ();
    }

    if (aud_get_bool (nullptr, "filepopup_showprogressbar") && filename &&
     current_file && ! strcmp (filename, current_file) && length > 0)
    {
        gtk_progress_bar_set_fraction ((GtkProgressBar *) widgets.progress, time / (float) length);
        gtk_progress_bar_set_text ((GtkProgressBar *) widgets.progress, str_format_time (time));
        gtk_widget_show (widgets.progress);
    }
    else
        gtk_widget_hide (widgets.progress);
}

static void infopopup_realized (GtkWidget * widget)
{
    GdkWindow * window = gtk_widget_get_window (widget);
    gdk_window_set_back_pixmap (window, nullptr, false);
    infopopup_move_to_mouse (widget);
}

/* borrowed from the gtkui infoarea */
static gboolean infopopup_draw_bg (GtkWidget * widget)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation (widget, & alloc);

    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));

    cairo_pattern_t * gradient = cairo_pattern_create_linear (0, 0, 0, alloc.height);
    cairo_pattern_add_color_stop_rgb (gradient, 0, 0.25, 0.25, 0.25);
    cairo_pattern_add_color_stop_rgb (gradient, 0.5, 0.15, 0.15, 0.15);
    cairo_pattern_add_color_stop_rgb (gradient, 0.5, 0.1, 0.1, 0.1);
    cairo_pattern_add_color_stop_rgb (gradient, 1, 0, 0, 0);

    cairo_set_source (cr, gradient);
    cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);

    cairo_pattern_destroy (gradient);
    cairo_destroy (cr);
    return false;
}

static void infopopup_add_category (GtkWidget * grid, int position,
 const char * text, GtkWidget * * header, GtkWidget * * label)
{
    * header = gtk_label_new (nullptr);
    * label = gtk_label_new (nullptr);

    gtk_misc_set_alignment ((GtkMisc *) * header, 1, 0.5);
    gtk_misc_set_alignment ((GtkMisc *) * label, 0, 0.5);

    gtk_widget_modify_fg (* header, GTK_STATE_NORMAL, & gray);
    gtk_widget_modify_fg (* label, GTK_STATE_NORMAL, & white);

    CharPtr markup (g_markup_printf_escaped ("<span style=\"italic\">%s</span>", text));
    gtk_label_set_markup ((GtkLabel *) * header, markup);

    gtk_table_attach ((GtkTable *) grid, * header, 0, 1, position, position + 1,
     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach ((GtkTable *) grid, * label, 1, 2, position, position + 1,
     GTK_FILL, GTK_FILL, 0, 0);

    gtk_widget_set_no_show_all (* header, true);
    gtk_widget_set_no_show_all (* label, true);
}

static void infopopup_destroyed ()
{
    hook_dissociate ("art ready", (HookFunction) infopopup_art_ready);

    timer_remove (TimerRate::Hz4, infopopup_progress_cb);

    memset (& widgets, 0, sizeof widgets);

    current_file = String ();
    infopopup_queued = nullptr;
}

static GtkWidget * infopopup_create ()
{
    int dpi = audgui_get_dpi ();

    GtkWidget * infopopup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_type_hint ((GtkWindow *) infopopup, GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_decorated ((GtkWindow *) infopopup, false);
    gtk_container_set_border_width ((GtkContainer *) infopopup, 4);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_container_add ((GtkContainer *) infopopup, hbox);

    widgets.image = gtk_image_new ();
    gtk_widget_set_size_request (widgets.image, dpi, dpi);
    gtk_box_pack_start ((GtkBox *) hbox, widgets.image, false, false, 0);
    gtk_widget_set_no_show_all (widgets.image, true);

    GtkWidget * grid = gtk_table_new (0, 0, false);
    gtk_table_set_col_spacings ((GtkTable *) grid, 6);
    gtk_box_pack_start ((GtkBox *) hbox, grid, true, true, 0);

    infopopup_add_category (grid, 0, _("Title"), & widgets.title_header, & widgets.title_label);
    infopopup_add_category (grid, 1, _("Artist"), & widgets.artist_header, & widgets.artist_label);
    infopopup_add_category (grid, 2, _("Album"), & widgets.album_header, & widgets.album_label);
    infopopup_add_category (grid, 3, _("Genre"), & widgets.genre_header, & widgets.genre_label);
    infopopup_add_category (grid, 4, _("Year"), & widgets.year_header, & widgets.year_label);
    infopopup_add_category (grid, 5, _("Track"), & widgets.track_header, & widgets.track_label);
    infopopup_add_category (grid, 6, _("Length"), & widgets.length_header, & widgets.length_label);

    /* track progress */
    widgets.progress = gtk_progress_bar_new ();
    gtk_progress_bar_set_text ((GtkProgressBar *) widgets.progress, "");
    gtk_table_set_row_spacing ((GtkTable *) grid, 6, 4);
    gtk_table_attach ((GtkTable *) grid, widgets.progress, 0, 2, 7, 8,
     GTK_FILL, GTK_FILL, 0, 0);

    /* do not show the track progress */
    gtk_widget_set_no_show_all (widgets.progress, true);

    /* override background drawing */
    gtk_widget_set_app_paintable (infopopup, true);

    GtkStyle * style = gtk_style_new ();
    gtk_widget_set_style (infopopup, style);
    g_object_unref (style);

    g_signal_connect (infopopup, "realize", (GCallback) infopopup_realized, nullptr);
    g_signal_connect (infopopup, "expose-event", (GCallback) infopopup_draw_bg, nullptr);

    return infopopup;
}

static void infopopup_set_field (GtkWidget * header, GtkWidget * label, const char * text)
{
    if (text)
    {
        gtk_label_set_text ((GtkLabel *) label, text);
        gtk_widget_show (header);
        gtk_widget_show (label);
    }
    else
    {
        gtk_widget_hide (header);
        gtk_widget_hide (label);
    }
}

static void infopopup_set_fields (const Tuple & tuple)
{
    String title = tuple.get_str (Tuple::Title);
    String artist = tuple.get_str (Tuple::Artist);
    String album = tuple.get_str (Tuple::Album);
    String genre = tuple.get_str (Tuple::Genre);

    infopopup_set_field (widgets.title_header, widgets.title_label, title);
    infopopup_set_field (widgets.artist_header, widgets.artist_label, artist);
    infopopup_set_field (widgets.album_header, widgets.album_label, album);
    infopopup_set_field (widgets.genre_header, widgets.genre_label, genre);

    int value = tuple.get_int (Tuple::Length);
    infopopup_set_field (widgets.length_header, widgets.length_label,
     (value > 0) ? (const char *) str_format_time (value) : nullptr);

    value = tuple.get_int (Tuple::Year);
    infopopup_set_field (widgets.year_header, widgets.year_label,
     (value > 0) ? (const char *) int_to_str (value) : nullptr);

    value = tuple.get_int (Tuple::Track);
    infopopup_set_field (widgets.track_header, widgets.track_label,
     (value > 0) ? (const char *) int_to_str (value) : nullptr);
}

static void infopopup_move_to_mouse (GtkWidget * infopopup)
{
    GdkScreen * screen = gtk_widget_get_screen (infopopup);
    GdkRectangle geom;
    int x, y, h, w;

    audgui_get_mouse_coords (screen, & x, & y);
    audgui_get_monitor_geometry (screen, x, y, & geom);
    gtk_window_get_size ((GtkWindow *) infopopup, & w, & h);

    /* If we show the popup right under the cursor, the underlying window gets
     * a "leave-notify-event" and immediately hides the popup again.  So, we
     * offset the popup slightly. */
    if (x + w > geom.x + geom.width)
        x -= w + 3;
    else
        x += 3;

    if (y + h > geom.y + geom.height)
        y -= h + 3;
    else
        y += 3;

    gtk_window_move ((GtkWindow *) infopopup, x, y);
}

static void infopopup_show (const char * filename, const Tuple & tuple)
{
    audgui_infopopup_hide ();

    current_file = String (filename);

    GtkWidget * infopopup = infopopup_create ();
    infopopup_set_fields (tuple);

    hook_associate ("art ready", (HookFunction) infopopup_art_ready, nullptr);

    g_signal_connect (infopopup, "destroy", (GCallback) infopopup_destroyed, nullptr);

    /* start a timer that updates a progress bar if the tooltip
       is shown for the song that is being currently played */
    timer_add (TimerRate::Hz4, infopopup_progress_cb);

    /* immediately run the callback once to update progressbar status */
    infopopup_progress_cb (nullptr);

    if (infopopup_display_image (filename))
        audgui_show_unique_window (AUDGUI_INFOPOPUP_WINDOW, infopopup);
    else
        infopopup_queued = infopopup;
}

EXPORT void audgui_infopopup_show (int playlist, int entry)
{
    String filename = aud_playlist_entry_get_filename (playlist, entry);
    Tuple tuple = aud_playlist_entry_get_tuple (playlist, entry);

    if (filename && tuple.valid ())
        infopopup_show (filename, tuple);
}

EXPORT void audgui_infopopup_show_current ()
{
    int playlist = aud_playlist_get_playing ();
    if (playlist < 0)
        playlist = aud_playlist_get_active ();

    int position = aud_playlist_get_position (playlist);
    if (position < 0)
        return;

    audgui_infopopup_show (playlist, position);
}

EXPORT void audgui_infopopup_hide ()
{
    audgui_hide_unique_window (AUDGUI_INFOPOPUP_WINDOW);

    if (infopopup_queued)
        gtk_widget_destroy (infopopup_queued);
}
