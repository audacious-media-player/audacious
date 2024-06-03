/*
 * infopopup.c
 * Copyright 2006-2012 Ariadne Conill, Giacomo Lozito, John Lindgren, and
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

#include "gtk-compat.h"
#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void infopopup_move_to_mouse (GtkWidget * infopopup);

static const char * gray = "#a0a0a0";
static const char * white = "#ffffff";

static struct {
    GtkWidget * title_header, * title_label;
    GtkWidget * artist_header, * artist_label;
    GtkWidget * album_header, * album_label;
    GtkWidget * genre_header, * genre_label;
    GtkWidget * year_header, * year_label;
    GtkWidget * track_header, * track_label;
    GtkWidget * disc_header, * disc_label;
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

    if (aud_get_bool ("filepopup_showprogressbar") && filename &&
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
#ifndef USE_GTK3
    GdkWindow * window = gtk_widget_get_window (widget);
    gdk_window_set_back_pixmap (window, nullptr, false);
#endif
    infopopup_move_to_mouse (widget);
}

#ifdef USE_GTK3
static gboolean infopopup_draw_bg (GtkWidget * widget, cairo_t * cr)
{
#else
static gboolean infopopup_draw_bg (GtkWidget * widget)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));
#endif
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    auto & c = (gtk_widget_get_style (widget))->base[GTK_STATE_NORMAL];
G_GNUC_END_IGNORE_DEPRECATIONS

    GtkAllocation alloc;
    gtk_widget_get_allocation (widget, & alloc);

    cairo_pattern_t * gradient = audgui_dark_bg_gradient (c, alloc.height);
    cairo_set_source (cr, gradient);
    cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);
    cairo_pattern_destroy (gradient);
#ifndef USE_GTK3
    cairo_destroy (cr);
#endif
    return false;
}

static void infopopup_set_markup (GtkWidget * label,
 const char * text, const char * color, bool italic)
{
    CharPtr markup (g_markup_printf_escaped (
     italic ? "<span color=\"%s\" style=\"italic\">%s</span>"
            : "<span color=\"%s\">%s</span>", color, text));
    gtk_label_set_markup ((GtkLabel *) label, markup);
}

static void infopopup_add_category (GtkWidget * grid, int position,
 const char * text, GtkWidget * * header, GtkWidget * * label)
{
    * header = gtk_label_new (nullptr);
    * label = gtk_label_new (nullptr);

    infopopup_set_markup (* header, text, gray, true);

#ifdef USE_GTK3
    gtk_widget_set_halign (* header, GTK_ALIGN_END);
    gtk_widget_set_halign (* label, GTK_ALIGN_START);

    gtk_grid_attach ((GtkGrid *) grid, * header, 0, position, 1, 1);
    gtk_grid_attach ((GtkGrid *) grid, * label, 1, position, 1, 1);
#else
    gtk_misc_set_alignment ((GtkMisc *) * header, 1, 0.5);
    gtk_misc_set_alignment ((GtkMisc *) * label, 0, 0.5);

    gtk_table_attach ((GtkTable *) grid, * header, 0, 1, position, position + 1,
     GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach ((GtkTable *) grid, * label, 1, 2, position, position + 1,
     GTK_FILL, GTK_FILL, 0, 0);
#endif

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

static GtkWidget * infopopup_create (GtkWindow * parent)
{
    int dpi = audgui_get_dpi ();

    GtkWidget * infopopup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_type_hint ((GtkWindow *) infopopup, GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_decorated ((GtkWindow *) infopopup, false);
    gtk_window_set_role ((GtkWindow *) infopopup, "infopopup");
    gtk_container_set_border_width ((GtkContainer *) infopopup, 4);

    if (parent)
        gtk_window_set_transient_for ((GtkWindow *) infopopup, parent);

    GtkWidget * hbox = audgui_hbox_new (6);
    gtk_container_add ((GtkContainer *) infopopup, hbox);

    widgets.image = gtk_image_new ();
    gtk_widget_set_size_request (widgets.image, dpi, dpi);
    gtk_box_pack_start ((GtkBox *) hbox, widgets.image, false, false, 0);
    gtk_widget_set_no_show_all (widgets.image, true);

    GtkWidget * grid = audgui_grid_new ();
    audgui_grid_set_column_spacing (grid, 6);
    gtk_box_pack_start ((GtkBox *) hbox, grid, true, true, 0);

    infopopup_add_category (grid, 0, _("Title"), & widgets.title_header, & widgets.title_label);
    infopopup_add_category (grid, 1, _("Artist"), & widgets.artist_header, & widgets.artist_label);
    infopopup_add_category (grid, 2, _("Album"), & widgets.album_header, & widgets.album_label);
    infopopup_add_category (grid, 3, _("Genre"), & widgets.genre_header, & widgets.genre_label);
    infopopup_add_category (grid, 4, _("Year"), & widgets.year_header, & widgets.year_label);
    infopopup_add_category (grid, 5, _("Track"), & widgets.track_header, & widgets.track_label);
    infopopup_add_category (grid, 6, _("Disc"), & widgets.disc_header, & widgets.disc_label);
    infopopup_add_category (grid, 7, _("Length"), & widgets.length_header, & widgets.length_label);

    /* track progress */
    widgets.progress = gtk_progress_bar_new ();
    gtk_progress_bar_set_text ((GtkProgressBar *) widgets.progress, "");

