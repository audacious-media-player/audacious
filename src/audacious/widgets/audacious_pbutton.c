/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007  Audacious development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "widgetcore.h"
#include "audacious_pbutton.h"
#include "../util.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkimage.h>

#define AUDACIOUS_PBUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), AUDACIOUS_TYPE_PBUTTON, AudaciousPButtonPrivate))
typedef struct _AudaciousPButtonPrivate AudaciousPButtonPrivate;

struct _AudaciousPButtonPrivate {
        //Skinned part
        GtkWidget        *image;
        GdkGC            *gc;
        gint             w;
        gint             h;
        SkinPixmapId     skin_index1;
        SkinPixmapId     skin_index2;
        GtkWidget        *fixed;
        gboolean         double_size;
};


static GtkBinClass *parent_class = NULL;
static void audacious_pbutton_class_init(AudaciousPButtonClass *klass);
static void audacious_pbutton_init(AudaciousPButton *button);
static GObject* audacious_pbutton_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params);
static void audacious_pbutton_destroy(GtkObject *object);
static void audacious_pbutton_realize(GtkWidget *widget);
static void audacious_pbutton_unrealize(GtkWidget *widget);
static void audacious_pbutton_map(GtkWidget *widget);
static void audacious_pbutton_unmap(GtkWidget *widget);
static gint audacious_pbutton_expose(GtkWidget *widget,GdkEventExpose *event);

static void audacious_pbutton_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void audacious_pbutton_update_state(AudaciousPButton *button);

static guint button_signals[LAST_SIGNAL] = { 0 };
static gint audacious_pbutton_button_press(GtkWidget *widget, GdkEventButton *event);
static gint audacious_pbutton_button_release(GtkWidget *widget, GdkEventButton *event);
static void button_pressed(AudaciousPButton *button);
static void button_released(AudaciousPButton *button);
static void audacious_pbutton_add(GtkContainer *container, GtkWidget *widget);
static void audacious_pbutton_toggle_doublesize(AudaciousPButton *button);

static gint audacious_pbutton_enter_notify(GtkWidget *widget, GdkEventCrossing *event);
static gint audacious_pbutton_leave_notify(GtkWidget *widget, GdkEventCrossing *event);
static void audacious_pbutton_paint(AudaciousPButton *button);

GType audacious_pbutton_get_type (void) {
        static GType button_type = 0;

        if (!button_type) {
                static const GTypeInfo button_info = {
                        sizeof (AudaciousPButtonClass),
                        NULL,                /* base_init */
                        NULL,                /* base_finalize */
                        (GClassInitFunc) audacious_pbutton_class_init,
                        NULL,                /* class_finalize */
                        NULL,                /* class_data */
                        sizeof (AudaciousPButton),
                        16,                /* n_preallocs */
                        (GInstanceInitFunc) audacious_pbutton_init,
                };

                button_type = g_type_register_static (GTK_TYPE_BIN, "AudaciousPButton", &button_info, 0);
        }
        return button_type;
}

