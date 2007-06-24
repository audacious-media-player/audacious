/*
 * audacious: Cross-platform multimedia player.
 * ui_skinned_button.c: Skinned WA2 buttons
 *
 * Copyright (c) 2007 Tomasz Mon
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "widgets/widgetcore.h"
#include "ui_skinned_button.h"
#include "util.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkimage.h>

#define UI_SKINNED_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UI_TYPE_SKINNED_BUTTON, UiSkinnedButtonPrivate))
typedef struct _UiSkinnedButtonPrivate UiSkinnedButtonPrivate;

struct _UiSkinnedButtonPrivate {
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
static void ui_skinned_button_class_init(UiSkinnedButtonClass *klass);
static void ui_skinned_button_init(UiSkinnedButton *button);
static GObject* ui_skinned_button_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params);
static void ui_skinned_button_destroy(GtkObject *object);
static void ui_skinned_button_realize(GtkWidget *widget);
static void ui_skinned_button_unrealize(GtkWidget *widget);
static void ui_skinned_button_map(GtkWidget *widget);
static void ui_skinned_button_unmap(GtkWidget *widget);
static gint ui_skinned_button_expose(GtkWidget *widget,GdkEventExpose *event);

static void ui_skinned_button_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void ui_skinned_button_update_state(UiSkinnedButton *button);

static guint button_signals[LAST_SIGNAL] = { 0 };
static gint ui_skinned_button_button_press(GtkWidget *widget, GdkEventButton *event);
static gint ui_skinned_button_button_release(GtkWidget *widget, GdkEventButton *event);
static void button_pressed(UiSkinnedButton *button);
static void button_released(UiSkinnedButton *button);
static void ui_skinned_button_pressed(UiSkinnedButton *button);
static void ui_skinned_button_released(UiSkinnedButton *button);
static void ui_skinned_button_clicked(UiSkinnedButton *button);
static void ui_skinned_button_set_pressed (UiSkinnedButton *button, gboolean pressed);

static void ui_skinned_button_add(GtkContainer *container, GtkWidget *widget);
static void ui_skinned_button_toggle_doublesize(UiSkinnedButton *button);

static gint ui_skinned_button_enter_notify(GtkWidget *widget, GdkEventCrossing *event);
static gint ui_skinned_button_leave_notify(GtkWidget *widget, GdkEventCrossing *event);
static void ui_skinned_button_paint(UiSkinnedButton *button);
static void ui_skinned_button_redraw(UiSkinnedButton *button);

GType ui_skinned_button_get_type (void) {
        static GType button_type = 0;

        if (!button_type) {
                static const GTypeInfo button_info = {
                        sizeof (UiSkinnedButtonClass),
                        NULL,                /* base_init */
                        NULL,                /* base_finalize */
                        (GClassInitFunc) ui_skinned_button_class_init,
                        NULL,                /* class_finalize */
                        NULL,                /* class_data */
                        sizeof (UiSkinnedButton),
                        16,                /* n_preallocs */
                        (GInstanceInitFunc) ui_skinned_button_init,
                };

                button_type = g_type_register_static (GTK_TYPE_BIN, "UiSkinnedButton", &button_info, 0);
        }
        return button_type;
}

