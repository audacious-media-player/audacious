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

#ifndef _WIDGETCORE_H_
#error Please do not include me directly! Use widgetcore.h instead!
#endif

#ifndef WIDGET_H
#define WIDGET_H


#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>


typedef struct _Widget Widget;


typedef void (*WidgetButtonPressFunc) (GtkWidget *, GdkEventButton *,
                                       gpointer);
typedef void (*WidgetButtonReleaseFunc) (GtkWidget *, GdkEventButton *,
                                         gpointer);
typedef void (*WidgetMotionFunc) (GtkWidget *, GdkEventMotion *, gpointer);
typedef void (*WidgetDrawFunc) (Widget *);
typedef void (*WidgetScrollFunc) (GtkWidget *, GdkEventScroll *, gpointer);


#define WIDGET(x)  ((Widget *)(x))
struct _Widget {
    GdkPixmap *parent;
    GdkGC *gc;

    gint x, y;
    gint width, height;

    gint visible;
    gboolean redraw;

    GMutex *mutex;

    WidgetButtonPressFunc button_press_cb;
    WidgetButtonReleaseFunc button_release_cb;
    WidgetMotionFunc motion_cb;
    WidgetDrawFunc draw;
    WidgetScrollFunc mouse_scroll_cb;
};


void widget_init(Widget * widget, GdkPixmap * parent, GdkGC * gc,
                 gint x, gint y, gint width, gint height, gint visible);

void widget_set_position(Widget * widget, gint x, gint y);
void widget_set_size(Widget * widget, gint width, gint height);
void widget_queue_redraw(Widget * widget);

void widget_lock(Widget * widget);
void widget_unlock(Widget * widget);

gboolean widget_contains(Widget * widget, gint x, gint y);

void widget_show(Widget * widget);
void widget_hide(Widget * widget);
gboolean widget_is_visible(Widget * widget);

void widget_resize(Widget * widget, gint width, gint height);
void widget_resize_relative(Widget * widget, gint width, gint height);
void widget_move(Widget * widget, gint x, gint y);
void widget_move_relative(Widget * widget, gint x, gint y);
void widget_draw(Widget * widget);
void widget_draw_quick(Widget * widget);

void handle_press_cb(GList * wlist, GtkWidget * widget,
                     GdkEventButton * event);
void handle_release_cb(GList * wlist, GtkWidget * widget,
                       GdkEventButton * event);
void handle_motion_cb(GList * wlist, GtkWidget * widget,
                      GdkEventMotion * event);
void handle_scroll_cb(GList * wlist, GtkWidget * widget,
                      GdkEventScroll * event);

void widget_list_add(GList ** list, Widget * widget);
void widget_list_draw(GList * list, gboolean * redraw, gboolean force);
void widget_list_change_pixmap(GList * list, GdkPixmap * pixmap);
void widget_list_clear_redraw(GList * list);
void widget_list_lock(GList * list);
void widget_list_unlock(GList * list);

#endif
