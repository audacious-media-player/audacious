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
 * along with this program;  If not, see <http://www.gnu.org/licenses>.
 */

#include "skin.h"
#include "ui_skinned_equalizer_slider.h"
#include "util.h"
#include "ui_equalizer.h"
#include "ui_main.h"
#include <glib/gi18n.h>

#define UI_TYPE_SKINNED_EQUALIZER_SLIDER           (ui_skinned_equalizer_slider_get_type())
#define UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UI_TYPE_SKINNED_EQUALIZER_SLIDER, UiSkinnedEqualizerSliderPrivate))
typedef struct _UiSkinnedEqualizerSliderPrivate UiSkinnedEqualizerSliderPrivate;

enum {
    DOUBLED,
    LAST_SIGNAL
};

struct _UiSkinnedEqualizerSliderPrivate {
    SkinPixmapId     skin_index;
    gboolean         double_size;
    gint             position;
    gint             width, height;
    gboolean         pressed;
    gint             drag_y;
};

static void ui_skinned_equalizer_slider_class_init         (UiSkinnedEqualizerSliderClass *klass);
static void ui_skinned_equalizer_slider_init               (UiSkinnedEqualizerSlider *equalizer_slider);
static void ui_skinned_equalizer_slider_destroy            (GtkObject *object);
static void ui_skinned_equalizer_slider_realize            (GtkWidget *widget);
static void ui_skinned_equalizer_slider_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_equalizer_slider_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_equalizer_slider_expose         (GtkWidget *widget, GdkEventExpose *event);
static gboolean ui_skinned_equalizer_slider_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_equalizer_slider_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_equalizer_slider_motion_notify  (GtkWidget *widget, GdkEventMotion *event);
static gboolean ui_skinned_equalizer_slider_scroll         (GtkWidget *widget, GdkEventScroll *event);
static void ui_skinned_equalizer_slider_toggle_doublesize  (UiSkinnedEqualizerSlider *equalizer_slider);
void ui_skinned_equalizer_slider_set_mainwin_text          (UiSkinnedEqualizerSlider * es);

static GtkWidgetClass *parent_class = NULL;
static guint equalizer_slider_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_equalizer_slider_get_type() {
    static GType equalizer_slider_type = 0;
    if (!equalizer_slider_type) {
        static const GTypeInfo equalizer_slider_info = {
            sizeof (UiSkinnedEqualizerSliderClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_equalizer_slider_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedEqualizerSlider),
            0,
            (GInstanceInitFunc) ui_skinned_equalizer_slider_init,
        };
        equalizer_slider_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedEqualizerSlider", &equalizer_slider_info, 0);
    }

    return equalizer_slider_type;
}

static void ui_skinned_equalizer_slider_class_init(UiSkinnedEqualizerSliderClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_equalizer_slider_destroy;

    widget_class->realize = ui_skinned_equalizer_slider_realize;
    widget_class->expose_event = ui_skinned_equalizer_slider_expose;
    widget_class->size_request = ui_skinned_equalizer_slider_size_request;
    widget_class->size_allocate = ui_skinned_equalizer_slider_size_allocate;
    widget_class->button_press_event = ui_skinned_equalizer_slider_button_press;
    widget_class->button_release_event = ui_skinned_equalizer_slider_button_release;
    widget_class->motion_notify_event = ui_skinned_equalizer_slider_motion_notify;
    widget_class->scroll_event = ui_skinned_equalizer_slider_scroll;

    klass->doubled = ui_skinned_equalizer_slider_toggle_doublesize;

    equalizer_slider_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedEqualizerSliderClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (gobject_class, sizeof (UiSkinnedEqualizerSliderPrivate));
}

static void ui_skinned_equalizer_slider_init(UiSkinnedEqualizerSlider *equalizer_slider) {
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(equalizer_slider);
    priv->pressed = FALSE;
}

GtkWidget* ui_skinned_equalizer_slider_new(GtkWidget *fixed, gint x, gint y) {
    UiSkinnedEqualizerSlider *es = g_object_new (ui_skinned_equalizer_slider_get_type (), NULL);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(es);

    es->x = x;
    es->y = y;
    priv->width = 14;
    priv->height = 63;
    priv->skin_index = SKIN_EQMAIN;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(es), es->x, es->y);

    return GTK_WIDGET(es);
}

static void ui_skinned_equalizer_slider_destroy(GtkObject *object) {
    UiSkinnedEqualizerSlider *equalizer_slider;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (object));

    equalizer_slider = UI_SKINNED_EQUALIZER_SLIDER (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_equalizer_slider_realize(GtkWidget *widget) {
    UiSkinnedEqualizerSlider *equalizer_slider;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    equalizer_slider = UI_SKINNED_EQUALIZER_SLIDER(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                             GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gdk_window_set_user_data(widget->window, widget);
}

static void ui_skinned_equalizer_slider_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);

    requisition->width = priv->width*(1+priv->double_size);
    requisition->height = priv->height*(1+priv->double_size);
}

static void ui_skinned_equalizer_slider_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedEqualizerSlider *equalizer_slider = UI_SKINNED_EQUALIZER_SLIDER (widget);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(equalizer_slider);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+priv->double_size);
    widget->allocation.y *= (1+priv->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    equalizer_slider->x = widget->allocation.x/(priv->double_size ? 2 : 1);
    equalizer_slider->y = widget->allocation.y/(priv->double_size ? 2 : 1);
}