static void audacious_pbutton_class_init (AudaciousPButtonClass *klass) {
        GObjectClass *gobject_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        GtkContainerClass *container_class;

        gobject_class = G_OBJECT_CLASS(klass);
        object_class = (GtkObjectClass*)klass;
        widget_class = (GtkWidgetClass*)klass;
        container_class = (GtkContainerClass*)klass;

        parent_class = g_type_class_peek_parent(klass);
        gobject_class->constructor = audacious_pbutton_constructor;
        object_class->destroy = audacious_pbutton_destroy;

        widget_class->realize = audacious_pbutton_realize;
        widget_class->unrealize = audacious_pbutton_unrealize;
        widget_class->map = audacious_pbutton_map;
        widget_class->unmap = audacious_pbutton_unmap;
        widget_class->size_allocate = audacious_pbutton_size_allocate;
        widget_class->expose_event = audacious_pbutton_expose;
        widget_class->button_press_event = audacious_pbutton_button_press;
        widget_class->button_release_event = audacious_pbutton_button_release;
        widget_class->enter_notify_event = audacious_pbutton_enter_notify;
        widget_class->leave_notify_event = audacious_pbutton_leave_notify;

        container_class->add = audacious_pbutton_add;

        klass->pressed = button_pressed;
        klass->released = button_released;
        klass->clicked = NULL;
        klass->doubled = audacious_pbutton_toggle_doublesize;
        klass->redraw = audacious_pbutton_paint;

        button_signals[PRESSED] = 
                    g_signal_new ("pressed", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST,
                                  G_STRUCT_OFFSET (AudaciousPButtonClass, pressed), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[RELEASED] = 
                    g_signal_new ("released", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST,
                                  G_STRUCT_OFFSET (AudaciousPButtonClass, released), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[CLICKED] = 
                    g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (AudaciousPButtonClass, clicked), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[DOUBLED] = 
                    g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (AudaciousPButtonClass, doubled), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[REDRAW] = 
                    g_signal_new ("redraw", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (AudaciousPButtonClass, redraw), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        g_type_class_add_private (gobject_class, sizeof (AudaciousPButtonPrivate));
}

static void audacious_pbutton_init (AudaciousPButton *button) {
        AudaciousPButtonPrivate *priv = AUDACIOUS_PBUTTON_GET_PRIVATE (button);
        priv->image = gtk_image_new();
        g_object_set (priv->image, "visible", TRUE, NULL);
        gtk_container_add(GTK_CONTAINER(GTK_WIDGET(button)), priv->image);

        GTK_WIDGET_SET_FLAGS (button, GTK_CAN_FOCUS);
        GTK_WIDGET_SET_FLAGS (button, GTK_NO_WINDOW);
}

static void audacious_pbutton_destroy (GtkObject *object) {
        (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static GObject* audacious_pbutton_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params) {
        GObject *object = (*G_OBJECT_CLASS (parent_class)->constructor)(type, n_construct_properties, construct_params);

        return object;
}

static void audacious_pbutton_realize (GtkWidget *widget) {
        AudaciousPButton *button = AUDACIOUS_PBUTTON (widget);
        GdkWindowAttr attrib;

        GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

        attrib.window_type = GDK_WINDOW_CHILD;
        attrib.x = widget->allocation.x;
        attrib.y = widget->allocation.y;
        attrib.width = widget->allocation.width;
        attrib.height = widget->allocation.height;
        attrib.wclass = GDK_INPUT_ONLY;
        attrib.event_mask = gtk_widget_get_events (widget);
        attrib.event_mask |= (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

        widget->window = gtk_widget_get_parent_window(widget);
        g_object_ref(widget->window);

        button->event_window = gdk_window_new(gtk_widget_get_parent_window(widget), &attrib, GDK_WA_X | GDK_WA_Y);
        gdk_window_set_user_data (button->event_window, button);
}

static void audacious_pbutton_unrealize(GtkWidget *widget) {
        AudaciousPButton *button = AUDACIOUS_PBUTTON (widget);

        if (button->event_window) {
                gdk_window_set_user_data (button->event_window, NULL);
                gdk_window_destroy (button->event_window);
                button->event_window = NULL;
        }

        GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void audacious_pbutton_map(GtkWidget *widget) {
        AudaciousPButton *button = AUDACIOUS_PBUTTON (widget);
        g_return_if_fail (AUDACIOUS_IS_PBUTTON (widget));
        GTK_WIDGET_CLASS (parent_class)->map(widget);
        gdk_window_show (button->event_window);
}

static void audacious_pbutton_unmap (GtkWidget *widget) {
        AudaciousPButton *button = AUDACIOUS_PBUTTON (widget);
        g_return_if_fail (AUDACIOUS_IS_PBUTTON(widget));

        if (button->event_window)
                gdk_window_hide (button->event_window);

        GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean audacious_pbutton_expose(GtkWidget *widget, GdkEventExpose *event) {
        if (GTK_WIDGET_DRAWABLE (widget))
                (*GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

        return FALSE;
}

GtkWidget* audacious_pbutton_new () {
        return g_object_new (AUDACIOUS_TYPE_PBUTTON, NULL);
}

void audacious_pbutton_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si) {

        AudaciousPButton *pbutton = AUDACIOUS_PBUTTON(button);
        AudaciousPButtonPrivate *priv = AUDACIOUS_PBUTTON_GET_PRIVATE(pbutton);
        priv->gc = gc;
        priv->w = w;
        priv->h = h;
        pbutton->x = x;
        pbutton->y = y;
        pbutton->nx = nx;
        pbutton->ny = ny;
        pbutton->px = px;
        pbutton->py = py;
        priv->skin_index1 = si;
        priv->skin_index2 = si;
        priv->fixed = fixed;
        priv->double_size = FALSE;

        gtk_widget_set_size_request(button, priv->w, priv->h);
        gtk_fixed_put(GTK_FIXED(priv->fixed),GTK_WIDGET(button), pbutton->x, pbutton->y);
}

static void audacious_pbutton_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
        AudaciousPButton *button = AUDACIOUS_PBUTTON (widget);
        GtkAllocation child_alloc;

        widget->allocation = *allocation;
        if (GTK_BIN (button)->child) {                //image should fill whole widget
                child_alloc.x = widget->allocation.x;
                child_alloc.y = widget->allocation.y;
                child_alloc.width = MAX (1, widget->allocation.width);
                child_alloc.height = MAX (1, widget->allocation.height);

                gtk_widget_size_allocate (GTK_BIN (button)->child, &child_alloc);
        }

        if (GDK_IS_WINDOW(button->event_window))
            gdk_window_move_resize (button->event_window, widget->allocation.x, widget->allocation.y,
                                    widget->allocation.width, widget->allocation.height);
}

static void button_pressed(AudaciousPButton *button) {
        button->button_down = TRUE;
        audacious_pbutton_update_state(button);
}

static void button_released(AudaciousPButton *button) {
        button->button_down = FALSE;
        if(button->hover) audacious_pbutton_clicked(button);
        audacious_pbutton_update_state(button);
}

static void audacious_pbutton_update_state(AudaciousPButton *button) {
        _audacious_pbutton_set_pressed(button, button->button_down); 
}

void _audacious_pbutton_set_pressed (AudaciousPButton *button, gboolean pressed) {
        if (pressed != button->pressed) {
                button->pressed = pressed;
                audacious_pbutton_paint(button);
        }
}

static gboolean audacious_pbutton_button_press(GtkWidget *widget, GdkEventButton *event) {
        AudaciousPButton *button;

        if (event->type == GDK_BUTTON_PRESS) {
                button = AUDACIOUS_PBUTTON(widget);

                if (event->button == 1)
                        audacious_pbutton_pressed (button);
        }

        return TRUE;
}

static gboolean audacious_pbutton_button_release(GtkWidget *widget, GdkEventButton *event) {
        AudaciousPButton *button;
        if (event->button == 1) {
                button = AUDACIOUS_PBUTTON(widget);
                audacious_pbutton_released(button);
        }

        return TRUE;
}

void audacious_pbutton_pressed(AudaciousPButton *button) {
        g_return_if_fail(AUDACIOUS_IS_PBUTTON(button));
        g_signal_emit(button, button_signals[PRESSED], 0);
}

void audacious_pbutton_released(AudaciousPButton *button) {
        g_return_if_fail(AUDACIOUS_IS_PBUTTON(button));
        g_signal_emit(button, button_signals[RELEASED], 0);
}

void audacious_pbutton_clicked(AudaciousPButton *button) {
        g_return_if_fail(AUDACIOUS_IS_PBUTTON(button));
        g_signal_emit(button, button_signals[CLICKED], 0);
}

static gboolean audacious_pbutton_enter_notify(GtkWidget *widget, GdkEventCrossing *event) {
        AudaciousPButton *button;

        button = AUDACIOUS_PBUTTON(widget);
        button->hover = TRUE;
        if(button->button_down) _audacious_pbutton_set_pressed(button, TRUE);

        return FALSE;
}

static gboolean audacious_pbutton_leave_notify(GtkWidget *widget, GdkEventCrossing *event) {
        AudaciousPButton *button;

        button = AUDACIOUS_PBUTTON (widget);
        button->hover = FALSE;
        if(button->button_down) _audacious_pbutton_set_pressed(button, FALSE);

        return FALSE;
}

static void audacious_pbutton_add(GtkContainer *container, GtkWidget *widget) {
        GTK_CONTAINER_CLASS (parent_class)->add(container, widget);
}

static void audacious_pbutton_toggle_doublesize(AudaciousPButton *button) {
        GtkWidget *widget = GTK_WIDGET (button);
        AudaciousPButtonPrivate *priv = AUDACIOUS_PBUTTON_GET_PRIVATE (button);
        priv->double_size = !priv->double_size;

        gtk_widget_set_size_request(widget, priv->w*(1+priv->double_size), priv->h*(1+priv->double_size));
        gtk_widget_set_uposition(widget, button->x*(1+priv->double_size), button->y*(1+priv->double_size));

        audacious_pbutton_paint(button);
}

static void audacious_pbutton_paint(AudaciousPButton *button) {
        GtkWidget *widget = GTK_WIDGET (button);
        AudaciousPButtonPrivate *priv = AUDACIOUS_PBUTTON_GET_PRIVATE (button);

        GdkPixmap *obj;
        obj = gdk_pixmap_new(NULL, priv->w, priv->h, gdk_rgb_get_visual()->depth);
        skin_draw_pixmap(bmp_active_skin, obj, priv->gc, priv->skin_index2,
                        button->pressed ? button->px : button->nx,
                        button->pressed ? button->py : button->ny,
                        0, 0, priv->w, priv->h);
        if(priv->double_size) {
             GdkImage *img, *img2x;
             img = gdk_drawable_get_image(obj, 0, 0, priv->w, priv->h);
             img2x = create_dblsize_image(img);
             gtk_image_set(GTK_IMAGE(priv->image), img2x, NULL);
             g_object_unref(img2x);
             g_object_unref(img);
        } else
             gtk_image_set_from_pixmap(GTK_IMAGE(priv->image), obj, NULL);
        g_object_unref(obj);
        gtk_widget_queue_resize(widget);
}
