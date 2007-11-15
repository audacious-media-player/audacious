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

#include "ui_skinned_horizontal_slider.h"
#include "main.h"
#include "util.h"

#define UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ui_skinned_horizontal_slider_get_type(), UiSkinnedHorizontalSliderPrivate))
typedef struct _UiSkinnedHorizontalSliderPrivate UiSkinnedHorizontalSliderPrivate;

enum {
    MOTION,
    RELEASE,
    DOUBLED,
    LAST_SIGNAL
};

struct _UiSkinnedHorizontalSliderPrivate {
    SkinPixmapId     skin_index;
    gboolean         double_size;
    gint             frame, frame_offset, frame_height, min, max;
    gint             knob_width, knob_height;
    gint             position;
    gint             width, height;
    gint             (*frame_cb) (gint);
};

static void ui_skinned_horizontal_slider_class_init         (UiSkinnedHorizontalSliderClass *klass);
static void ui_skinned_horizontal_slider_init               (UiSkinnedHorizontalSlider *horizontal_slider);
static void ui_skinned_horizontal_slider_destroy            (GtkObject *object);
static void ui_skinned_horizontal_slider_realize            (GtkWidget *widget);
static void ui_skinned_horizontal_slider_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_horizontal_slider_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_horizontal_slider_expose         (GtkWidget *widget, GdkEventExpose *event);
static gboolean ui_skinned_horizontal_slider_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_horizontal_slider_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_horizontal_slider_motion_notify  (GtkWidget *widget, GdkEventMotion *event);
static void ui_skinned_horizontal_slider_toggle_doublesize  (UiSkinnedHorizontalSlider *horizontal_slider);

static GtkWidgetClass *parent_class = NULL;
static guint horizontal_slider_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_horizontal_slider_get_type() {
    static GType horizontal_slider_type = 0;
    if (!horizontal_slider_type) {
        static const GTypeInfo horizontal_slider_info = {
            sizeof (UiSkinnedHorizontalSliderClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_horizontal_slider_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedHorizontalSlider),
            0,
            (GInstanceInitFunc) ui_skinned_horizontal_slider_init,
        };
        horizontal_slider_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedHorizontalSlider", &horizontal_slider_info, 0);
    }

    return horizontal_slider_type;
}

static void ui_skinned_horizontal_slider_class_init(UiSkinnedHorizontalSliderClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_horizontal_slider_destroy;

    widget_class->realize = ui_skinned_horizontal_slider_realize;
    widget_class->expose_event = ui_skinned_horizontal_slider_expose;
    widget_class->size_request = ui_skinned_horizontal_slider_size_request;
    widget_class->size_allocate = ui_skinned_horizontal_slider_size_allocate;
    widget_class->button_press_event = ui_skinned_horizontal_slider_button_press;
    widget_class->button_release_event = ui_skinned_horizontal_slider_button_release;
    widget_class->motion_notify_event = ui_skinned_horizontal_slider_motion_notify;

    klass->motion = NULL;
    klass->release = NULL;
    klass->doubled = ui_skinned_horizontal_slider_toggle_doublesize;

    horizontal_slider_signals[MOTION] = 
        g_signal_new ("motion", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedHorizontalSliderClass, motion), NULL, NULL,
                      gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    horizontal_slider_signals[RELEASE] = 
        g_signal_new ("release", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedHorizontalSliderClass, release), NULL, NULL,
                      gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    horizontal_slider_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedHorizontalSliderClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (gobject_class, sizeof (UiSkinnedHorizontalSliderPrivate));
}

static void ui_skinned_horizontal_slider_init(UiSkinnedHorizontalSlider *horizontal_slider) {
    horizontal_slider->pressed = FALSE;
}

GtkWidget* ui_skinned_horizontal_slider_new(GtkWidget *fixed, gint x, gint y, gint w, gint h, gint knx, gint kny,
                                            gint kpx, gint kpy, gint kw, gint kh, gint fh,
                                            gint fo, gint min, gint max, gint(*fcb) (gint), SkinPixmapId si) {

    UiSkinnedHorizontalSlider *hs = g_object_new (ui_skinned_horizontal_slider_get_type (), NULL);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(hs);

    hs->x = x;
    hs->y = y;
    priv->width = w;
    priv->height = h;
    hs->knob_nx = knx;
    hs->knob_ny = kny;
    hs->knob_px = kpx;
    hs->knob_py = kpy;
    priv->knob_width = kw;
    priv->knob_height = kh;
    priv->frame_height = fh;
    priv->frame_offset = fo;
    priv->min = min;
    priv->position = min;
    priv->max = max;
    priv->frame_cb = fcb;
    if (priv->frame_cb)
        priv->frame = priv->frame_cb(0);
    priv->skin_index = si;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(hs), hs->x, hs->y);

    return GTK_WIDGET(hs);
}