#ifdef USE_GTK3
    gtk_widget_set_margin_top (widgets.progress, 4);
    gtk_grid_attach ((GtkGrid *) grid, widgets.progress, 0, 8, 2, 1);
#else
    gtk_table_set_row_spacing ((GtkTable *) grid, 7, 4);
    gtk_table_attach ((GtkTable *) grid, widgets.progress, 0, 2, 7, 8,
     GTK_FILL, GTK_FILL, 0, 0);
#endif

    /* override background drawing */
    gtk_widget_set_app_paintable (infopopup, true);

    g_signal_connect (infopopup, AUDGUI_DRAW_SIGNAL, (GCallback) infopopup_draw_bg, nullptr);
    g_signal_connect (infopopup, "realize", (GCallback) infopopup_realized, nullptr);

    /* do not show the track progress */
    gtk_widget_set_no_show_all (widgets.progress, true);

    return infopopup;
}

static void infopopup_set_field (GtkWidget * header, GtkWidget * label, const char * text)
{
    if (text)
    {
        infopopup_set_markup (label, text, white, false);
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

    value = tuple.get_int (Tuple::Disc);
    infopopup_set_field (widgets.disc_header, widgets.disc_label,
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

static void infopopup_show (GtkWindow * parent, const char * filename, const Tuple & tuple)
{
    audgui_infopopup_hide ();

    current_file = String (filename);

    GtkWidget * infopopup = infopopup_create (parent);
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

EXPORT void audgui_infopopup_show (GtkWindow * parent, Playlist playlist, int entry)
{
    String filename = playlist.entry_filename (entry);
    Tuple tuple = playlist.entry_tuple (entry);

    if (filename && tuple.valid ())
        infopopup_show (parent, filename, tuple);
}

EXPORT void audgui_infopopup_show (Playlist playlist, int entry)
{
    audgui_infopopup_show (nullptr, playlist, entry);
}

EXPORT void audgui_infopopup_show_current (GtkWindow * parent)
{
    auto playlist = Playlist::playing_playlist ();
    if (playlist == Playlist ())
        playlist = Playlist::active_playlist ();

    int position = playlist.get_position ();
    if (position < 0)
        return;

    audgui_infopopup_show (parent, playlist, position);
}

EXPORT void audgui_infopopup_show_current ()
{
    audgui_infopopup_show_current (nullptr);
}

EXPORT void audgui_infopopup_hide ()
{
    audgui_hide_unique_window (AUDGUI_INFOPOPUP_WINDOW);

    if (infopopup_queued)
        gtk_widget_destroy (infopopup_queued);
}
