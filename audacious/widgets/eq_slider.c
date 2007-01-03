/*  BMP - Cross-platform multimedia player
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

#include "widgetcore.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "ui_equalizer.h"
#include "mainwin.h"
#include "skin.h"

void
eqslider_set_position(EqSlider * es,
                      gfloat pos)
{
    es->es_position = 25 - (gint) ((pos * 25.0) / 20.0);

    if (es->es_position < 0)
        es->es_position = 0;

    if (es->es_position > 50)
        es->es_position = 50;

    if (es->es_position >= 24 && es->es_position <= 26)
        es->es_position = 25;

    widget_draw(WIDGET(es));
}

gfloat
eqslider_get_position(EqSlider * es)
{
    return 20.0 - (((gfloat) es->es_position * 20.0) / 25.0);
}

void
eqslider_draw(Widget * w)
{
    EqSlider *es = (EqSlider *) w;
    GdkPixmap *obj;
    SkinPixmapId src;
    gint frame;

    src = SKIN_EQMAIN;
    obj = es->es_widget.parent;

    frame = 27 - ((es->es_position * 27) / 50);
    if (frame < 14)
        skin_draw_pixmap(bmp_active_skin, obj, es->es_widget.gc, src,
                         (frame * 15) + 13, 164, es->es_widget.x,
                         es->es_widget.y, es->es_widget.width,
                         es->es_widget.height);
    else
        skin_draw_pixmap(bmp_active_skin, obj, es->es_widget.gc, src,
                         ((frame - 14) * 15) + 13, 229, es->es_widget.x,
                         es->es_widget.y, es->es_widget.width,
                         es->es_widget.height);
    if (es->es_isdragging)
        skin_draw_pixmap(bmp_active_skin, obj, es->es_widget.gc, src, 0,
                         176, es->es_widget.x + 1,
                         es->es_widget.y + es->es_position, 11, 11);
    else
        skin_draw_pixmap(bmp_active_skin, obj, es->es_widget.gc, src, 0,
                         164, es->es_widget.x + 1,
                         es->es_widget.y + es->es_position, 11, 11);
}

void
eqslider_set_mainwin_text(EqSlider * es)
{
    gint band = 0;
    const gchar *bandname[11] = { N_("PREAMP"), N_("60HZ"), N_("170HZ"),
        N_("310HZ"), N_("600HZ"), N_("1KHZ"),
        N_("3KHZ"), N_("6KHZ"), N_("12KHZ"),
        N_("14KHZ"), N_("16KHZ")
    };
    gchar *tmp;

    if (es->es_widget.x > 21)
        band = ((es->es_widget.x - 78) / 18) + 1;

    tmp =
        g_strdup_printf("EQ: %s: %+.1f DB", _(bandname[band]),
                        eqslider_get_position(es));
    mainwin_lock_info_text(tmp);
    g_free(tmp);
}

void
eqslider_button_press_cb(GtkWidget * w,
                         GdkEventButton * event,
                         gpointer data)
{
    EqSlider *es = EQ_SLIDER(data);
    gint y;

    if (widget_contains(&es->es_widget, event->x, event->y)) {
        if (event->button == 1) {
            y = event->y - es->es_widget.y;
            es->es_isdragging = TRUE;
            if (y >= es->es_position && y < es->es_position + 11)
                es->es_drag_y = y - es->es_position;
            else {
                es->es_position = y - 5;
                es->es_drag_y = 5;
                if (es->es_position < 0)
                    es->es_position = 0;
                if (es->es_position > 50)
                    es->es_position = 50;
                if (es->es_position >= 24 && es->es_position <= 26)
                    es->es_position = 25;
                equalizerwin_eq_changed();
            }

            eqslider_set_mainwin_text(es);
            widget_draw(WIDGET(es));
        }
        if (event->button == 4) {
            es->es_position -= 2;
            if (es->es_position < 0)
                es->es_position = 0;
            equalizerwin_eq_changed();
            widget_draw(WIDGET(es));
        }
    }
}

void
eqslider_mouse_scroll_cb(GtkWidget * w,
                         GdkEventScroll * event,
                         gpointer data)
{
    EqSlider *es = EQ_SLIDER(data);

    if (!widget_contains(&es->es_widget, event->x, event->y))
        return;

    if (event->direction == GDK_SCROLL_UP) {
        es->es_position -= 2;

        if (es->es_position < 0)
            es->es_position = 0;

        equalizerwin_eq_changed();
        widget_draw(WIDGET(es));
    }
    else {
        es->es_position += 2;

        if (es->es_position > 50)
            es->es_position = 50;

        equalizerwin_eq_changed();
        widget_draw(WIDGET(es));
    }
}

void
eqslider_motion_cb(GtkWidget * w,
                   GdkEventMotion * event,
                   gpointer data)
{
    EqSlider *es = EQ_SLIDER(data);
    gint y;

    y = event->y - es->es_widget.y;
    if (es->es_isdragging) {
        es->es_position = y - es->es_drag_y;
        if (es->es_position < 0)
            es->es_position = 0;
        if (es->es_position > 50)
            es->es_position = 50;
        if (es->es_position >= 24 && es->es_position <= 26)
            es->es_position = 25;
        equalizerwin_eq_changed();
        eqslider_set_mainwin_text(es);
        widget_draw(WIDGET(es));
    }
}

void
eqslider_button_release_cb(GtkWidget * w,
                           GdkEventButton * event,
                           gpointer data)
{
    EqSlider *es = EQ_SLIDER(data);

    if (es->es_isdragging) {
        es->es_isdragging = FALSE;
        mainwin_release_info_text();
        widget_draw(WIDGET(es));
    }
}

EqSlider *
create_eqslider(GList ** wlist,
                GdkPixmap * parent,
                GdkGC * gc,
                gint x, gint y)
{
    EqSlider *es;

    es = g_new0(EqSlider, 1);
    widget_init(&es->es_widget, parent, gc, x, y, 14, 63, TRUE);
    es->es_widget.button_press_cb = eqslider_button_press_cb;
    es->es_widget.button_release_cb = eqslider_button_release_cb;
    es->es_widget.motion_cb = eqslider_motion_cb;
    es->es_widget.draw = eqslider_draw;
    es->es_widget.mouse_scroll_cb = eqslider_mouse_scroll_cb;

    widget_list_add(wlist, WIDGET(es));

    return es;
}
