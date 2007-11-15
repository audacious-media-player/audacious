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
#include "ui_skinned_equalizer_graph.h"
#include "main.h"
#include "util.h"

#define UI_TYPE_SKINNED_EQUALIZER_GRAPH           (ui_skinned_equalizer_graph_get_type())

enum {
    DOUBLED,
    LAST_SIGNAL
};

static void ui_skinned_equalizer_graph_class_init         (UiSkinnedEqualizerGraphClass *klass);
static void ui_skinned_equalizer_graph_init               (UiSkinnedEqualizerGraph *equalizer_graph);
static void ui_skinned_equalizer_graph_destroy            (GtkObject *object);
static void ui_skinned_equalizer_graph_realize            (GtkWidget *widget);
static void ui_skinned_equalizer_graph_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_equalizer_graph_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_equalizer_graph_expose         (GtkWidget *widget, GdkEventExpose *event);
static void ui_skinned_equalizer_graph_toggle_doublesize  (UiSkinnedEqualizerGraph *equalizer_graph);

static GtkWidgetClass *parent_class = NULL;
static guint equalizer_graph_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_equalizer_graph_get_type() {
    static GType equalizer_graph_type = 0;
    if (!equalizer_graph_type) {
        static const GTypeInfo equalizer_graph_info = {
            sizeof (UiSkinnedEqualizerGraphClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_equalizer_graph_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedEqualizerGraph),
            0,
            (GInstanceInitFunc) ui_skinned_equalizer_graph_init,
        };
        equalizer_graph_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedEqualizerGraph", &equalizer_graph_info, 0);
    }

    return equalizer_graph_type;
}

static void ui_skinned_equalizer_graph_class_init(UiSkinnedEqualizerGraphClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_equalizer_graph_destroy;

    widget_class->realize = ui_skinned_equalizer_graph_realize;
    widget_class->expose_event = ui_skinned_equalizer_graph_expose;
    widget_class->size_request = ui_skinned_equalizer_graph_size_request;
    widget_class->size_allocate = ui_skinned_equalizer_graph_size_allocate;

    klass->doubled = ui_skinned_equalizer_graph_toggle_doublesize;

    equalizer_graph_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedEqualizerGraphClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ui_skinned_equalizer_graph_init(UiSkinnedEqualizerGraph *equalizer_graph) {
    equalizer_graph->width = 113;
    equalizer_graph->height = 19;
}

GtkWidget* ui_skinned_equalizer_graph_new(GtkWidget *fixed, gint x, gint y) {
    UiSkinnedEqualizerGraph *equalizer_graph = g_object_new (ui_skinned_equalizer_graph_get_type (), NULL);

    equalizer_graph->x = x;
    equalizer_graph->y = y;
    equalizer_graph->skin_index = SKIN_EQMAIN;
    equalizer_graph->double_size = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(equalizer_graph), equalizer_graph->x, equalizer_graph->y);

    return GTK_WIDGET(equalizer_graph);
}

