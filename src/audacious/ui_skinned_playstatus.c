/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007 Tomasz Moń
 *
 * Based on:
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2004  BMP development team.
 * XMMS:
 * Copyright (C) 1998-2003  XMMS development team.
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

#include "skin.h"
#include "ui_skinned_playstatus.h"
#include "main.h"
#include "util.h"

#define UI_TYPE_SKINNED_PLAYSTATUS           (ui_skinned_playstatus_get_type())

enum {
    DOUBLED,
    LAST_SIGNAL
};

static void ui_skinned_playstatus_class_init         (UiSkinnedPlaystatusClass *klass);
static void ui_skinned_playstatus_init               (UiSkinnedPlaystatus *playstatus);
static void ui_skinned_playstatus_destroy            (GtkObject *object);
static void ui_skinned_playstatus_realize            (GtkWidget *widget);
static void ui_skinned_playstatus_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_playstatus_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_playstatus_expose         (GtkWidget *widget, GdkEventExpose *event);
static void ui_skinned_playstatus_toggle_doublesize  (UiSkinnedPlaystatus *playstatus);

static GtkWidgetClass *parent_class = NULL;
static guint playstatus_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_playstatus_get_type() {
    static GType playstatus_type = 0;
    if (!playstatus_type) {
        static const GTypeInfo playstatus_info = {
            sizeof (UiSkinnedPlaystatusClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_playstatus_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedPlaystatus),
            0,
            (GInstanceInitFunc) ui_skinned_playstatus_init,
        };
        playstatus_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedPlaystatus", &playstatus_info, 0);
    }

    return playstatus_type;
}

static void ui_skinned_playstatus_class_init(UiSkinnedPlaystatusClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_playstatus_destroy;

    widget_class->realize = ui_skinned_playstatus_realize;
    widget_class->expose_event = ui_skinned_playstatus_expose;
    widget_class->size_request = ui_skinned_playstatus_size_request;
    widget_class->size_allocate = ui_skinned_playstatus_size_allocate;

    klass->doubled = ui_skinned_playstatus_toggle_doublesize;

    playstatus_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedPlaystatusClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ui_skinned_playstatus_init(UiSkinnedPlaystatus *playstatus) {
    playstatus->width = 11;
    playstatus->height = 9;
}

GtkWidget* ui_skinned_playstatus_new(GtkWidget *fixed, gint x, gint y) {
    UiSkinnedPlaystatus *playstatus = g_object_new (ui_skinned_playstatus_get_type (), NULL);

    playstatus->x = x;
    playstatus->y = y;

    playstatus->double_size = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(playstatus), playstatus->x, playstatus->y);

    return GTK_WIDGET(playstatus);
}

static void ui_skinned_playstatus_destroy(GtkObject *object) {
    UiSkinnedPlaystatus *playstatus;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYSTATUS (object));

    playstatus = UI_SKINNED_PLAYSTATUS (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_playstatus_realize(GtkWidget *widget) {
    UiSkinnedPlaystatus *playstatus;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYSTATUS(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    playstatus = UI_SKINNED_PLAYSTATUS(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);

    gdk_window_set_user_data(widget->window, widget);
}

static void ui_skinned_playstatus_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS(widget);

    requisition->width = playstatus->width*(1+playstatus->double_size);
    requisition->height = playstatus->height*(1+playstatus->double_size);
}

static void ui_skinned_playstatus_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+playstatus->double_size);
    widget->allocation.y *= (1+playstatus->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    playstatus->x = widget->allocation.x/(playstatus->double_size ? 2 : 1);
    playstatus->y = widget->allocation.y/(playstatus->double_size ? 2 : 1);
}

static gboolean ui_skinned_playstatus_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_PLAYSTATUS (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS (widget);
    g_return_val_if_fail (playstatus->width > 0 || playstatus->height > 0, FALSE);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, playstatus->width, playstatus->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    if (playstatus->status == STATUS_STOP && playstatus->buffering == TRUE)
        playstatus->buffering = FALSE;
    if (playstatus->status == STATUS_PLAY && playstatus->buffering == TRUE)
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 39, 0, 0, 0, 3, playstatus->height);
    else if (playstatus->status == STATUS_PLAY)
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 36, 0, 0, 0, 3, playstatus->height);
    else
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 27, 0, 0, 0, 2, playstatus->height);
    switch (playstatus->status) {
    case STATUS_STOP:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 18, 0, 2, 0, 9, playstatus->height);
        break;
    case STATUS_PAUSE:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 9, 0, 2, 0, 9, playstatus->height);
        break;
    case STATUS_PLAY:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, SKIN_PLAYPAUSE, 1, 0, 3, 0, 8, playstatus->height);
        break;
    }

    GdkPixmap *image = NULL;

    if (playstatus->double_size) {
        image = create_dblsize_pixmap(obj);
    } else {
        image = gdk_pixmap_new(NULL, playstatus->width, playstatus->height, gdk_rgb_get_visual()->depth);
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, playstatus->width, playstatus->height);
    }

    g_object_unref(obj);

    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0,
                       playstatus->width*(1+playstatus->double_size), playstatus->height*(1+playstatus->double_size));
    g_object_unref(gc);
    g_object_unref(image);

    return FALSE;
}

static void ui_skinned_playstatus_toggle_doublesize(UiSkinnedPlaystatus *playstatus) {
    GtkWidget *widget = GTK_WIDGET (playstatus);

    playstatus->double_size = !playstatus->double_size;
    gtk_widget_set_size_request(widget, playstatus->width*(1+playstatus->double_size), playstatus->height*(1+playstatus->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(playstatus));
}

void ui_skinned_playstatus_set_status(GtkWidget *widget, PStatus status) {
    g_return_if_fail (UI_SKINNED_IS_PLAYSTATUS (widget));
    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS (widget);

    playstatus->status = status;
    gtk_widget_queue_draw(widget);
}

void ui_skinned_playstatus_set_buffering(GtkWidget *widget, gboolean status) {
    g_return_if_fail (UI_SKINNED_IS_PLAYSTATUS (widget));
    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS (widget);

    playstatus->buffering = status;
    gtk_widget_queue_draw(widget);
}

void ui_skinned_playstatus_set_size(GtkWidget *widget, gint width, gint height) {
    g_return_if_fail (UI_SKINNED_IS_PLAYSTATUS (widget));
    UiSkinnedPlaystatus *playstatus = UI_SKINNED_PLAYSTATUS (widget);

    playstatus->width = width;
    playstatus->height = height;

    gtk_widget_set_size_request(widget, width*(1+playstatus->double_size), height*(1+playstatus->double_size));
}