static gboolean ui_skinned_equalizer_slider_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedEqualizerSlider *es = UI_SKINNED_EQUALIZER_SLIDER (widget);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(es);
    g_return_val_if_fail (priv->width > 0 && priv->height > 0, FALSE);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, priv->width, priv->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    gint frame;
    frame = 27 - ((priv->position * 27) / 50);
    if (frame < 14)
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, priv->skin_index, (frame * 15) + 13, 164, 0, 0, priv->width, priv->height);
    else
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, priv->skin_index, ((frame - 14) * 15) + 13, 229, 0, 0, priv->width, priv->height);

    if (priv->pressed)
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, priv->skin_index, 0, 176, 1, priv->position, 11, 11);
    else
        skin_draw_pixmap(widget, bmp_active_skin, obj, gc, priv->skin_index, 0, 164, 1, priv->position, 11, 11);

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

static gboolean ui_skinned_equalizer_slider_button_press(GtkWidget *widget, GdkEventButton *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedEqualizerSlider *es = UI_SKINNED_EQUALIZER_SLIDER (widget);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(es);

    gint y;

    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 1) {
            priv->pressed = TRUE;
            y = event->y/(priv->double_size ? 2 : 1);

            if (y >= priv->position && y < priv->position + 11)
                priv->drag_y = y - priv->position;
            else {
                priv->position = y - 5;
                priv->drag_y = 5;
                if (priv->position < 0)
                    priv->position = 0;
                if (priv->position > 50)
                    priv->position = 50;
                if (priv->position >= 24 && priv->position <= 26)
                    priv->position = 25;
                equalizerwin_eq_changed();
            }

            ui_skinned_equalizer_slider_set_mainwin_text(es);
            gtk_widget_queue_draw(widget);
        }
    }

    return TRUE;
}

static gboolean ui_skinned_equalizer_slider_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);

    if (event->button == 1) {
        priv->pressed = FALSE;
        mainwin_release_info_text();
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean ui_skinned_equalizer_slider_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    UiSkinnedEqualizerSlider *es = UI_SKINNED_EQUALIZER_SLIDER(widget);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);

    if (priv->pressed) {
        gint y;

        y = event->y/(priv->double_size ? 2 : 1);
        priv->position = y - priv->drag_y;

        if (priv->position < 0)
            priv->position = 0;
        if (priv->position > 50)
            priv->position = 50;
        if (priv->position >= 24 && priv->position <= 26)
            priv->position = 25;

        ui_skinned_equalizer_slider_set_mainwin_text(es);
        equalizerwin_eq_changed();
        gtk_widget_queue_draw(widget);
    }

    return TRUE;
}

static gboolean ui_skinned_equalizer_slider_scroll(GtkWidget *widget, GdkEventScroll *event) {
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);

    if (event->direction == GDK_SCROLL_UP) {
        priv->position -= 2;

        if (priv->position < 0)
            priv->position = 0;
    }
    else {
        priv->position += 2;

        if (priv->position > 50)
            priv->position = 50;
    }

    equalizerwin_eq_changed();
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static void ui_skinned_equalizer_slider_toggle_doublesize(UiSkinnedEqualizerSlider *equalizer_slider) {
    GtkWidget *widget = GTK_WIDGET (equalizer_slider);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(equalizer_slider);

    priv->double_size = !priv->double_size;

    gtk_widget_set_size_request(widget, priv->width*(1+priv->double_size), priv->height*(1+priv->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(equalizer_slider));
}

void ui_skinned_equalizer_slider_set_position(GtkWidget *widget, gint pos) {
    g_return_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget));
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);

    if (priv->pressed)
        return;

    priv->position = 25 - (gint) ((pos * 25.0) / 20.0);

    if (priv->position < 0)
        priv->position = 0;

    if (priv->position > 50)
        priv->position = 50;

    if (priv->position >= 24 && priv->position <= 26)
        priv->position = 25;

    gtk_widget_queue_draw(widget);
}

gfloat ui_skinned_equalizer_slider_get_position(GtkWidget *widget) {
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_SLIDER (widget), -1);
    UiSkinnedEqualizerSliderPrivate *priv = UI_SKINNED_EQUALIZER_SLIDER_GET_PRIVATE(widget);
    return (20.0 - (((gfloat) priv->position * 20.0) / 25.0));
}

void ui_skinned_equalizer_slider_set_mainwin_text(UiSkinnedEqualizerSlider * es) {
    gint band = 0;
    const gchar *bandname[11] = { N_("PREAMP"), N_("60HZ"), N_("170HZ"),
        N_("310HZ"), N_("600HZ"), N_("1KHZ"),
        N_("3KHZ"), N_("6KHZ"), N_("12KHZ"),
        N_("14KHZ"), N_("16KHZ")
    };
    gchar *tmp;

    if (es->x > 21)
        band = ((es->x - 78) / 18) + 1;

    tmp =
        g_strdup_printf("EQ: %s: %+.1f DB", _(bandname[band]),
                        ui_skinned_equalizer_slider_get_position(GTK_WIDGET(es)));
    mainwin_lock_info_text(tmp);
    g_free(tmp);
}