static void ui_skinned_equalizer_graph_destroy(GtkObject *object) {
    UiSkinnedEqualizerGraph *equalizer_graph;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_EQUALIZER_GRAPH (object));

    equalizer_graph = UI_SKINNED_EQUALIZER_GRAPH (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_equalizer_graph_realize(GtkWidget *widget) {
    UiSkinnedEqualizerGraph *equalizer_graph;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_EQUALIZER_GRAPH(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    equalizer_graph = UI_SKINNED_EQUALIZER_GRAPH(widget);

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

static void ui_skinned_equalizer_graph_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedEqualizerGraph *equalizer_graph = UI_SKINNED_EQUALIZER_GRAPH(widget);

    requisition->width = equalizer_graph->width*(1+equalizer_graph->double_size);
    requisition->height = equalizer_graph->height*(1+equalizer_graph->double_size);
}

static void ui_skinned_equalizer_graph_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedEqualizerGraph *equalizer_graph = UI_SKINNED_EQUALIZER_GRAPH (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+equalizer_graph->double_size);
    widget->allocation.y *= (1+equalizer_graph->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    equalizer_graph->x = widget->allocation.x/(equalizer_graph->double_size ? 2 : 1);
    equalizer_graph->y = widget->allocation.y/(equalizer_graph->double_size ? 2 : 1);
}

void
init_spline(gfloat * x, gfloat * y, gint n, gfloat * y2)
{
    gint i, k;
    gfloat p, qn, sig, un, *u;

    u = (gfloat *) g_malloc(n * sizeof(gfloat));

    y2[0] = u[0] = 0.0;

    for (i = 1; i < n - 1; i++) {
        sig = ((gfloat) x[i] - x[i - 1]) / ((gfloat) x[i + 1] - x[i - 1]);
        p = sig * y2[i - 1] + 2.0;
        y2[i] = (sig - 1.0) / p;
        u[i] =
            (((gfloat) y[i + 1] - y[i]) / (x[i + 1] - x[i])) -
            (((gfloat) y[i] - y[i - 1]) / (x[i] - x[i - 1]));
        u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }
    qn = un = 0.0;

    y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);
    for (k = n - 2; k >= 0; k--)
        y2[k] = y2[k] * y2[k + 1] + u[k];
    g_free(u);
}

gfloat
eval_spline(gfloat xa[], gfloat ya[], gfloat y2a[], gint n, gfloat x)
{
    gint klo, khi, k;
    gfloat h, b, a;

    klo = 0;
    khi = n - 1;
    while (khi - klo > 1) {
        k = (khi + klo) >> 1;
        if (xa[k] > x)
            khi = k;
        else
            klo = k;
    }
    h = xa[khi] - xa[klo];
    a = (xa[khi] - x) / h;
    b = (x - xa[klo]) / h;
    return (a * ya[klo] + b * ya[khi] +
            ((a * a * a - a) * y2a[klo] +
             (b * b * b - b) * y2a[khi]) * (h * h) / 6.0);
}

static gboolean ui_skinned_equalizer_graph_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_EQUALIZER_GRAPH (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedEqualizerGraph *equalizer_graph = UI_SKINNED_EQUALIZER_GRAPH (widget);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, equalizer_graph->width, equalizer_graph->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    GdkColor col;
    guint32 cols[19];
    gint i, y, ymin, ymax, py = 0;
    gfloat x[] = { 0, 11, 23, 35, 47, 59, 71, 83, 97, 109 }, yf[10];

    /*
     * This avoids the init_spline() function to be inlined.
     * Inlining the function caused troubles when compiling with
     * `-O' (at least on FreeBSD).
     */
    void (*__init_spline) (gfloat *, gfloat *, gint, gfloat *) = init_spline;

    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, equalizer_graph->skin_index, 0, 294, 0, 0,
                     equalizer_graph->width, equalizer_graph->height);
    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, equalizer_graph->skin_index, 0, 314,
                     0, 9 + ((cfg.equalizer_preamp * 9) / 20),
                     equalizer_graph->width, 1);

    skin_get_eq_spline_colors(bmp_active_skin, cols);

    __init_spline(x, cfg.equalizer_bands, 10, yf);
    for (i = 0; i < 109; i++) {
        y = 9 -
            (gint) ((eval_spline(x, cfg.equalizer_bands, yf, 10, i) *
                     9.0) / 20.0);
        if (y < 0)
            y = 0;
        if (y > 18)
            y = 18;
        if (!i)
            py = y;
        if (y < py) {
            ymin = y;
            ymax = py;
        }
        else {
            ymin = py;
            ymax = y;
        }
        py = y;
        for (y = ymin; y <= ymax; y++) {
            col.pixel = cols[y];
            gdk_gc_set_foreground(gc, &col);
            gdk_draw_point(obj, gc, i + 2, y);
        }
    }

    GdkPixmap *image = NULL;

    if (equalizer_graph->double_size) {
        image = create_dblsize_pixmap(obj);
    } else {
        image = gdk_pixmap_new(NULL, equalizer_graph->width, equalizer_graph->height, gdk_rgb_get_visual()->depth);
        gdk_draw_drawable (image, gc, obj, 0, 0, 0, 0, equalizer_graph->width, equalizer_graph->height);
    }

    g_object_unref(obj);

    gdk_draw_drawable (widget->window, gc, image, 0, 0, 0, 0,
                       equalizer_graph->width*(1+equalizer_graph->double_size), equalizer_graph->height*(1+equalizer_graph->double_size));
    g_object_unref(gc);
    g_object_unref(image);

    return FALSE;
}

static void ui_skinned_equalizer_graph_toggle_doublesize(UiSkinnedEqualizerGraph *equalizer_graph) {
    GtkWidget *widget = GTK_WIDGET (equalizer_graph);

    equalizer_graph->double_size = !equalizer_graph->double_size;
    gtk_widget_set_size_request(widget, equalizer_graph->width*(1+equalizer_graph->double_size),
                                        equalizer_graph->height*(1+equalizer_graph->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(equalizer_graph));
}
