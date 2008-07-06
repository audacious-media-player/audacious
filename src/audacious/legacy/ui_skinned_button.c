/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007 Tomasz Moń
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

#include "ui_skinned_button.h"
#include <math.h>

#define UI_SKINNED_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ui_skinned_button_get_type(), UiSkinnedButtonPrivate))
typedef struct _UiSkinnedButtonPrivate UiSkinnedButtonPrivate;

enum {
    PRESSED,
    RELEASED,
    CLICKED,
    DOUBLED,
    REDRAW,
    LAST_SIGNAL
};

struct _UiSkinnedButtonPrivate {
    //Skinned part
    GdkGC            *gc;
    gint             w;
    gint             h;
    SkinPixmapId     skin_index1;
    SkinPixmapId     skin_index2;
    gboolean         scaled;
    gint             move_x, move_y;

    gint             nx, ny, px, py;
    //Toogle button needs also those
    gint             pnx, pny, ppx, ppy;
};


static GtkWidgetClass *parent_class = NULL;
static void ui_skinned_button_class_init(UiSkinnedButtonClass *klass);
static void ui_skinned_button_init(UiSkinnedButton *button);
static void ui_skinned_button_destroy(GtkObject *object);
static void ui_skinned_button_realize(GtkWidget *widget);
static void ui_skinned_button_unrealize(GtkWidget *widget);
static void ui_skinned_button_map(GtkWidget *widget);
static void ui_skinned_button_unmap(GtkWidget *widget);
static void ui_skinned_button_size_request(GtkWidget *widget, GtkRequisition *requisition);
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

static void ui_skinned_button_toggle_scaled(UiSkinnedButton *button);

static gint ui_skinned_button_enter_notify(GtkWidget *widget, GdkEventCrossing *event);
static gint ui_skinned_button_leave_notify(GtkWidget *widget, GdkEventCrossing *event);
static void ui_skinned_button_redraw(UiSkinnedButton *button);

GType ui_skinned_button_get_type() {
    static GType button_type = 0;
    if (!button_type) {
        static const GTypeInfo button_info = {
            sizeof (UiSkinnedButtonClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_button_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedButton),
            0,
            (GInstanceInitFunc) ui_skinned_button_init,
        };
        button_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedButton_", &button_info, 0);
    }

    return button_type;
}

static void ui_skinned_button_class_init (UiSkinnedButtonClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_button_destroy;

    widget_class->realize = ui_skinned_button_realize;
    widget_class->unrealize = ui_skinned_button_unrealize;
    widget_class->map = ui_skinned_button_map;
    widget_class->unmap = ui_skinned_button_unmap;
    widget_class->expose_event = ui_skinned_button_expose;
    widget_class->size_request = ui_skinned_button_size_request;
    widget_class->size_allocate = ui_skinned_button_size_allocate;
    widget_class->button_press_event = ui_skinned_button_button_press;
    widget_class->button_release_event = ui_skinned_button_button_release;
    widget_class->enter_notify_event = ui_skinned_button_enter_notify;
    widget_class->leave_notify_event = ui_skinned_button_leave_notify;

    klass->pressed = button_pressed;
    klass->released = button_released;
    klass->clicked = NULL;
    klass->scaled = ui_skinned_button_toggle_scaled;
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
        g_signal_new ("toggle-scaled", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedButtonClass, scaled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    button_signals[REDRAW] = 
        g_signal_new ("redraw", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedButtonClass, redraw), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (gobject_class, sizeof (UiSkinnedButtonPrivate));
}

static void ui_skinned_button_init (UiSkinnedButton *button) {
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    button->inside = FALSE;
    button->type = TYPE_NOT_SET;
    priv->move_x = 0;
    priv->move_y = 0;
    button->event_window = NULL;
}

static void ui_skinned_button_destroy (GtkObject *object) {
    UiSkinnedButton *button;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_BUTTON (object));

    button = UI_SKINNED_BUTTON(object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_button_realize (GtkWidget *widget) {
    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_BUTTON(widget));
    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
    GdkWindowAttr attributes;
    gint attributes_mask;

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;

    if (button->type == TYPE_SMALL || button->type == TYPE_NOT_SET) {
        widget->window = gtk_widget_get_parent_window (widget);
        g_object_ref (widget->window);
        attributes.wclass = GDK_INPUT_ONLY;
        attributes_mask = GDK_WA_X | GDK_WA_Y;
        button->event_window = gdk_window_new (widget->window, &attributes, attributes_mask);
        GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
        gdk_window_set_user_data(button->event_window, widget);
    } else {
        attributes.visual = gtk_widget_get_visual(widget);
        attributes.colormap = gtk_widget_get_colormap(widget);
        attributes.wclass = GDK_INPUT_OUTPUT;
        attributes.event_mask |= GDK_EXPOSURE_MASK;
        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
        widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);
        GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW);
        gdk_window_set_user_data(widget->window, widget);
    }

    widget->style = gtk_style_attach(widget->style, widget->window);
}

