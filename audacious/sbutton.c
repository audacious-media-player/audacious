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

#include "sbutton.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

void
sbutton_button_press_cb(GtkWidget * widget,
                        GdkEventButton * event,
                        SButton * button)
{
    if (event->button != 1)
        return;

    if (widget_contains(&button->sb_widget, event->x, event->y)) {
        button->sb_pressed = 1;
        button->sb_inside = 1;
    }
}

void
sbutton_button_release_cb(GtkWidget * widget, GdkEventButton * event,
                          SButton * button)
{
    if (event->button != 1)
        return;
    if (button->sb_inside && button->sb_pressed) {
        button->sb_inside = 0;
        if (button->sb_push_cb)
            button->sb_push_cb();
    }
    if (button->sb_pressed)
        button->sb_pressed = 0;
}

void
sbutton_motion_cb(GtkWidget * widget, GdkEventMotion * event,
                  SButton * button)
{
    int inside;

    if (!button->sb_pressed)
        return;

    inside = widget_contains(&button->sb_widget, event->x, event->y);

    if (inside != button->sb_inside)
        button->sb_inside = inside;
}

SButton *
create_sbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
               gint x, gint y, gint w, gint h, void (*cb) (void))
{
    SButton *b;

    b = g_new0(SButton, 1);
    widget_init(&b->sb_widget, parent, gc, x, y, w, h, 1);
    b->sb_widget.button_press_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        sbutton_button_press_cb;
    b->sb_widget.button_release_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        sbutton_button_release_cb;
    b->sb_widget.motion_cb =
        (void (*)(GtkWidget *, GdkEventMotion *, gpointer))
        sbutton_motion_cb;
    b->sb_push_cb = cb;

    widget_list_add(wlist, WIDGET(b));
    return b;
}

void
free_sbutton(SButton * b)
{
    g_free(b);
}
