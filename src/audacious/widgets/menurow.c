/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
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
#include <gdk/gdk.h>

#include "main.h"
#include "menurow.h"
#include "widget.h"

void
menurow_draw(Widget * widget)
{
    MenuRow *mr = MENU_ROW(widget);

    GdkPixmap *obj = mr->mr_widget.parent;

    if (mr->mr_selected == MENUROW_NONE) {
        if (cfg.always_show_cb || mr->mr_bpushed)
            skin_draw_pixmap(bmp_active_skin, obj,
                             mr->mr_widget.gc,
                             mr->mr_skin_index,
                             mr->mr_nx, mr->mr_ny,
                             mr->mr_widget.x, mr->mr_widget.y, 8, 43);
        else
            skin_draw_pixmap(bmp_active_skin, obj,
                             mr->mr_widget.gc,
                             mr->mr_skin_index,
                             mr->mr_nx + 8, mr->mr_ny,
                             mr->mr_widget.x, mr->mr_widget.y, 8, 43);
    }
    else {
        skin_draw_pixmap(bmp_active_skin, obj,
                         mr->mr_widget.gc,
                         mr->mr_skin_index,
                         mr->mr_sx + ((mr->mr_selected - 1) * 8),
                         mr->mr_sy, mr->mr_widget.x, mr->mr_widget.y, 8, 43);
    }
    if (cfg.always_show_cb || mr->mr_bpushed) {
        if (mr->mr_always_selected)
            skin_draw_pixmap(bmp_active_skin, obj,
                             mr->mr_widget.gc,
                             mr->mr_skin_index,
                             mr->mr_sx + 8, mr->mr_sy + 10,
                             mr->mr_widget.x, mr->mr_widget.y + 10, 8, 8);
        if (mr->mr_doublesize_selected)
            skin_draw_pixmap(bmp_active_skin, obj,
                             mr->mr_widget.gc,
                             mr->mr_skin_index,
                             mr->mr_sx + 24, mr->mr_sy + 26,
                             mr->mr_widget.x, mr->mr_widget.y + 26, 8, 8);
    }

}

MenuRowItem
menurow_find_selected(MenuRow * mr, gint x, gint y)
{
    MenuRowItem ret = MENUROW_NONE;

    x -= mr->mr_widget.x;
    y -= mr->mr_widget.y;
    if (x > 0 && x < 8) {
        if (y >= 0 && y <= 10)
            ret = MENUROW_OPTIONS;
        if (y >= 10 && y <= 17)
            ret = MENUROW_ALWAYS;
        if (y >= 18 && y <= 25)
            ret = MENUROW_FILEINFOBOX;
        if (y >= 26 && y <= 33)
            ret = MENUROW_DOUBLESIZE;
        if (y >= 34 && y <= 42)
            ret = MENUROW_VISUALIZATION;
    }
    return ret;
}

void
menurow_button_press(GtkWidget * widget,
                     GdkEventButton * event,
                     gpointer data)
{
    MenuRow *mr = MENU_ROW(data);

    if (event->button != 1)
        return;

    if (widget_contains(&mr->mr_widget, event->x, event->y)) {
        mr->mr_bpushed = TRUE;
        mr->mr_selected = menurow_find_selected(mr, event->x, event->y);

        widget_draw(WIDGET(mr));

        if (mr->mr_change_callback)
            mr->mr_change_callback(mr->mr_selected);
    }
}

void
menurow_motion(GtkWidget * widget,
               GdkEventMotion * event,
               gpointer data)
{
    MenuRow *mr = MENU_ROW(data);

    if (mr->mr_bpushed) {
        mr->mr_selected = menurow_find_selected(mr, event->x, event->y);

        widget_draw(WIDGET(mr));

        if (mr->mr_change_callback)
            mr->mr_change_callback(mr->mr_selected);
    }
}

void
menurow_button_release(GtkWidget * widget,
                       GdkEventButton * event,
                       gpointer data)
{
    MenuRow *mr = MENU_ROW(data);

    if (mr->mr_bpushed) {
        mr->mr_bpushed = FALSE;

        if (mr->mr_selected == MENUROW_ALWAYS)
            mr->mr_always_selected = !mr->mr_always_selected;

        if (mr->mr_selected == MENUROW_DOUBLESIZE)
            mr->mr_doublesize_selected = !mr->mr_doublesize_selected;

        if ((int)(mr->mr_selected) != -1 && mr->mr_release_callback)
            mr->mr_release_callback(mr->mr_selected);

        mr->mr_selected = MENUROW_NONE;

        widget_draw(WIDGET(mr));
    }
}

MenuRow *
create_menurow(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
               gint x, gint y, gint nx, gint ny, gint sx, gint sy,
               void (*ccb) (MenuRowItem),
               void (*rcb) (MenuRowItem), SkinPixmapId si)
{
    MenuRow *mr;

    mr = g_new0(MenuRow, 1);
    widget_init(&mr->mr_widget, parent, gc, x, y, 8, 43, 1);
    mr->mr_widget.draw = menurow_draw;
    mr->mr_widget.button_press_cb = menurow_button_press;
    mr->mr_widget.motion_cb = menurow_motion;
    mr->mr_widget.button_release_cb = menurow_button_release;
    mr->mr_nx = nx;
    mr->mr_ny = ny;
    mr->mr_sx = sx;
    mr->mr_sy = sy;
    mr->mr_selected = MENUROW_NONE;
    mr->mr_change_callback = ccb;
    mr->mr_release_callback = rcb;
    mr->mr_skin_index = si;

    widget_list_add(wlist, WIDGET(mr));
    return mr;
}
