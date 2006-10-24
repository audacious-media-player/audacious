/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 *  A note about Pango and some funky spacey fonts: Weirdly baselined
 *  fonts, or fonts with weird ascents or descents _will_ display a
 *  little bit weird in the playlist widget, but the display engine
 *  won't make it look too bad, just a little deranged.  I honestly
 *  don't think it's worth fixing (around...), it doesn't have to be
 *  perfectly fitting, just the general look has to be ok, which it
 *  IMHO is.
 *
 *  A second note: The numbers aren't perfectly aligned, but in the
 *  end it looks better when using a single Pango layout for each
 *  number.
 */

#include "widgetcore.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "util.h"

#include "debug.h"

static PangoFontDescription *playlist_list_font = NULL;
static gint ascent, descent, width_delta_digit_one;
static gboolean has_slant;
static guint padding;

/* FIXME: the following globals should not be needed. */
static gint width_approx_letters;
static gint width_colon, width_colon_third;
static gint width_approx_digits, width_approx_digits_half;

GdkPixmap *rootpix;

void playlist_list_draw(Widget * w);

/* Sort of stolen from XChat, but not really, as theres uses Xlib */
static void
shade_gdkimage_generic (GdkVisual *visual, GdkImage *ximg, int bpl, int w, int h, int rm, int gm, int bm, int bg)
{
	int x, y;
	int bgr = (256 - rm) * (bg & visual->red_mask);
	int bgg = (256 - gm) * (bg & visual->green_mask);
	int bgb = (256 - bm) * (bg & visual->blue_mask);

	for (x = 0; x < w; x++)
	{
		for (y = 0; y < h; y++)
		{
			unsigned long pixel = gdk_image_get_pixel (ximg, x, y);
			int r, g, b;

			r = rm * (pixel & visual->red_mask) + bgr;
			g = gm * (pixel & visual->green_mask) + bgg;
			b = bm * (pixel & visual->blue_mask) + bgb;

			gdk_image_put_pixel (ximg, x, y,
				((r >> 8) & visual->red_mask) |
				((g >> 8) & visual->green_mask) |
				((b >> 8) & visual->blue_mask));
		}
	}
}

/* and this is definately mine... -nenolod */
GdkPixmap *
shade_pixmap(GdkPixmap *in, gint x, gint y, gint x_offset, gint y_offset, gint w, gint h, GdkColor *shade_color)
{
	GdkImage *ximg;
	GdkPixmap *p;
	GdkGC *gc;

	g_return_val_if_fail(in != NULL, NULL);

	p = gdk_pixmap_new(in, w, h, -1);
	gc = gdk_gc_new(p);

        gdk_draw_pixmap(p, gc, in, x, y, 0, 0, w, h);

	gdk_error_trap_push();

	ximg = gdk_drawable_copy_to_image(in, NULL, x, y, 0, 0, w, h);	/* copy */

	gdk_error_trap_pop();

	if (GDK_IS_IMAGE(ximg))
	{
		shade_gdkimage_generic(gdk_drawable_get_visual(GDK_WINDOW(playlistwin->window)),
			ximg, ximg->bpl, w, h, 60, 60, 60, shade_color->pixel);

		gdk_draw_image(p, gc, ximg, 0, 0, x, y, w, h);
	}
	else {
		cfg.playlist_transparent = FALSE;
	}

	g_object_unref(in);
	g_object_unref(gc);

	return p;
}

#ifdef GDK_WINDOWING_X11

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

GdkDrawable *get_transparency_pixmap(void)
{
	GdkDrawable *root;
	XID *pixmaps;
	GdkAtom prop_type;
	gint prop_size;
	GdkPixmap *pixmap;
	gboolean ret;

	root = gdk_get_default_root_window();

	pixmap = NULL;
	pixmaps = NULL;

	gdk_error_trap_push();

	ret = gdk_property_get(root, gdk_atom_intern("_XROOTPMAP_ID", TRUE),
			0, 0, INT_MAX - 3,
			FALSE,
			&prop_type, NULL, &prop_size,
			(guchar **) &pixmaps);

	gdk_error_trap_pop();

	if ((ret == TRUE) && (prop_type == GDK_TARGET_PIXMAP) && (prop_size >= sizeof(XID)) && (pixmaps != NULL))
	{
		pixmap = gdk_pixmap_foreign_new_for_display(gdk_drawable_get_display(root),
			pixmaps[0]);

		if (pixmaps != NULL)
			g_free(pixmaps);
	}

	return GDK_DRAWABLE(pixmap);
}