static void ui_skinned_button_unrealize (GtkWidget *widget) {
    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);

    if ( button->event_window != NULL )
    {
      gdk_window_set_user_data( button->event_window , NULL );
      gdk_window_destroy( button->event_window );
      button->event_window = NULL;
    }

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void ui_skinned_button_map (GtkWidget *widget)
{
    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);

    if (button->event_window != NULL)
      gdk_window_show (button->event_window);

    if (GTK_WIDGET_CLASS (parent_class)->map)
      (* GTK_WIDGET_CLASS (parent_class)->map) (widget);
}

static void ui_skinned_button_unmap (GtkWidget *widget)
{
    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);

    if (button->event_window != NULL)
      gdk_window_hide (button->event_window);

    if (GTK_WIDGET_CLASS (parent_class)->unmap)
      (* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static void ui_skinned_button_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(widget);
    requisition->width = priv->w*(priv->scaled ? cfg.scale_factor : 1);
    requisition->height = priv->h*(priv->scaled ? cfg.scale_factor : 1);
}

static void ui_skinned_button_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    widget->allocation = *allocation;
    widget->allocation.x = ceil(widget->allocation.x*(priv->scaled ? cfg.scale_factor : 1));
    widget->allocation.y = ceil(widget->allocation.y*(priv->scaled ? cfg.scale_factor : 1));

    if (GTK_WIDGET_REALIZED (widget))
    {
        if ( button->event_window != NULL )
            gdk_window_move_resize(button->event_window, ceil(allocation->x*(priv->scaled ? cfg.scale_factor : 1)), ceil(allocation->y*(priv->scaled ? cfg.scale_factor : 1)), allocation->width, allocation->height);
        else
            gdk_window_move_resize(widget->window, ceil(allocation->x*(priv->scaled ? cfg.scale_factor : 1)), ceil(allocation->y*(priv->scaled ? cfg.scale_factor : 1)), allocation->width, allocation->height);
    }

    if (button->x + priv->move_x == ceil(widget->allocation.x/(priv->scaled ? cfg.scale_factor : 1)))
        priv->move_x = 0;
    if (button->y + priv->move_y == ceil(widget->allocation.y/(priv->scaled ? cfg.scale_factor : 1)))
        priv->move_y = 0;

    button->x = ceil(widget->allocation.x/(priv->scaled ? cfg.scale_factor : 1));
    button->y = ceil(widget->allocation.y/(priv->scaled ? cfg.scale_factor : 1));
}

static gboolean ui_skinned_button_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_BUTTON (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedButton *button = UI_SKINNED_BUTTON (widget);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    g_return_val_if_fail (priv->w > 0 && priv->h > 0, FALSE);

    //TYPE_SMALL doesn't have its own face
    if (button->type == TYPE_SMALL || button->type == TYPE_NOT_SET)
        return FALSE;

    /* paranoia */
    if (button->event_window != NULL)
        return FALSE;

    GdkPixbuf *obj;
    obj = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, priv->w, priv->h);

    switch (button->type) {
        case TYPE_PUSH:
            skin_draw_pixbuf(widget, aud_active_skin, obj,
                             button->pressed ? priv->skin_index2 : priv->skin_index1,
                             button->pressed ? priv->px : priv->nx,
                             button->pressed ? priv->py : priv->ny,
                             0, 0, priv->w, priv->h);
            break;
        case TYPE_TOGGLE:
            if (button->inside)
                skin_draw_pixbuf(widget, aud_active_skin, obj,
                                 button->pressed ? priv->skin_index2 : priv->skin_index1,
                                 button->pressed ? priv->ppx : priv->pnx,
                                 button->pressed ? priv->ppy : priv->pny,
                                 0, 0, priv->w, priv->h);
            else
                skin_draw_pixbuf(widget, aud_active_skin, obj,
                                 button->pressed ? priv->skin_index2 : priv->skin_index1,
                                 button->pressed ? priv->px : priv->nx,
                                 button->pressed ? priv->py : priv->ny,
                                 0, 0, priv->w, priv->h);
            break;
        default:
            break;
    }

    ui_skinned_widget_draw(widget, obj, priv->w, priv->h, priv->scaled);
    g_object_unref(obj);

    return FALSE;
}

