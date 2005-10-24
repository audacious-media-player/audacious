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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "pbutton.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "skin.h"
#include "widget.h"

void
pbutton_draw(PButton * button)
{
    GdkPixmap *obj;

    if (button->pb_allow_draw) {
        obj = button->pb_widget.parent;

        if (button->pb_pressed && button->pb_inside) {
            skin_draw_pixmap(bmp_active_skin, obj,
                             button->pb_widget.gc,
                             button->pb_skin_index2, button->pb_px,
                             button->pb_py, button->pb_widget.x,
                             button->pb_widget.y,
                             button->pb_widget.width,
                             button->pb_widget.height);
        }
        else {
            skin_draw_pixmap(bmp_active_skin, obj,
                             button->pb_widget.gc,
                             button->pb_skin_index1,
                             button->pb_nx, button->pb_ny,
                             button->pb_widget.x, button->pb_widget.y,
                             button->pb_widget.width,
                             button->pb_widget.height);
        }
    }
}

void
pbutton_button_press_cb(GtkWidget * widget,
                        GdkEventButton * event,
                        PButton * button)
{
    if (event->button != 1)
        return;

    if (widget_contains(&button->pb_widget, event->x, event->y)) {
        button->pb_pressed = 1;
        button->pb_inside = 1;
        widget_draw(WIDGET(button));
    }
}

void
pbutton_button_release_cb(GtkWidget * widget,
                          GdkEventButton * event,
                          PButton * button)
{
    if (event->button != 1)
        return;
    if (button->pb_inside && button->pb_pressed) {
        button->pb_inside = 0;
        widget_draw(WIDGET(button));
        if (button->pb_push_cb)
            button->pb_push_cb();
    }
    if (button->pb_pressed)
        button->pb_pressed = 0;
}

void
pbutton_motion_cb(GtkWidget * widget, GdkEventMotion * event,
                  PButton * button)
{
    gint inside;

    if (!button->pb_pressed)
        return;

    inside = widget_contains(&button->pb_widget, event->x, event->y);

    if (inside != button->pb_inside) {
        button->pb_inside = inside;
        widget_draw(WIDGET(button));
    }
}

void
pbutton_set_skin_index(PButton * b, SkinPixmapId si)
{
    b->pb_skin_index1 = b->pb_skin_index2 = si;
}

void
pbutton_set_skin_index1(PButton * b, SkinPixmapId si)
{
    b->pb_skin_index1 = si;
}

void
pbutton_set_skin_index2(PButton * b, SkinPixmapId si)
{
    b->pb_skin_index2 = si;
}

void
pbutton_set_button_data(PButton * b, gint nx, gint ny, gint px, gint py)
{
    if (nx > -1)
        b->pb_nx = nx;
    if (ny > -1)
        b->pb_ny = ny;
    if (px > -1)
        b->pb_px = px;
    if (py > -1)
        b->pb_py = py;
}


PButton *
create_pbutton_ex(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                  gint x, gint y, gint w, gint h, gint nx,
                  gint ny, gint px, gint py, void (*cb) (void),
                  SkinPixmapId si1, SkinPixmapId si2)
{
    PButton *b;

    b = g_new0(PButton, 1);
    widget_init(&b->pb_widget, parent, gc, x, y, w, h, 1);
    b->pb_widget.button_press_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        pbutton_button_press_cb;
    b->pb_widget.button_release_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        pbutton_button_release_cb;
    b->pb_widget.motion_cb =
        (void (*)(GtkWidget *, GdkEventMotion *, gpointer))
        pbutton_motion_cb;

    b->pb_widget.draw = (void (*)(Widget *)) pbutton_draw;
    b->pb_nx = nx;
    b->pb_ny = ny;
    b->pb_px = px;
    b->pb_py = py;
    b->pb_push_cb = cb;
    b->pb_skin_index1 = si1;
    b->pb_skin_index2 = si2;
    b->pb_allow_draw = TRUE;
    b->pb_inside = 0;
    b->pb_pressed = 0;
    widget_list_add(wlist, WIDGET(b));
    return b;
}

PButton *
create_pbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
               gint x, gint y, gint w, gint h, gint nx, gint ny,
               gint px, gint py, void (*cb) (void), SkinPixmapId si)
{
    return create_pbutton_ex(wlist, parent, gc, x, y, w, h, nx, ny, px, py,
                             cb, si, si);
}

void
free_pbutton(PButton * b)
{
    g_free(b);
}
