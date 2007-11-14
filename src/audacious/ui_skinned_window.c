/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include "platform/smartinclude.h"
#include "skin.h"

#include <gtk/gtkmain.h>
#include <glib-object.h>
#include <glib/gmacros.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkwindow.h>
#include <string.h>

#include "main.h"
#include "dock.h"
#include "ui_skinned_window.h"
#include "ui_skinned_cursor.h"
#include "util.h"
#include "ui_playlist.h"

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

    if (!GTK_WIDGET_VISIBLE(widget))
        return FALSE;

    switch(window->type) {
        case WINDOW_MAIN:
            if (cfg.show_wm_decorations)
                gdk_window_get_root_origin(widget->window, &cfg.player_x, &cfg.player_y);
            else
                gdk_window_get_deskrelative_origin(widget->window, &cfg.player_x, &cfg.player_y);
            break;
        case WINDOW_EQ:
            cfg.equalizer_x = event->x;
            cfg.equalizer_y = event->y;
            break;
        case WINDOW_PLAYLIST:
            cfg.playlist_x = event->x;
            cfg.playlist_y = event->y;
            break;
    }

    window->x = event->x;
    window->y = event->y;

    gtk_window_set_keep_above(GTK_WINDOW(widget), cfg.always_on_top);

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

static gboolean ui_skinned_window_focus_in(GtkWidget *widget, GdkEventFocus *focus) {
    gboolean val = GTK_WIDGET_CLASS (parent)->focus_in_event (widget, focus);
    gtk_widget_queue_draw(widget);
    return val;
}

static gboolean ui_skinned_window_focus_out(GtkWidget *widget, GdkEventFocus *focus) {
    gboolean val = GTK_WIDGET_CLASS (parent)->focus_out_event (widget, focus);
    gtk_widget_queue_draw(widget);
    return val;
}

static gboolean ui_skinned_window_expose(GtkWidget *widget, GdkEventExpose *event) {
    SkinnedWindow *window = SKINNED_WINDOW(widget);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    gint width = 0, height = 0;
    switch (window->type) {
        case WINDOW_MAIN:
            width = bmp_active_skin->properties.mainwin_width;
            height = bmp_active_skin->properties.mainwin_height;
            break;
        case WINDOW_EQ:
            width = 275;
            height = 116;
            break;
        case WINDOW_PLAYLIST:
            width = playlistwin_get_width();
            height = cfg.playlist_height;
            break;
        default:
            return FALSE;
    }
    obj = gdk_pixmap_new(NULL, width, height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    gboolean focus = gtk_window_has_toplevel_focus(GTK_WINDOW(widget));

    switch (window->type) {
        case WINDOW_MAIN:
            skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_MAIN, 0, 0, 0, 0, width, height);
            skin_draw_mainwin_titlebar(bmp_active_skin, obj, gc, cfg.player_shaded, focus || !cfg.dim_titlebar);
            break;
        case WINDOW_EQ:
            skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_EQMAIN, 0, 0, 0, 0, width, height);
            if (focus || !cfg.dim_titlebar) {
                if (!cfg.equalizer_shaded)
                    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_EQMAIN, 0, 134, 0, 0, width, 14);
                else
                    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_EQ_EX, 0, 0, 0, 0, width, 14);
            } else {
                if (!cfg.equalizer_shaded)
                    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_EQMAIN, 0, 149, 0, 0, width, 14);
                else
                    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_EQ_EX, 0, 15, 0, 0, width, 14);
            }
            break;
        case WINDOW_PLAYLIST:
            focus |= !cfg.dim_titlebar;
            if (cfg.playlist_shaded) {
                skin_draw_playlistwin_shaded(bmp_active_skin, obj, gc, playlistwin_get_width(), focus);
            } else {
                skin_draw_playlistwin_frame(bmp_active_skin, obj, gc, playlistwin_get_width(), cfg.playlist_height, focus);
            }
            break;
    }

    GdkPixmap *image = NULL;

    if (window->type != WINDOW_PLAYLIST && cfg.doublesize) {
        image = create_dblsize_pixmap(obj);
    } else {
        image = gdk_pixmap_new(NULL, width, height, gdk_rgb_get_visual()->depth);
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, width, height);
    }
    g_object_unref(obj);
    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0, width*(1+cfg.doublesize), height*(1+cfg.doublesize));
    g_object_unref(gc);
    g_object_unref(image);

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
    widget_class->expose_event = ui_skinned_window_expose;
    widget_class->focus_in_event = ui_skinned_window_focus_in;
    widget_class->focus_out_event = ui_skinned_window_focus_out;
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
ui_skinned_window_new(const gchar *wmclass_name)
{
    GtkWidget *widget = g_object_new(ui_skinned_window_get_type(), NULL);
    GtkWindow *window = GTK_WINDOW(widget);

    window->type = SKINNED_WINDOW_TYPE;

    if (wmclass_name)
        gtk_window_set_wmclass(GTK_WINDOW(widget), wmclass_name, "Audacious");

    gtk_widget_add_events(GTK_WIDGET(widget),
                          GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK |
                          GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK);
    gtk_widget_realize(GTK_WIDGET(widget));

    dock_window_list = dock_window_set_decorated(dock_window_list,
	GTK_WINDOW(widget), cfg.show_wm_decorations);
    gtk_widget_set_app_paintable(GTK_WIDGET(widget), TRUE);

    ui_skinned_cursor_set(GTK_WIDGET(widget));

    if (!strcmp(wmclass_name, "player"))
        SKINNED_WINDOW(widget)->type = WINDOW_MAIN;
    if (!strcmp(wmclass_name, "equalizer"))
        SKINNED_WINDOW(widget)->type = WINDOW_EQ;
    if (!strcmp(wmclass_name, "playlist"))
        SKINNED_WINDOW(widget)->type = WINDOW_PLAYLIST;

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
    gtk_container_add(GTK_CONTAINER(widget), GTK_WIDGET(SKINNED_WINDOW(widget)->fixed));
    return widget;
}

void ui_skinned_window_draw_all(GtkWidget *widget) {
    if (SKINNED_WINDOW(widget)->type == WINDOW_MAIN)
        mainwin_refresh_hints();

    gtk_widget_queue_draw(widget);
    GList *iter;
    for (iter = GTK_FIXED (SKINNED_WINDOW(widget)->fixed)->children; iter; iter = g_list_next (iter)) {
         GtkFixedChild *child_data = (GtkFixedChild *) iter->data;
         GtkWidget *child = child_data->widget;
         gtk_widget_queue_draw(child);
    }
}
