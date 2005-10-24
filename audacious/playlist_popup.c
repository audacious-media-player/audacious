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

#include "playlist_popup.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#include "skin.h"
#include "util.h"

typedef struct {
    GtkWidget *window;
    GdkGC *gc;
    gint num_items;
    gint *nx, *ny;
    gint *sx, *sy;
    gint barx, bary;
    gint active, base;
    void (*handler) (gint item);
} PlaylistPopup;

static PlaylistPopup *popup = NULL;

static void
playlist_popup_draw(PlaylistPopup * popup)
{
    gint i;

    skin_draw_pixmap(bmp_active_skin, popup->window->window, popup->gc,
                     SKIN_PLEDIT, popup->barx, popup->bary, 0, 0, 3,
                     popup->num_items * 18);
    for (i = 0; i < popup->num_items; i++) {
        if (i == popup->active)
            skin_draw_pixmap(bmp_active_skin, popup->window->window,
                             popup->gc, SKIN_PLEDIT, popup->sx[i],
                             popup->sy[i], 3, i * 18, 22, 18);
        else
            skin_draw_pixmap(bmp_active_skin, popup->window->window,
                             popup->gc, SKIN_PLEDIT, popup->nx[i],
                             popup->ny[i], 3, i * 18, 22, 18);
    }
    /* FIXME: What is this flush doing here? */
    gdk_flush();
}

void
playlist_popup_destroy(void)
{
    if (popup) {
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
        gdk_flush();
        gtk_widget_destroy(popup->window);
        g_object_unref(popup->gc);
        g_free(popup->nx);
        g_free(popup->ny);
        g_free(popup->sx);
        g_free(popup->sy);
        if (popup->handler && popup->active != -1)
            popup->handler(popup->active + popup->base);
        g_free(popup);
        popup = NULL;
    }
}

static void
playlist_popup_expose(GtkWidget * widget, GdkEvent * event,
                      gpointer callback_data)
{
    playlist_popup_draw(popup);
}

static void
playlist_popup_motion(GtkWidget * widget,
                      GdkEventMotion * event, gpointer callback_data)
{
    gint active;

    if (event->x >= 0 && event->x < 25 && event->y >= 0
        && event->y < popup->num_items * 18) {
        active = event->y / 18;
        if (popup->active != active) {
            popup->active = active;
            playlist_popup_draw(popup);
        }
    }
    else if (popup->active != -1) {
        popup->active = -1;
        playlist_popup_draw(popup);
    }
}

static void
playlist_popup_release(GtkWidget * widget,
                       GdkEventButton * event, gpointer callback_data)
{
    playlist_popup_destroy();
}

void
playlist_popup(gint x, gint y, gint num_items, gint * nx, gint * ny,
               gint * sx, gint * sy, gint barx, gint bary, gint base,
               void (*handler) (gint item))
{
    if (popup)
        playlist_popup_destroy();
    popup = g_new0(PlaylistPopup, 1);
    popup->num_items = num_items;
    popup->nx = g_new0(gint, num_items);
    memcpy(popup->nx, nx, sizeof(gint) * num_items);
    popup->ny = g_new0(gint, num_items);
    memcpy(popup->ny, ny, sizeof(gint) * num_items);
    popup->sx = g_new0(gint, num_items);
    memcpy(popup->sx, sx, sizeof(gint) * num_items);
    popup->sy = g_new0(gint, num_items);
    memcpy(popup->sy, sy, sizeof(gint) * num_items);
    popup->barx = barx;
    popup->bary = bary;
    popup->handler = handler;
    popup->active = num_items - 1;
    popup->base = base;

    popup->window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_default_size(GTK_WINDOW(popup->window), 25,
                                num_items * 18);
    gtk_widget_set_app_paintable(popup->window, TRUE);
    gtk_widget_set_events(popup->window,
                          GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK
                          | GDK_EXPOSURE_MASK);
    gtk_widget_realize(popup->window);

    popup->gc = gdk_gc_new(popup->window->window);

    g_signal_connect(popup->window, "expose_event",
                     G_CALLBACK(playlist_popup_expose), NULL);
    g_signal_connect(popup->window, "motion_notify_event",
                     G_CALLBACK(playlist_popup_motion), NULL);
    g_signal_connect(popup->window, "button_release_event",
                     G_CALLBACK(playlist_popup_release), NULL);

    util_set_cursor(popup->window);

    gtk_window_move(GTK_WINDOW(popup->window), x - 1, y - 1);
    gtk_widget_show(popup->window);
    gdk_window_raise(popup->window->window);
    gdk_flush();

    playlist_popup_draw(popup);

    gdk_pointer_grab(popup->window->window, FALSE,
                     GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                     NULL, NULL, GDK_CURRENT_TIME);
    gdk_flush();
}
