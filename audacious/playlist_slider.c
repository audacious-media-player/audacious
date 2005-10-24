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

#include "playlist_slider.h"

#include <glib.h>

#include "playlist.h"
#include "playlistwin.h"
#include "skin.h"
#include "widget.h"

void
playlistslider_draw(Widget * w)
{
    PlaylistSlider *ps = (PlaylistSlider *) w;
    GdkPixmap *obj;
    gint y, skinx;

    g_return_if_fail(ps != NULL);
    g_return_if_fail(ps->ps_list != NULL);

    if (playlist_get_length() > ps->ps_list->pl_num_visible)
        y = (ps->ps_list->pl_first * (ps->ps_widget.height - 19)) /
            (playlist_get_length() - ps->ps_list->pl_num_visible);
    else
        y = 0;

    obj = ps->ps_widget.parent;

    if (ps->ps_back_image) {
        if (skin_get_id() != ps->ps_skin_id)
            ps->ps_skin_id = skin_get_id();
        else if (ps->ps_widget.height == ps->ps_prev_height)
            gdk_draw_image(obj, ps->ps_widget.gc,
                           ps->ps_back_image, 0, 0,
                           ps->ps_widget.x,
                           ps->ps_widget.y + ps->ps_prev_y, 8, 18);
        gdk_image_destroy(ps->ps_back_image);
    }

    ps->ps_prev_y = y;
    ps->ps_prev_height = ps->ps_widget.height;
    ps->ps_back_image = gdk_drawable_get_image(obj, ps->ps_widget.x,
                                               ps->ps_widget.y + y, 8, 18);
    if (ps->ps_is_draging)
        skinx = 61;
    else
        skinx = 52;

    skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc, SKIN_PLEDIT,
                     skinx, 53, ps->ps_widget.x, ps->ps_widget.y + y, 8, 18);
}

static void
playlistslider_set_pos(PlaylistSlider * ps, gint y)
{
    gint pos;

    y = CLAMP(y, 0, ps->ps_widget.height - 19);

    pos = (y * (playlist_get_length() - ps->ps_list->pl_num_visible)) /
        (ps->ps_widget.height - 19);
    playlistwin_set_toprow(pos);
}


void
playlistslider_button_press_cb(GtkWidget * widget,
                               GdkEventButton * event, PlaylistSlider * ps)
{
    gint y = event->y - ps->ps_widget.y;

    if (!widget_contains(&ps->ps_widget, event->x, event->y))
        return;

    if (event->button != 1 && event->button != 2)
        return;

    if ((y >= ps->ps_prev_y && y < ps->ps_prev_y + 18)) {
        ps->ps_is_draging |= event->button;
        ps->ps_drag_y = y - ps->ps_prev_y;
        widget_draw(WIDGET(ps));
    }
    else if (event->button == 2) {
        playlistslider_set_pos(ps, y);
        ps->ps_is_draging |= event->button;
        ps->ps_drag_y = 0;
        widget_draw(WIDGET(ps));
    }
    else {
        gint n = ps->ps_list->pl_num_visible / 2;
        if (y < ps->ps_prev_y)
            n *= -1;
        playlistwin_scroll(n);
    }
}

void
playlistslider_button_release_cb(GtkWidget * widget,
                                 GdkEventButton * event,
                                 PlaylistSlider * ps)
{
    if (ps->ps_is_draging) {
        ps->ps_is_draging &= ~event->button;
        widget_draw(WIDGET(ps));
    }
}

void
playlistslider_motion_cb(GtkWidget * widget, GdkEventMotion * event,
                         PlaylistSlider * ps)
{
    gint y;

    if (!ps->ps_is_draging)
        return;

    y = event->y - ps->ps_widget.y - ps->ps_drag_y;
    playlistslider_set_pos(ps, y);
}

PlaylistSlider *
create_playlistslider(GList ** wlist, GdkPixmap * parent,
                      GdkGC * gc, gint x, gint y, gint h,
                      PlayList_List * list)
{
    PlaylistSlider *ps;

    ps = g_new0(PlaylistSlider, 1);
    widget_init(&ps->ps_widget, parent, gc, x, y, 8, h, 1);

    ps->ps_widget.button_press_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        playlistslider_button_press_cb;

    ps->ps_widget.button_release_cb =
        (void (*)(GtkWidget *, GdkEventButton *, gpointer))
        playlistslider_button_release_cb;

    ps->ps_widget.motion_cb =
        (void (*)(GtkWidget *, GdkEventMotion *, gpointer))
        playlistslider_motion_cb;

    ps->ps_widget.draw = playlistslider_draw;
    ps->ps_list = list;

    widget_list_add(wlist, WIDGET(ps));
    return ps;
}