static GdkFilterReturn
root_event_cb (GdkXEvent *xev, GdkEventProperty *event, gpointer data)
{
        static Atom at = None;
        XEvent *xevent = (XEvent *)xev;

        if (xevent->type == PropertyNotify)
        {
                if (at == None)
                        at = XInternAtom (xevent->xproperty.display, "_XROOTPMAP_ID", True);

                if (at == xevent->xproperty.atom)
		{
                        rootpix = shade_pixmap(get_transparency_pixmap(), 0, 0, 0, 0, gdk_screen_width(), gdk_screen_height(),
                            skin_get_color(bmp_active_skin, SKIN_PLEDIT_NORMALBG));

			if (cfg.playlist_transparent)
			{
				playlistwin_update_list();
				draw_playlist_window(TRUE);
			}
		}
        }

        return GDK_FILTER_CONTINUE;
}

#else

GdkPixmap *get_transparency_pixmap(void)
{
    return NULL;
}

#endif

static gboolean
playlist_list_auto_drag_down_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_down) {
        playlist_list_move_down(pl);
        pl->pl_first++;
        playlistwin_update_list();
        return TRUE;
    }
    return FALSE;
}

static gboolean
playlist_list_auto_drag_up_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_up) {
        playlist_list_move_up(pl);
        pl->pl_first--;
        playlistwin_update_list();
        return TRUE;

    }
    return FALSE;
}

void
playlist_list_move_up(PlayList_List * pl)
{
    GList *list;

    PLAYLIST_LOCK();
    if ((list = playlist_get()) == NULL) {
        PLAYLIST_UNLOCK();
        return;
    }
    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the top */
        PLAYLIST_UNLOCK();
        return;
    }
    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_moveup(list);
        list = g_list_next(list);
    }
    PLAYLIST_UNLOCK();
    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected--;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min--;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max--;
}

void
playlist_list_move_down(PlayList_List * pl)
{
    GList *list;

    PLAYLIST_LOCK();

    if (!(list = g_list_last(playlist_get()))) {
        PLAYLIST_UNLOCK();
        return;
    }

    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the bottom */
        PLAYLIST_UNLOCK();
        return;
    }

    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_movedown(list);
        list = g_list_previous(list);
    }

    PLAYLIST_UNLOCK();

    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected++;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min++;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max++;
}

static void
playlist_list_button_press_cb(GtkWidget * widget,
                              GdkEventButton * event,
                              PlayList_List * pl)
{
    gint nr, y;

    if (event->button == 1 && pl->pl_fheight &&
        widget_contains(&pl->pl_widget, event->x, event->y)) {

        y = event->y - pl->pl_widget.y;
        nr = (y / pl->pl_fheight) + pl->pl_first;

        if (nr >= playlist_get_length())
            nr = playlist_get_length() - 1;

        if (!(event->state & GDK_CONTROL_MASK))
            playlist_select_all(FALSE);

        if (event->state & GDK_SHIFT_MASK && pl->pl_prev_selected != -1) {
            playlist_select_range(pl->pl_prev_selected, nr, TRUE);
            pl->pl_prev_min = pl->pl_prev_selected;
            pl->pl_prev_max = nr;
            pl->pl_drag_pos = nr - pl->pl_first;
        }
        else {
            if (playlist_select_invert(nr)) {
                if (event->state & GDK_CONTROL_MASK) {
                    if (pl->pl_prev_min == -1) {
                        pl->pl_prev_min = pl->pl_prev_selected;
                        pl->pl_prev_max = pl->pl_prev_selected;
                    }
                    if (nr < pl->pl_prev_min)
                        pl->pl_prev_min = nr;
                    else if (nr > pl->pl_prev_max)
                        pl->pl_prev_max = nr;
                }
                else
                    pl->pl_prev_min = -1;
                pl->pl_prev_selected = nr;
                pl->pl_drag_pos = nr - pl->pl_first;
            }
        }
        if (event->type == GDK_2BUTTON_PRESS) {
            /*
             * Ungrab the pointer to prevent us from
             * hanging on to it during the sometimes slow
             * bmp_playback_initiate().
             */
            gdk_pointer_ungrab(GDK_CURRENT_TIME);
            gdk_flush();
            playlist_set_position(nr);
            if (!bmp_playback_get_playing())
                bmp_playback_initiate();
        }

        pl->pl_dragging = TRUE;
        playlistwin_update_list();
    }
}