GtkWidget* ui_skinned_button_new () {
    UiSkinnedButton *button = g_object_new (ui_skinned_button_get_type (), NULL);

    return GTK_WIDGET(button);
}

void ui_skinned_push_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si) {

    UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
    priv->w = w;
    priv->h = h;
    sbutton->x = x;
    sbutton->y = y;
    priv->nx = nx;
    priv->ny = ny;
    priv->px = px;
    priv->py = py;
    sbutton->type = TYPE_PUSH;
    priv->skin_index1 = si;
    priv->skin_index2 = si;
    priv->scaled = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(button), sbutton->x, sbutton->y);
}

void ui_skinned_toggle_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, gint pnx, gint pny, gint ppx, gint ppy, SkinPixmapId si) {

    UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
    priv->w = w;
    priv->h = h;
    sbutton->x = x;
    sbutton->y = y;
    priv->nx = nx;
    priv->ny = ny;
    priv->px = px;
    priv->py = py;
    priv->pnx = pnx;
    priv->pny = pny;
    priv->ppx = ppx;
    priv->ppy = ppy;
    sbutton->type = TYPE_TOGGLE;
    priv->skin_index1 = si;
    priv->skin_index2 = si;
    priv->scaled = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(button), sbutton->x, sbutton->y);
}

void ui_skinned_small_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h) {

    UiSkinnedButton *sbutton = UI_SKINNED_BUTTON(button);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(sbutton);
    priv->w = w;
    priv->h = h;
    sbutton->x = x;
    sbutton->y = y;
    sbutton->type = TYPE_SMALL;
    priv->scaled = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(button), sbutton->x, sbutton->y);
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
        gtk_widget_queue_draw(GTK_WIDGET(button));
    }
}

static gboolean ui_skinned_button_button_press(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedButton *button;

    if (event->type == GDK_BUTTON_PRESS) {
        button = UI_SKINNED_BUTTON(widget);

        if (event->button == 1)
            ui_skinned_button_pressed (button);
        else if (event->button == 3) {
            event->x = event->x + button->x;
            event->y = event->y + button->y;
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean ui_skinned_button_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedButton *button;
    if (event->button == 1) {
            button = UI_SKINNED_BUTTON(widget);
            ui_skinned_button_released(button);
    }
    return TRUE;
}

static void ui_skinned_button_pressed(UiSkinnedButton *button) {
    g_return_if_fail(UI_SKINNED_IS_BUTTON(button));
    g_signal_emit(button, button_signals[PRESSED], 0);
}

static void ui_skinned_button_released(UiSkinnedButton *button) {
    g_return_if_fail(UI_SKINNED_IS_BUTTON(button));
    g_signal_emit(button, button_signals[RELEASED], 0);
}

static void ui_skinned_button_clicked(UiSkinnedButton *button) {
    g_return_if_fail(UI_SKINNED_IS_BUTTON(button));
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

static void ui_skinned_button_toggle_scaled(UiSkinnedButton *button) {
    GtkWidget *widget = GTK_WIDGET (button);
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    priv->scaled = !priv->scaled;

    gtk_widget_set_size_request(widget, priv->w*(priv->scaled ? cfg.scale_factor : 1), priv->h*(priv->scaled ? cfg.scale_factor : 1));

    gtk_widget_queue_draw(widget);
}

static void ui_skinned_button_redraw(UiSkinnedButton *button) {
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    if (priv->move_x || priv->move_y)
        gtk_fixed_move(GTK_FIXED(gtk_widget_get_parent(GTK_WIDGET(button))), GTK_WIDGET(button),
                       button->x+priv->move_x, button->y+priv->move_y);

    gtk_widget_queue_draw(GTK_WIDGET(button));
}


void ui_skinned_set_push_button_data(GtkWidget *button, gint nx, gint ny, gint px, gint py) {
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE(button);
    if (nx > -1) priv->nx = nx;
    if (ny > -1) priv->ny = ny;
    if (px > -1) priv->px = px;
    if (py > -1) priv->py = py;
    gtk_widget_queue_draw(button);
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

void ui_skinned_button_move_relative(GtkWidget *button, gint x, gint y) {
    UiSkinnedButtonPrivate *priv = UI_SKINNED_BUTTON_GET_PRIVATE (button);
    priv->move_x += x;
    priv->move_y += y;
}