static void ui_skinned_button_class_init (UiSkinnedButtonClass *klass) {
        GObjectClass *gobject_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        GtkContainerClass *container_class;

        gobject_class = G_OBJECT_CLASS(klass);
        object_class = (GtkObjectClass*)klass;
        widget_class = (GtkWidgetClass*)klass;
        container_class = (GtkContainerClass*)klass;

        parent_class = g_type_class_peek_parent(klass);
        gobject_class->constructor = ui_skinned_button_constructor;
        object_class->destroy = ui_skinned_button_destroy;

        widget_class->realize = ui_skinned_button_realize;
        widget_class->unrealize = ui_skinned_button_unrealize;
        widget_class->map = ui_skinned_button_map;
        widget_class->unmap = ui_skinned_button_unmap;
        widget_class->size_allocate = ui_skinned_button_size_allocate;
        widget_class->expose_event = ui_skinned_button_expose;
        widget_class->button_press_event = ui_skinned_button_button_press;
        widget_class->button_release_event = ui_skinned_button_button_release;
        widget_class->enter_notify_event = ui_skinned_button_enter_notify;
        widget_class->leave_notify_event = ui_skinned_button_leave_notify;

        container_class->add = ui_skinned_button_add;

        klass->pressed = button_pressed;
        klass->released = button_released;
        klass->clicked = NULL;
        klass->doubled = ui_skinned_button_toggle_doublesize;
        klass->redraw = ui_skinned_button_redraw;

        button_signals[PRESSED] = 
                    g_signal_new ("pressed", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST,
                                  G_STRUCT_OFFSET (UiSkinnedButtonClass, pressed), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[RELEASED] = 
                    g_signal_new ("released", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST,
                                  G_STRUCT_OFFSET (UiSkinnedButtonClass, released), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[CLICKED] = 
                    g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (UiSkinnedButtonClass, clicked), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[DOUBLED] = 
                    g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (UiSkinnedButtonClass, doubled), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        button_signals[REDRAW] = 
                    g_signal_new ("redraw", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (UiSkinnedButtonClass, redraw), NULL, NULL,
                                  gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

        g_type_class_add_private (gobject_class, sizeof (UiSkinnedButtonPrivate));
}

static void ui_skinned_button_init (UiSkinnedButton *button) {
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
        priv->image = gtk_image_new();
        button->redraw = TRUE;
        button->inside = FALSE;
        button->type = TYPE_NOT_SET;

        g_object_set (priv->image, "visible", TRUE, NULL);
        gtk_container_add(GTK_CONTAINER(GTK_WIDGET(button)), priv->image);

        GTK_WIDGET_SET_FLAGS (button, GTK_CAN_FOCUS);
        GTK_WIDGET_SET_FLAGS (button, GTK_NO_WINDOW);
}

static void ui_skinned_button_destroy (GtkObject *object) {
        (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static GObject* ui_skinned_button_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params) {
        GObject *object = (*G_OBJECT_CLASS (parent_class)->constructor)(type, n_construct_properties, construct_params);

        return object;
}

static void ui_skinned_button_realize (GtkWidget *widget) {
        UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
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

static void ui_skinned_button_unrealize(GtkWidget *widget) {
        UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);

        if (button->event_window) {
                gdk_window_set_user_data (button->event_window, NULL);
                gdk_window_destroy (button->event_window);
                button->event_window = NULL;
        }

        GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void ui_skinned_button_map(GtkWidget *widget) {
        UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
        g_return_if_fail (UI_IS_SKINNED_BUTTON (widget));
        GTK_WIDGET_CLASS (parent_class)->map(widget);
        gdk_window_show (button->event_window);
}

static void ui_skinned_button_unmap (GtkWidget *widget) {
        UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
        g_return_if_fail (UI_IS_SKINNED_BUTTON(widget));

        if (button->event_window)
                gdk_window_hide (button->event_window);

        GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean ui_skinned_button_expose(GtkWidget *widget, GdkEventExpose *event) {
        if (GTK_WIDGET_DRAWABLE (widget))
                (*GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

        return FALSE;
}

GtkWidget* ui_skinned_button_new () {
        return g_object_new (UI_TYPE_SKINNED_BUTTON, NULL);
}

void ui_skinned_push_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si) {

        UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
        priv->gc = gc;
        priv->w = w;
        priv->h = h;
        sbutton->x = x;
        sbutton->y = y;
        sbutton->nx = nx;
        sbutton->ny = ny;
        sbutton->px = px;
        sbutton->py = py;
        sbutton->type = TYPE_PUSH;
        priv->skin_index1 = si;
        priv->skin_index2 = si;
        priv->fixed = fixed;
        priv->double_size = FALSE;

        gtk_widget_set_size_request(button, priv->w, priv->h);
        gtk_fixed_put(GTK_FIXED(priv->fixed),GTK_WIDGET(button), sbutton->x, sbutton->y);
}

void ui_skinned_toggle_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, gint pnx, gint pny, gint ppx, gint ppy, SkinPixmapId si) {

        UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
        priv->gc = gc;
        priv->w = w;
        priv->h = h;
        sbutton->x = x;
        sbutton->y = y;
        sbutton->nx = nx;
        sbutton->ny = ny;
        sbutton->px = px;
        sbutton->py = py;
        sbutton->pnx = pnx;
        sbutton->pny = pny;
        sbutton->ppx = ppx;
        sbutton->ppy = ppy;
        sbutton->type = TYPE_TOGGLE;
        priv->skin_index1 = si;
        priv->skin_index2 = si;
        priv->fixed = fixed;
        priv->double_size = FALSE;

        gtk_widget_set_size_request(button, priv->w, priv->h);
        gtk_fixed_put(GTK_FIXED(priv->fixed),GTK_WIDGET(button), sbutton->x, sbutton->y);
}

void ui_skinned_small_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h) {

        UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
        priv->gc = gc;
        priv->w = w;
        priv->h = h;
        sbutton->x = x;
        sbutton->y = y;
        sbutton->type = TYPE_SMALL;
        priv->fixed = fixed;
        priv->double_size = FALSE;

        gtk_widget_set_size_request(button, priv->w, priv->h);
        gtk_fixed_put(GTK_FIXED(priv->fixed),GTK_WIDGET(button), sbutton->x, sbutton->y);
}

static void ui_skinned_button_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
        UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
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

static void button_pressed(UiSkinnedButton *button) {
        button->button_down = TRUE;
        ui_skinned_button_update_state(button);
}

static void button_released(UiSkinnedButton *button) {
        button->button_down = FALSE;
        if(button->hover) ui_skinned_button_clicked(button);
        ui_skinned_button_update_state(button);
}

static void ui_skinned_button_update_state(UiSkinnedButton *button) {
        ui_skinned_button_set_pressed(button, button->button_down); 
}

static void ui_skinned_button_set_pressed (UiSkinnedButton *button, gboolean pressed) {
        if (pressed != button->pressed) {
                button->pressed = pressed;
                button->redraw = TRUE;
                ui_skinned_button_paint(button);
        }
}

static gboolean ui_skinned_button_button_press(GtkWidget *widget, GdkEventButton *event) {
        UiSkinnedButton *button;

        if (event->type == GDK_BUTTON_PRESS) {
                button = UI_SKINNED_BUTTON(widget);

                if (event->button == 1)
                        ui_skinned_button_pressed (button);
        }

        return TRUE;
}

static gboolean ui_skinned_button_button_release(GtkWidget *widget, GdkEventButton *event) {
        UiSkinnedButton *button;
        if (event->button == 1) {
                button = UI_SKINNED_BUTTON(widget);
                button->redraw = TRUE;
                ui_skinned_button_released(button);
        }

        return TRUE;
}

static void ui_skinned_button_pressed(UiSkinnedButton *button) {
        g_return_if_fail(UI_IS_SKINNED_BUTTON(button));
        g_signal_emit(button, button_signals[PRESSED], 0);
}

static void ui_skinned_button_released(UiSkinnedButton *button) {
        g_return_if_fail(UI_IS_SKINNED_BUTTON(button));
        g_signal_emit(button, button_signals[RELEASED], 0);
}

static void ui_skinned_button_clicked(UiSkinnedButton *button) {
        g_return_if_fail(UI_IS_SKINNED_BUTTON(button));
        button->inside = !button->inside;
        g_signal_emit(button, button_signals[CLICKED], 0);
}

static gboolean ui_skinned_button_enter_notify(GtkWidget *widget, GdkEventCrossing *event) {
        UiSkinnedButton *button;

        button = UI_SKINNED_BUTTON(widget);
        button->hover = TRUE;
        if(button->button_down) ui_skinned_button_set_pressed(button, TRUE);

        return FALSE;
}

static gboolean ui_skinned_button_leave_notify(GtkWidget *widget, GdkEventCrossing *event) {
        UiSkinnedButton *button;

        button = UI_SKINNED_BUTTON (widget);
        button->hover = FALSE;
        if(button->button_down) ui_skinned_button_set_pressed(button, FALSE);

        return FALSE;
}

static void ui_skinned_button_add(GtkContainer *container, GtkWidget *widget) {
        GTK_CONTAINER_CLASS (parent_class)->add(container, widget);
}

static void ui_skinned_button_toggle_doublesize(UiSkinnedButton *button) {
        GtkWidget *widget = GTK_WIDGET (button);
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
        priv->double_size = !priv->double_size;

        gtk_widget_set_size_request(widget, priv->w*(1+priv->double_size), priv->h*(1+priv->double_size));
        gtk_widget_set_uposition(widget, button->x*(1+priv->double_size), button->y*(1+priv->double_size));

        button->redraw = TRUE;
        ui_skinned_button_paint(button);
}

static void ui_skinned_button_paint(UiSkinnedButton *button) {
        GtkWidget *widget = GTK_WIDGET (button);
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);

        //TYPE_SMALL doesn't have its own face
        if (button->type == TYPE_SMALL || button->type == TYPE_NOT_SET)
            return;

        if (button->redraw == TRUE) {
            button->redraw = FALSE;

            GdkPixmap *obj;
            obj = gdk_pixmap_new(NULL, priv->w, priv->h, gdk_rgb_get_visual()->depth);
            switch (button->type) {
                case TYPE_PUSH:
                    skin_draw_pixmap(bmp_active_skin, obj, priv->gc,
                                     button->pressed ? priv->skin_index2 : priv->skin_index1,
                                     button->pressed ? button->px : button->nx,
                                     button->pressed ? button->py : button->ny,
                                     0, 0, priv->w, priv->h);
                    break;
                case TYPE_TOGGLE:
                    if (button->inside)
                        skin_draw_pixmap(bmp_active_skin, obj, priv->gc,
                                         button->pressed ? priv->skin_index2 : priv->skin_index1,
                                         button->pressed ? button->ppx : button->pnx,
                                         button->pressed ? button->ppy : button->pny,
                                         0, 0, priv->w, priv->h);
                    else
                        skin_draw_pixmap(bmp_active_skin, obj, priv->gc,
                                         button->pressed ? priv->skin_index2 : priv->skin_index1,
                                         button->pressed ? button->px : button->nx,
                                         button->pressed ? button->py : button->ny,
                                         0, 0, priv->w, priv->h);
                    break;
                default:
                    break;
            }

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
}

static void ui_skinned_button_redraw(UiSkinnedButton *button) {
        button->redraw = TRUE;
        ui_skinned_button_paint(button);
}


void ui_skinned_set_push_button_data(GtkWidget *button, gint nx, gint ny, gint px, gint py) {
        UiSkinnedButton *b = UI_SKINNED_BUTTON(button);
        if (nx > -1) b->nx = nx;
        if (ny > -1) b->ny = ny;
        if (px > -1) b->px = px;
        if (py > -1) b->py = py;
}

void ui_skinned_button_set_skin_index(GtkWidget *button, SkinPixmapId si) {
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
        priv->skin_index1 = priv->skin_index2 = si;
}

void ui_skinned_button_set_skin_index1(GtkWidget *button, SkinPixmapId si) {
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
        priv->skin_index1 = si;
}

void ui_skinned_button_set_skin_index2(GtkWidget *button, SkinPixmapId si) {
        UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
        priv->skin_index2 = si;
}
