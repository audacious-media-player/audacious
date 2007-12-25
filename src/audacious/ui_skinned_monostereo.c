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
#include "ui_skinned_monostereo.h"
#include "main.h"
#include "util.h"

enum {
    DOUBLED,
    LAST_SIGNAL
};

static void ui_skinned_monostereo_class_init         (UiSkinnedMonoStereoClass *klass);
static void ui_skinned_monostereo_init               (UiSkinnedMonoStereo *monostereo);
static void ui_skinned_monostereo_destroy            (GtkObject *object);
static void ui_skinned_monostereo_realize            (GtkWidget *widget);
static void ui_skinned_monostereo_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_monostereo_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_monostereo_expose         (GtkWidget *widget, GdkEventExpose *event);
static void ui_skinned_monostereo_toggle_doublesize  (UiSkinnedMonoStereo *monostereo);

static GtkWidgetClass *parent_class = NULL;
static guint monostereo_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_monostereo_get_type() {
    static GType monostereo_type = 0;
    if (!monostereo_type) {
        static const GTypeInfo monostereo_info = {
            sizeof (UiSkinnedMonoStereoClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_monostereo_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedMonoStereo),
            0,
            (GInstanceInitFunc) ui_skinned_monostereo_init,
        };
        monostereo_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedMonoStereo", &monostereo_info, 0);
    }

    return monostereo_type;
}

static void ui_skinned_monostereo_class_init(UiSkinnedMonoStereoClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_monostereo_destroy;

    widget_class->realize = ui_skinned_monostereo_realize;
    widget_class->expose_event = ui_skinned_monostereo_expose;
    widget_class->size_request = ui_skinned_monostereo_size_request;
    widget_class->size_allocate = ui_skinned_monostereo_size_allocate;

    klass->doubled = ui_skinned_monostereo_toggle_doublesize;

    monostereo_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedMonoStereoClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ui_skinned_monostereo_init(UiSkinnedMonoStereo *monostereo) {
    monostereo->width = 56;
    monostereo->height = 12;
}

GtkWidget* ui_skinned_monostereo_new(GtkWidget *fixed, gint x, gint y, SkinPixmapId si) {
    UiSkinnedMonoStereo *monostereo = g_object_new (ui_skinned_monostereo_get_type (), NULL);

    monostereo->x = x;
    monostereo->y = y;
    monostereo->skin_index = si;
    monostereo->double_size = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(monostereo), monostereo->x, monostereo->y);

    return GTK_WIDGET(monostereo);
}

static void ui_skinned_monostereo_destroy(GtkObject *object) {
    UiSkinnedMonoStereo *monostereo;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_MONOSTEREO (object));

    monostereo = UI_SKINNED_MONOSTEREO (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_monostereo_realize(GtkWidget *widget) {
    UiSkinnedMonoStereo *monostereo;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_MONOSTEREO(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    monostereo = UI_SKINNED_MONOSTEREO(widget);

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

static void ui_skinned_monostereo_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedMonoStereo *monostereo = UI_SKINNED_MONOSTEREO(widget);

    requisition->width = monostereo->width*(1+monostereo->double_size);
    requisition->height = monostereo->height*(1+monostereo->double_size);
}

static void ui_skinned_monostereo_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedMonoStereo *monostereo = UI_SKINNED_MONOSTEREO (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+monostereo->double_size);
    widget->allocation.y *= (1+monostereo->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    monostereo->x = widget->allocation.x/(monostereo->double_size ? 2 : 1);
    monostereo->y = widget->allocation.y/(monostereo->double_size ? 2 : 1);
}

static gboolean ui_skinned_monostereo_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_MONOSTEREO (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedMonoStereo *monostereo = UI_SKINNED_MONOSTEREO (widget);
    g_return_val_if_fail (monostereo->width > 0 || monostereo->height > 0, FALSE);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, monostereo->width, monostereo->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    switch (monostereo->num_channels) {
    case 1:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 29, 0, 0, 0, 27, 12);
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 0, 12, 27, 0, 29, 12);
        break;
    case 2:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 29, 12, 0, 0, 27, 12);
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 0, 0, 27, 0, 29, 12);
        break;
    default:
    case 0:
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 29, 12, 0, 0, 27, 12);
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, monostereo->skin_index, 0, 12, 27, 0, 29, 12);
        break;
    }

    GdkPixmap *image = NULL;

    if (monostereo->double_size) {
        image = create_dblsize_pixmap(obj);
    } else {
        image = gdk_pixmap_new(NULL, monostereo->width, monostereo->height, gdk_rgb_get_visual()->depth);
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, monostereo->width, monostereo->height);
    }

    g_object_unref(obj);

    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0,
                       monostereo->width*(1+monostereo->double_size), monostereo->height*(1+monostereo->double_size));
    g_object_unref(gc);
    g_object_unref(image);

    return FALSE;
}

static void ui_skinned_monostereo_toggle_doublesize(UiSkinnedMonoStereo *monostereo) {
    GtkWidget *widget = GTK_WIDGET (monostereo);

    monostereo->double_size = !monostereo->double_size;
    gtk_widget_set_size_request(widget, monostereo->width*(1+monostereo->double_size), monostereo->height*(1+monostereo->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(monostereo));
}

void ui_skinned_monostereo_set_num_channels(GtkWidget *widget, gint nch) {
    g_return_if_fail (UI_SKINNED_IS_MONOSTEREO (widget));
    UiSkinnedMonoStereo *monostereo = UI_SKINNED_MONOSTEREO (widget);

    monostereo->num_channels = nch;
    gtk_widget_queue_draw(widget);
}