gint
playlist_list_get_playlist_position(PlayList_List * pl,
                                    gint x,
                                    gint y)
{
    gint iy, length;
    gint ret;

    if (!widget_contains(WIDGET(pl), x, y) || !pl->pl_fheight)
        return -1;

    if ((length = playlist_get_length()) == 0)
        return -1;
    iy = y - pl->pl_widget.y;

    ret = (iy / pl->pl_fheight) + pl->pl_first;

    if(ret > length-1)
	    ret = -1;

    return ret;
}

static void
playlist_list_motion_cb(GtkWidget * widget,
                        GdkEventMotion * event,
                        PlayList_List * pl)
{
    gint nr, y, off, i;

    if (pl->pl_dragging) {
        y = event->y - pl->pl_widget.y;
        nr = (y / pl->pl_fheight);
        if (nr < 0) {
            nr = 0;
            if (!pl->pl_auto_drag_up) {
                pl->pl_auto_drag_up = TRUE;
                pl->pl_auto_drag_up_tag =
                    gtk_timeout_add(100, playlist_list_auto_drag_up_func, pl);
            }
        }
        else if (pl->pl_auto_drag_up)
            pl->pl_auto_drag_up = FALSE;

        if (nr >= pl->pl_num_visible) {
            nr = pl->pl_num_visible - 1;
            if (!pl->pl_auto_drag_down) {
                pl->pl_auto_drag_down = TRUE;
                pl->pl_auto_drag_down_tag =
                    gtk_timeout_add(100, playlist_list_auto_drag_down_func,
                                    pl);
            }
        }
        else if (pl->pl_auto_drag_down)
            pl->pl_auto_drag_down = FALSE;

        off = nr - pl->pl_drag_pos;
        if (off) {
            for (i = 0; i < abs(off); i++) {
                if (off < 0)
                    playlist_list_move_up(pl);
                else
                    playlist_list_move_down(pl);

            }
            playlistwin_update_list();
        }
        pl->pl_drag_pos = nr;
    }
}

static void
playlist_list_button_release_cb(GtkWidget * widget,
                                GdkEventButton * event,
                                PlayList_List * pl)
{
    pl->pl_dragging = FALSE;
    pl->pl_auto_drag_down = FALSE;
    pl->pl_auto_drag_up = FALSE;
}

static void
playlist_list_draw_string(PlayList_List * pl,
                          PangoFontDescription * font,
                          gint line,
                          gint width,
                          const gchar * text,
                          guint ppos)
{
    guint plist_length_int;

    PangoLayout *layout;

    REQUIRE_STATIC_LOCK(playlist);

    if (cfg.show_numbers_in_pl) {
        gchar *pos_string = g_strdup_printf(cfg.show_separator_in_pl == TRUE ? "%d" : "%d.", ppos);
        plist_length_int =
            gint_count_digits(playlist_get_length_nolock()) + !cfg.show_separator_in_pl + 1; /* cf.show_separator_in_pl will be 0 if false */

        padding = plist_length_int;
        padding = ((padding + 1) * width_approx_digits);

        layout = gtk_widget_create_pango_layout(playlistwin, pos_string);
        pango_layout_set_font_description(layout, playlist_list_font);
        pango_layout_set_width(layout, plist_length_int * 100);

        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                        pl->pl_widget.x +
                        (width_approx_digits *
                         (-1 + plist_length_int - strlen(pos_string))) +
                        (width_approx_digits / 4),
                        pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                        ascent + abs(descent), layout);
        g_free(pos_string);
        g_object_unref(layout);

        if (!cfg.show_separator_in_pl)
            padding -= (width_approx_digits * 1.5);
    }
    else {
        padding = 3;
    }

    width -= padding;

    layout = gtk_widget_create_pango_layout(playlistwin, text);

    pango_layout_set_font_description(layout, playlist_list_font);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                    pl->pl_widget.x + padding + (width_approx_letters / 4),
                    pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                    ascent + abs(descent), layout);

    g_object_unref(layout);
}

