/*  Audacious
 *  Copyright (C) 2005-2007  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include "widgetcore.h"

#include <glib.h>
#include <gdk/gdk.h>

#include "debug.h"


void
widget_init(Widget * widget, GdkPixmap * parent, GdkGC * gc,
            gint x, gint y, gint width, gint height, gint visible)
{
    widget->parent = parent;
    widget->gc = gc;
    widget_set_position(widget, x, y);
    widget_set_size(widget, width, height);
    widget->visible = visible;
    widget->redraw = TRUE;
    widget->mutex = g_mutex_new();
}

void
widget_set_position(Widget * widget, gint x, gint y)
{
    widget->x = x;
    widget->y = y;
    widget_queue_redraw(widget);
}

void
widget_set_size(Widget * widget, gint width, gint height)
{
    widget->width = width;
    widget->height = height;
    widget_queue_redraw(widget);
}

void
widget_queue_redraw(Widget * widget)
{
    widget->redraw = TRUE;
}

gboolean
widget_contains(Widget * widget, gint x, gint y)
{
    return (widget->visible &&
            x >= widget->x && 
            y >= widget->y && 
            x <  widget->x + widget->width && 
            y <  widget->y + widget->height);
}

void
widget_show(Widget * widget)
{
    if (!widget)
        return;

    widget->visible = TRUE;
    widget_draw(widget);
}

void
widget_hide(Widget * widget)
{
    if (!widget)
        return;

    widget->visible = FALSE;
}

gboolean
widget_is_visible(Widget * widget)
{
    if (!widget)
        return FALSE;

    return widget->visible;
}

void
widget_resize(Widget * widget, gint width, gint height)
{
    widget_set_size(widget, width, height);
}

void
widget_resize_relative(Widget * widget, gint width, gint height)
{
    widget_resize(widget, widget->width + width, widget->height + height);
}

void
widget_move(Widget * widget, gint x, gint y)
{
    widget_lock(widget);
    widget_set_position(widget, x, y);
    widget_unlock(widget);
}

void
widget_move_relative(Widget * widget, gint x, gint y)
{
    widget_move(widget, widget->x + x, widget->y + y);
}

void
widget_draw(Widget * widget)
{
    if (widget->visible != TRUE)
        return;

    widget_lock(widget);
    WIDGET(widget)->redraw = TRUE;
    widget_unlock(widget);
}

void
widget_draw_quick(Widget * widget)
{
    widget_lock(widget);
    if (WIDGET(widget)->draw != NULL)
    {
        WIDGET(widget)->draw(widget);
        gdk_flush();
    }
    widget_unlock(widget);
}

void
widget_list_add(GList ** list, Widget * widget)
{
    (*list) = g_list_append(*list, widget);
}

void
handle_press_cb(GList * widget_list, GtkWidget * widget,
                GdkEventButton * event)
{
    GList *wl;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        if (WIDGET(wl->data)->button_press_cb)
            WIDGET(wl->data)->button_press_cb(widget, event, wl->data);
    }
}

void
handle_release_cb(GList * widget_list, GtkWidget * widget,
                  GdkEventButton * event)
{
    GList *wl;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        if (WIDGET(wl->data)->button_release_cb)
            WIDGET(wl->data)->button_release_cb(widget, event, wl->data);
    }
}

void
handle_motion_cb(GList * widget_list, GtkWidget * widget,
                 GdkEventMotion * event)
{
    GList *wl;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        if (WIDGET(wl->data)->motion_cb)
            WIDGET(wl->data)->motion_cb(widget, event, wl->data);
    }
}

void
handle_scroll_cb(GList * wlist, GtkWidget * widget, GdkEventScroll * event)
{
    GList *wl;

    for (wl = wlist; wl; wl = g_list_next(wl)) {
        if (WIDGET(wl->data)->mouse_scroll_cb)
            WIDGET(wl->data)->mouse_scroll_cb(widget, event, wl->data);
    }
}

void
widget_list_draw(GList * widget_list, gboolean * redraw, gboolean force)
{
    GList *wl;
    Widget *w;

    *redraw = FALSE;
    wl = widget_list;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        w = WIDGET(wl->data);

        REQUIRE_LOCK(w->mutex);

        if (!w->draw)
            continue;

        if (!w->visible)
            continue;

        if (w->redraw || force) {
            w->draw(w);
/*             w->redraw = FALSE; */
            *redraw = TRUE;
        }
    }
}

void
widget_list_change_pixmap(GList * widget_list, GdkPixmap * pixmap)
{
    GList *wl;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        Widget *widget = wl->data;
        widget->parent = pixmap;
        widget_queue_redraw(widget);
    }
}

void
widget_list_clear_redraw(GList * widget_list)
{
    GList *wl;

    for (wl = widget_list; wl; wl = g_list_next(wl)) {
        REQUIRE_LOCK(WIDGET(wl->data)->mutex);
        WIDGET(wl->data)->redraw = FALSE;
    }
}

void
widget_lock(Widget * widget)
{
    g_mutex_lock(WIDGET(widget)->mutex);
}

void
widget_unlock(Widget * widget)
{
    g_mutex_unlock(WIDGET(widget)->mutex);
}

void
widget_list_lock(GList * widget_list)
{
    g_list_foreach(widget_list, (GFunc) widget_lock, NULL);
}

void
widget_list_unlock(GList * widget_list)
{
    g_list_foreach(widget_list, (GFunc) widget_unlock, NULL);
}
