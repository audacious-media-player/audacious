/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "platform/smartinclude.h"
#include "widgets/widgetcore.h"

#include <gtk/gtkmain.h>
#include <glib-object.h>
#include <glib/gmacros.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkwindow.h>

#include "main.h"
#include "dock.h"
#include "ui_skinned_window.h"
#include "ui_skinned_cursor.h"

static void ui_skinned_window_class_init(SkinnedWindowClass *klass);
static void ui_skinned_window_init(GtkWidget *widget);
static GtkWindowClass *parent = NULL;

GType
ui_skinned_window_get_type(void)
{
  static GType window_type = 0;

  if (!window_type)
    {
      static const GTypeInfo window_info =
      {
        sizeof (SkinnedWindowClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) ui_skinned_window_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (SkinnedWindow),
        0,              /* n_preallocs */
        (GInstanceInitFunc) ui_skinned_window_init
      };

      window_type =
        g_type_register_static (GTK_TYPE_WINDOW, "SkinnedWindow",
                                &window_info, 0);
    }

  return window_type;
}

static gboolean
ui_skinned_window_configure(GtkWidget *widget,
                            GdkEventConfigure *event)
{
    GtkWidgetClass *widget_class;
    SkinnedWindow *window = SKINNED_WINDOW(widget);

    widget_class = (GtkWidgetClass*) parent;

    if (widget_class->configure_event != NULL)
        widget_class->configure_event(widget, event);

    window->x = event->x;
    window->y = event->y;

#if 0
    g_print("%p window->x = %d, window->y = %d\n", window, window->x, window->y);
#endif

    return FALSE;
}

static gboolean
ui_skinned_window_motion_notify_event(GtkWidget *widget,
                                      GdkEventMotion *event)
{
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass*) parent;

    if (widget_class->motion_notify_event != NULL)
        widget_class->motion_notify_event(widget, event);

    if (dock_is_moving(GTK_WINDOW(widget)))
        dock_move_motion(GTK_WINDOW(widget), event);

    return FALSE;
}

static void
ui_skinned_window_class_init(SkinnedWindowClass *klass)
{
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass*) klass;

    parent = gtk_type_class(gtk_window_get_type());

    widget_class->configure_event = ui_skinned_window_configure;
    widget_class->motion_notify_event = ui_skinned_window_motion_notify_event;
}

void
ui_skinned_window_hide(SkinnedWindow *window)
{
    g_return_if_fail(SKINNED_CHECK_WINDOW(window));

    gtk_window_get_position(GTK_WINDOW(window), &window->x, &window->y);
    gtk_widget_hide(GTK_WIDGET(window));
}

void
ui_skinned_window_show(SkinnedWindow *window)
{
    g_return_if_fail(SKINNED_CHECK_WINDOW(window));

    gtk_window_move(GTK_WINDOW(window), window->x, window->y);
    gtk_widget_show_all(GTK_WIDGET(window));
}

static void
ui_skinned_window_init(GtkWidget *widget)
{
    SkinnedWindow *window;
    window = SKINNED_WINDOW(widget);
    window->x = -1;
    window->y = -1;
}

GtkWidget *
ui_skinned_window_new(GtkWindowType type, const gchar *wmclass_name)
{
    GtkWidget *widget = g_object_new(ui_skinned_window_get_type(), NULL);

    if (wmclass_name)
        gtk_window_set_wmclass(GTK_WINDOW(widget), wmclass_name, "Audacious");

    gtk_widget_add_events(GTK_WIDGET(widget),
                          GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK |
                          GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_realize(GTK_WIDGET(widget));

    dock_window_list = dock_window_set_decorated(dock_window_list,
	GTK_WINDOW(widget), cfg.show_wm_decorations);
    gtk_widget_set_app_paintable(GTK_WIDGET(widget), TRUE);

    ui_skinned_cursor_set(GTK_WIDGET(widget));

    SKINNED_WINDOW(widget)->gc = gdk_gc_new(widget->window);

    /* GtkFixed hasn't got its GdkWindow, this means that it can be used to
       display widgets while the logo below will be displayed anyway;
       however fixed positions are not that great, cause the button sizes may (will)
       vary depending on the gtk style used, so it's not possible to center
       them unless a fixed width and heigth is forced (and this may bring to cutted
       text if someone, i.e., uses a big font for gtk widgets);
       other types of container most likely have their GdkWindow, this simply
       means that the logo must be drawn on the container widget, instead of the
       window; otherwise, it won't be displayed correctly */
    SKINNED_WINDOW(widget)->fixed = gtk_fixed_new();

    return widget;
}

void
ui_skinned_window_widgetlist_associate(GtkWidget * widget, Widget * w)
{
    SkinnedWindow *sw;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(w != NULL);

    sw = SKINNED_WINDOW(widget);

    sw->widget_list = g_list_append(sw->widget_list, w);
}

void
ui_skinned_window_widgetlist_dissociate(GtkWidget * widget, Widget * w)
{
    SkinnedWindow *sw;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(w != NULL);

    sw = SKINNED_WINDOW(widget);

    sw->widget_list = g_list_remove(sw->widget_list, w);
}

gboolean
ui_skinned_window_widgetlist_contained(GtkWidget * widget, gint x, gint y)
{
    SkinnedWindow *sw;
    GList *l;

    g_return_val_if_fail(widget != NULL, FALSE);

    sw = SKINNED_WINDOW(widget);

    for (l = sw->widget_list; l != NULL; l = g_list_next(l))
    {
        Widget *w = WIDGET(l->data);

        if (widget_contains(WIDGET(w), x, y) == TRUE)
            return TRUE;
    }

    return FALSE;
}