void
playlist_list_draw(Widget * w)
{
    PlayList_List *pl = PLAYLIST_LIST(w);
    GList *list;
    GdkGC *gc;
    GdkPixmap *obj;
    PangoLayout *layout;
    gchar *title;
    gint width, height;
    gint i, max_first;
    guint padding, padding_dwidth, padding_plength;
    guint max_time_len = 0;
    gfloat queue_tailpadding = 0;
    gint tpadding; 
    gsize tpadding_dwidth = 0;
    gint x, y;
    guint tail_width;
    guint tail_len;

    gchar tail[100];
    gchar queuepos[255];
    gchar length[40];

    gchar **frags;
    gchar *frag0;

    gint plw_w, plw_h;

    GdkRectangle *playlist_rect;

    gc = pl->pl_widget.gc;

    width = pl->pl_widget.width;
    height = pl->pl_widget.height;

    obj = pl->pl_widget.parent;

    gtk_window_get_size(GTK_WINDOW(playlistwin), &plw_w, &plw_h);

    playlist_rect = g_new0(GdkRectangle, 1);

    playlist_rect->x = 0;
    playlist_rect->y = 0;
    playlist_rect->width = plw_w - 17;
    playlist_rect->height = plw_h - 36;

    gdk_gc_set_clip_origin(gc, 31, 58);
    gdk_gc_set_clip_rectangle(gc, playlist_rect);

    if (cfg.playlist_transparent == FALSE)
    {
        gdk_gc_set_foreground(gc,
                              skin_get_color(bmp_active_skin,
                                             SKIN_PLEDIT_NORMALBG));
        gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x, pl->pl_widget.y,
                              width, height);
    }
    else
    {
	if (!rootpix)
           rootpix = shade_pixmap(get_transparency_pixmap(), 0, 0, 0, 0, gdk_screen_width(), gdk_screen_height(), 
    			    skin_get_color(bmp_active_skin, SKIN_PLEDIT_NORMALBG));
        gdk_draw_pixmap(obj, gc, rootpix, cfg.playlist_x + pl->pl_widget.x,
                    cfg.playlist_y + pl->pl_widget.y, pl->pl_widget.x, pl->pl_widget.y,
                    width, height);
    }

    if (!playlist_list_font) {
        g_critical("Couldn't open playlist font");
        return;
    }

    pl->pl_fheight = (ascent + abs(descent));
    pl->pl_num_visible = height / pl->pl_fheight;

    max_first = playlist_get_length() - pl->pl_num_visible;
    max_first = MAX(max_first, 0);

    pl->pl_first = CLAMP(pl->pl_first, 0, max_first);

    PLAYLIST_LOCK();
    list = playlist_get();
    list = g_list_nth(list, pl->pl_first);

    /* It sucks having to run the iteration twice but this is the only
       way you can reliably get the maximum width so we can get our
       playlist nice and aligned... -- plasmaroo */

    for (i = pl->pl_first;
         list && i < pl->pl_first + pl->pl_num_visible;
         list = g_list_next(list), i++) {
        PlaylistEntry *entry = list->data;

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
            tpadding_dwidth = MAX(tpadding_dwidth, strlen(length));
        }
    }

    /* Reset */
    list = playlist_get();
    list = g_list_nth(list, pl->pl_first);

    for (i = pl->pl_first;
         list && i < pl->pl_first + pl->pl_num_visible;
         list = g_list_next(list), i++) {
        gint pos;
        PlaylistEntry *entry = list->data;

        if (entry->selected) {
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_SELECTEDBG));
            gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x,
                               pl->pl_widget.y +
                               ((i - pl->pl_first) * pl->pl_fheight),
                               width, pl->pl_fheight);
        }

        /* FIXME: entry->title should NEVER be NULL, and there should
           NEVER be a need to do a UTF-8 conversion. Playlist title
           strings should be kept properly. */

        if (!entry->title) {
            gchar *basename = g_path_get_basename(entry->filename);
            title = filename_to_utf8(basename);
            g_free(basename);
        }
        else
            title = str_to_utf8(entry->title);

        pos = playlist_get_queue_position(entry);

        tail[0] = 0;
        queuepos[0] = 0;
        length[0] = 0;

        if (pos != -1)
            g_snprintf(queuepos, sizeof(queuepos), "%d", pos + 1);

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
        }

        strncat(tail, length, sizeof(tail));
        tail_len = strlen(tail);

        max_time_len = MAX(max_time_len, tail_len);

        if (pos != -1 && tpadding_dwidth <= 0)
            tail_width = width - (width_approx_digits * (strlen(queuepos) + 2.25));
        else if (pos != -1)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + strlen(queuepos) + 4));
        else if (tpadding_dwidth > 0)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + 2.5));
        else
            tail_width = width;

        if (i == playlist_get_position_nolock())
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_CURRENT));
        else
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_NORMAL));
        playlist_list_draw_string(pl, playlist_list_font,
                                  i - pl->pl_first, tail_width, title,
                                  i + 1);

        x = pl->pl_widget.x + width - width_approx_digits * 2;
        y = pl->pl_widget.y + ((i - pl->pl_first) -
                               1) * pl->pl_fheight + ascent;

        frags = NULL;
        frag0 = NULL;

        if ((strlen(tail) > 0) && (tail != NULL)) {
            frags = g_strsplit(tail, ":", 0);
            frag0 = g_strconcat(frags[0], ":", NULL);

            layout = gtk_widget_create_pango_layout(playlistwin, frags[1]);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
            gdk_draw_layout(obj, gc, x - (0.5 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            layout = gtk_widget_create_pango_layout(playlistwin, frag0);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
            gdk_draw_layout(obj, gc, x - (0.75 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            g_free(frag0);
            g_strfreev(frags);
        }

        if (pos != -1) {

            /* DON'T remove the commented code yet please     -- Milosz */

            if (tpadding_dwidth > 0)
                queue_tailpadding = tpadding_dwidth + 1;
            else
                queue_tailpadding = -0.75;

            gdk_draw_rectangle(obj, gc, FALSE,
                               x -
                               (((queue_tailpadding +
                                  strlen(queuepos)) *
                                 width_approx_digits) +
                                (width_approx_digits / 4)),
                               y + abs(descent),
                               (strlen(queuepos)) *
                               width_approx_digits +
                               (width_approx_digits / 2),
                               pl->pl_fheight - 2);

            layout =
                gtk_widget_create_pango_layout(playlistwin, queuepos);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

            gdk_draw_layout(obj, gc,
                            x -
                            ((queue_tailpadding +
                              strlen(queuepos)) * width_approx_digits) +
                            (width_approx_digits / 4),
                            y + abs(descent), layout);
            g_object_unref(layout);
        }

        g_free(title);
    }


    /*
     * Drop target hovering over the playlist, so draw some hint where the
     * drop will occur.
     *
     * This is (currently? unfixably?) broken when dragging files from Qt/KDE apps,
     * probably due to DnD signaling problems (actually i have no clue).
     *
     */

    if (pl->pl_drag_motion) {
        guint pos, plength, lpadding;
	gint x, y, plx, ply;

        if (cfg.show_numbers_in_pl) {
            lpadding = gint_count_digits(playlist_get_length_nolock()) + 1;
            lpadding = ((lpadding + 1) * width_approx_digits);
        }
        else {
            lpadding = 3;
        };

        /* We already hold the mutex and have the playlist locked, so call
           the non-locking function. */
        plength = playlist_get_length_nolock();

        x = pl->drag_motion_x;
        y = pl->drag_motion_y;

        plx = pl->pl_widget.x;
        ply = pl->pl_widget.y;

        if ((x > pl->pl_widget.x) && !(x > pl->pl_widget.width)) {

            if ((y > pl->pl_widget.y)
                && !(y > (pl->pl_widget.height + ply))) {

                pos = ((y - ((Widget *) pl)->y) / pl->pl_fheight) +
                    pl->pl_first;

                if (pos > (plength)) {
                    pos = plength;
                }

                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
			      pl->pl_widget.y + ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));
            }

        }

        /* When dropping on the borders of the playlist, outside the text area,
         * files get appended at the end of the list. Show that too.
         */

        if ((y < ply) || (y > pl->pl_widget.height + ply)) {
            if ((y >= 0) || (y <= (pl->pl_widget.height + ply))) {
                pos = plength;
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));

            }
        }
    }

    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMAL));

    if (cfg.show_numbers_in_pl) {

        padding_plength = playlist_get_length_nolock();

        if (padding_plength == 0) {
            padding_dwidth = 0;
        }
        else {
            padding_dwidth = gint_count_digits(playlist_get_length_nolock());
        }

        padding =
            (padding_dwidth *
             width_approx_digits) + width_approx_digits;


        /* For italic or oblique fonts we add another half of the
         * approximate width */
        if (has_slant)
            padding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc,
                          pl->pl_widget.x + padding,
                          pl->pl_widget.y,
                          pl->pl_widget.x + padding,
                          pl->pl_widget.y + pl->pl_widget.height - 1);
        }
    }

    if (tpadding_dwidth != 0)
    {
        tpadding = (tpadding_dwidth * width_approx_digits) + (width_approx_digits * 1.5);

        if (has_slant)
            tpadding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc,
                          pl->pl_widget.x + pl->pl_widget.width - tpadding,
                          pl->pl_widget.y,
                          pl->pl_widget.x + pl->pl_widget.width - tpadding,
                          pl->pl_widget.y + pl->pl_widget.height - 1);
        }
    }

    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, NULL);

    PLAYLIST_UNLOCK();

    gdk_flush();

    g_free(playlist_rect);
}