static void ui_skinned_horizontal_slider_destroy(GtkObject *object) {
    UiSkinnedHorizontalSlider *horizontal_slider;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (object));

    horizontal_slider = UI_SKINNED_HORIZONTAL_SLIDER (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_horizontal_slider_realize(GtkWidget *widget) {
    UiSkinnedHorizontalSlider *horizontal_slider;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    horizontal_slider = UI_SKINNED_HORIZONTAL_SLIDER(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
                             GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gdk_window_set_user_data(widget->window, widget);
}

static void ui_skinned_horizontal_slider_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(widget);

    requisition->width = priv->width*(1+priv->double_size);
    requisition->height = priv->height*(1+priv->double_size);
}

static void ui_skinned_horizontal_slider_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedHorizontalSlider *horizontal_slider = UI_SKINNED_HORIZONTAL_SLIDER (widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(horizontal_slider);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+priv->double_size);
    widget->allocation.y *= (1+priv->double_size);

    if (priv->knob_height == priv->height)
        priv->knob_height = allocation->height/(priv->double_size ? 2 : 1);
    priv->width = allocation->width/(priv->double_size ? 2 : 1);
    priv->height = allocation->height/(priv->double_size ? 2 : 1);

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    horizontal_slider->x = widget->allocation.x/(priv->double_size ? 2 : 1);
    horizontal_slider->y = widget->allocation.y/(priv->double_size ? 2 : 1);
}

static gboolean ui_skinned_horizontal_slider_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedHorizontalSlider *hs = UI_SKINNED_HORIZONTAL_SLIDER (widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(hs);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    if (priv->position > priv->max) priv->position = priv->max;
    else if (priv->position < priv->min) priv->position = priv->min;

    obj = gdk_pixmap_new(NULL, priv->width, priv->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    skin_draw_pixmap(widget, bmp_active_skin, obj, gc,
                     priv->skin_index, priv->frame_offset,
                     priv->frame * priv->frame_height,
                     0, 0, priv->width, priv->height);
    if (hs->pressed)
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc,
                         priv->skin_index, hs->knob_px,
                         hs->knob_py, priv->position,
                         ((priv->height - priv->knob_height) / 2),
                         priv->knob_width, priv->knob_height);
    else
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc,
                         priv->skin_index, hs->knob_nx,
                         hs->knob_ny, priv->position,
                         ((priv->height - priv->knob_height) / 2),
                         priv->knob_width, priv->knob_height);

    GdkPixmap *image = NULL;

    if (priv->double_size) {
        image = create_dblsize_pixmap(obj);
    } else {
        image = gdk_pixmap_new(NULL, priv->width, priv->height, gdk_rgb_get_visual()->depth);
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, priv->width, priv->height);
    }

    g_object_unref(obj);

    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0,
                       priv->width*(1+priv->double_size), priv->height*(1+priv->double_size));
    g_object_unref(gc);
    g_object_unref(image);

    return FALSE;
}

static gboolean ui_skinned_horizontal_slider_button_press(GtkWidget *widget, GdkEventButton *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedHorizontalSlider *hs = UI_SKINNED_HORIZONTAL_SLIDER (widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(hs);

    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 1) {
            gint x;

            x = event->x - (priv->knob_width / (priv->double_size ? 1 : 2));
            hs->pressed = TRUE;

            priv->position = x/(1+priv->double_size);
            if (priv->position < priv->min)
                priv->position = priv->min;
            if (priv->position > priv->max)
                priv->position = priv->max;
            if (priv->frame_cb)
                priv->frame = priv->frame_cb(priv->position);

            g_signal_emit_by_name(widget, "motion", priv->position);
            gtk_widget_queue_draw(widget);
        } else if (event->button == 3) {
            if (hs->pressed) {
                hs->pressed = FALSE;
                g_signal_emit_by_name(widget, "release", priv->position);
                gtk_widget_queue_draw(widget);
            }
            event->x = event->x + hs->x;
            event->y = event->y + hs->y;
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean ui_skinned_horizontal_slider_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedHorizontalSlider *hs = UI_SKINNED_HORIZONTAL_SLIDER(widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(widget);

    if (hs->pressed) {
        hs->pressed = FALSE;
        g_signal_emit_by_name(widget, "release", priv->position);
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean ui_skinned_horizontal_slider_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    UiSkinnedHorizontalSlider *hs = UI_SKINNED_HORIZONTAL_SLIDER(widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(widget);

    if (hs->pressed) {
        gint x;

        x = event->x - (priv->knob_width / (priv->double_size ? 1 : 2));
        priv->position = x/(1+priv->double_size);

        if (priv->position < priv->min)
            priv->position = priv->min;

        if (priv->position > priv->max)
            priv->position = priv->max;

        if (priv->frame_cb)
            priv->frame = priv->frame_cb(priv->position);

        g_signal_emit_by_name(widget, "motion", priv->position);
        gtk_widget_queue_draw(widget);
    }

    return TRUE;
}

static void ui_skinned_horizontal_slider_toggle_doublesize(UiSkinnedHorizontalSlider *horizontal_slider) {
    GtkWidget *widget = GTK_WIDGET (horizontal_slider);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(horizontal_slider);

    priv->double_size = !priv->double_size;

    gtk_widget_set_size_request(widget, priv->width*(1+priv->double_size), priv->height*(1+priv->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(horizontal_slider));
}

void ui_skinned_horizontal_slider_set_position(GtkWidget *widget, gint pos) {
    g_return_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (widget));
    UiSkinnedHorizontalSlider *hs = UI_SKINNED_HORIZONTAL_SLIDER(widget);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(widget);

    if (pos == priv->position || hs->pressed)
        return;

    priv->position = pos;

    if (priv->frame_cb)
        priv->frame = priv->frame_cb(priv->position);

    gtk_widget_queue_draw(widget);
}

gint ui_skinned_horizontal_slider_get_position(GtkWidget *widget) {
    g_return_val_if_fail (UI_SKINNED_IS_HORIZONTAL_SLIDER (widget), -1);
    UiSkinnedHorizontalSliderPrivate *priv = UI_SKINNED_HORIZONTAL_SLIDER_GET_PRIVATE(widget);
    return priv->position;
}