PlayList_List *
create_playlist_list(GList ** wlist,
                     GdkPixmap * parent,
                     GdkGC * gc,
                     gint x, gint y,
                     gint w, gint h)
{
    PlayList_List *pl;

    pl = g_new0(PlayList_List, 1);
    widget_init(&pl->pl_widget, parent, gc, x, y, w, h, TRUE);

    pl->pl_widget.button_press_cb =
        (WidgetButtonPressFunc) playlist_list_button_press_cb;
    pl->pl_widget.button_release_cb =
        (WidgetButtonReleaseFunc) playlist_list_button_release_cb;
    pl->pl_widget.motion_cb = (WidgetMotionFunc) playlist_list_motion_cb;
    pl->pl_widget.draw = playlist_list_draw;

    pl->pl_prev_selected = -1;
    pl->pl_prev_min = -1;
    pl->pl_prev_max = -1;

    widget_list_add(wlist, WIDGET(pl));

#ifdef GDK_WINDOWING_X11
    gdk_window_set_events (gdk_get_default_root_window(), GDK_PROPERTY_CHANGE_MASK);
    gdk_window_add_filter (gdk_get_default_root_window(), (GdkFilterFunc)root_event_cb, pl);
#endif

    return pl;
}

void
playlist_list_set_font(const gchar * font)
{

    /* Welcome to bad hack central 2k3 */

    gchar *font_lower;
    gint width_temp;
    gint width_temp_0;

    playlist_list_font = pango_font_description_from_string(font);

    text_get_extents(font,
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz ",
                     &width_approx_letters, NULL, &ascent, &descent);

    width_approx_letters = (width_approx_letters / 53);

    /* Experimental: We don't weigh the 1 into total because it's width is almost always
     * very different from the rest
     */
    text_get_extents(font, "023456789", &width_approx_digits, NULL, NULL,
                     NULL);
    width_approx_digits = (width_approx_digits / 9);

    /* Precache some often used calculations */
    width_approx_digits_half = width_approx_digits / 2;

    /* FIXME: We assume that any other number is broader than the "1" */
    text_get_extents(font, "1", &width_temp, NULL, NULL, NULL);
    text_get_extents(font, "2", &width_temp_0, NULL, NULL, NULL);

    if (abs(width_temp_0 - width_temp) < 2) {
        width_delta_digit_one = 0;
    }
    else {
        width_delta_digit_one = ((width_temp_0 - width_temp) / 2) + 2;
    }

    text_get_extents(font, ":", &width_colon, NULL, NULL, NULL);
    width_colon_third = width_colon / 4;

    font_lower = g_utf8_strdown(font, strlen(font));
    /* This doesn't take any i18n into account, but i think there is none with TTF fonts
     * FIXME: This can probably be retrieved trough Pango too
     */
    has_slant = g_strstr_len(font_lower, strlen(font_lower), "oblique")
        || g_strstr_len(font_lower, strlen(font_lower), "italic");

    g_free(font_lower);
}
