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

#include "ui_skinned_menurow.h"
#include "main.h"

enum {
    DOUBLED,
    CHANGE,
    RELEASE,
    LAST_SIGNAL
};

static void ui_skinned_menurow_class_init         (UiSkinnedMenurowClass *klass);
static void ui_skinned_menurow_init               (UiSkinnedMenurow *menurow);
static void ui_skinned_menurow_destroy            (GtkObject *object);
static void ui_skinned_menurow_realize            (GtkWidget *widget);
static void ui_skinned_menurow_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_menurow_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_menurow_expose         (GtkWidget *widget, GdkEventExpose *event);
static MenuRowItem menurow_find_selected          (UiSkinnedMenurow * mr, gint x, gint y);
static gboolean ui_skinned_menurow_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_menurow_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_menurow_motion_notify  (GtkWidget *widget, GdkEventMotion *event);
static void ui_skinned_menurow_toggle_scaled  (UiSkinnedMenurow *menurow);

static GtkWidgetClass *parent_class = NULL;
static guint menurow_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_menurow_get_type() {
    static GType menurow_type = 0;
    if (!menurow_type) {
        static const GTypeInfo menurow_info = {
            sizeof (UiSkinnedMenurowClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_menurow_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedMenurow),
            0,
            (GInstanceInitFunc) ui_skinned_menurow_init,
        };
        menurow_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedMenurow", &menurow_info, 0);
    }

    return menurow_type;
}

static void ui_skinned_menurow_class_init(UiSkinnedMenurowClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_menurow_destroy;

    widget_class->realize = ui_skinned_menurow_realize;
    widget_class->expose_event = ui_skinned_menurow_expose;
    widget_class->size_request = ui_skinned_menurow_size_request;
    widget_class->size_allocate = ui_skinned_menurow_size_allocate;
    widget_class->button_press_event = ui_skinned_menurow_button_press;
    widget_class->button_release_event = ui_skinned_menurow_button_release;
    widget_class->motion_notify_event = ui_skinned_menurow_motion_notify;

    klass->scaled = ui_skinned_menurow_toggle_scaled;
    klass->change = NULL;
    klass->release = NULL;

    menurow_signals[DOUBLED] = 
        g_signal_new ("toggle-scaled", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedMenurowClass, scaled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);


    menurow_signals[CHANGE] = 
        g_signal_new ("change", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedMenurowClass, change), NULL, NULL,
                      gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    menurow_signals[RELEASE] = 
        g_signal_new ("release", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedMenurowClass, release), NULL, NULL,
                      g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);

}

static void ui_skinned_menurow_init(UiSkinnedMenurow *menurow) {
    menurow->scale_selected = cfg.scaled;
    menurow->always_selected = cfg.always_on_top;
}

GtkWidget* ui_skinned_menurow_new(GtkWidget *fixed, gint x, gint y, gint nx, gint ny, gint sx, gint sy, SkinPixmapId si) {
    UiSkinnedMenurow *menurow = g_object_new (ui_skinned_menurow_get_type (), NULL);

    menurow->x = x;
    menurow->y = y;
    menurow->width = 8;
    menurow->height = 43;
    menurow->nx = nx;
    menurow->ny = ny;
    menurow->sx = sx;
    menurow->sy = sy;
    menurow->selected = MENUROW_NONE;

    menurow->skin_index = si;

    menurow->scaled = FALSE;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(menurow), menurow->x, menurow->y);

    return GTK_WIDGET(menurow);
}

static void ui_skinned_menurow_destroy(GtkObject *object) {
    UiSkinnedMenurow *menurow;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_MENUROW (object));

    menurow = UI_SKINNED_MENUROW (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_menurow_realize(GtkWidget *widget) {
    UiSkinnedMenurow *menurow;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_MENUROW(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    menurow = UI_SKINNED_MENUROW(widget);

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

static void ui_skinned_menurow_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW(widget);

    requisition->width = menurow->width*(menurow->scaled ? cfg.scale_factor : 1);
    requisition->height = menurow->height*(menurow->scaled ? cfg.scale_factor : 1);
}

static void ui_skinned_menurow_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (menurow->scaled ? cfg.scale_factor : 1);
    widget->allocation.y *= (menurow->scaled ? cfg.scale_factor : 1);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    menurow->x = widget->allocation.x/(menurow->scaled ? cfg.scale_factor : 1);
    menurow->y = widget->allocation.y/(menurow->scaled ? cfg.scale_factor : 1);
}

static gboolean ui_skinned_menurow_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_MENUROW (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW (widget);
    g_return_val_if_fail (menurow->width > 0 && menurow->height > 0, FALSE);

    GdkPixbuf *obj = NULL;
    obj = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, menurow->width, menurow->height);

    if (menurow->selected == MENUROW_NONE) {
        if (cfg.always_show_cb || menurow->pushed)
            skin_draw_pixbuf(widget, aud_active_skin, obj, menurow->skin_index,
                             menurow->nx, menurow->ny, 0, 0, 8, 43);
        else
            skin_draw_pixbuf(widget, aud_active_skin, obj, menurow->skin_index,
                             menurow->nx + 8, menurow->ny, 0, 0, 8, 43);
    }
    else {
        skin_draw_pixbuf(widget, aud_active_skin, obj, menurow->skin_index,
                         menurow->sx + ((menurow->selected - 1) * 8),
                         menurow->sy, 0, 0, 8, 43);
    }
    if (cfg.always_show_cb || menurow->pushed) {
        if (menurow->always_selected)
            skin_draw_pixbuf(widget, aud_active_skin, obj, menurow->skin_index,
                             menurow->sx + 8, menurow->sy + 10, 0, 10, 8, 8);
        if (menurow->scale_selected)
            skin_draw_pixbuf(widget, aud_active_skin, obj, menurow->skin_index,
                             menurow->sx + 24, menurow->sy + 26, 0, 26, 8, 8);
    }

    ui_skinned_widget_draw(widget, obj, menurow->width, menurow->height, menurow->scaled);

    g_object_unref(obj);

    return FALSE;
}

static MenuRowItem menurow_find_selected(UiSkinnedMenurow * mr, gint x, gint y) {
    MenuRowItem ret = MENUROW_NONE;

    x = x/(mr->scaled ? cfg.scale_factor : 1);
    y = y/(mr->scaled ? cfg.scale_factor : 1);
    if (x > 0 && x < 8) {
        if (y >= 0 && y <= 10)
            ret = MENUROW_OPTIONS;
        if (y >= 10 && y <= 17)
            ret = MENUROW_ALWAYS;
        if (y >= 18 && y <= 25)
            ret = MENUROW_FILEINFOBOX;
        if (y >= 26 && y <= 33)
            ret = MENUROW_SCALE;
        if (y >= 34 && y <= 42)
            ret = MENUROW_VISUALIZATION;
    }
    return ret;
}

static gboolean ui_skinned_menurow_button_press(GtkWidget *widget, GdkEventButton *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_MENUROW (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW (widget);

    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 1) {

        menurow->pushed = TRUE;
        menurow->selected = menurow_find_selected(menurow, event->x, event->y);

        gtk_widget_queue_draw(widget);
        g_signal_emit_by_name(widget, "change", menurow->selected);
        }
    }

    return TRUE;
}

static gboolean ui_skinned_menurow_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW(widget);
    if (menurow->pushed) {
        menurow->pushed = FALSE;

        if (menurow->selected == MENUROW_ALWAYS)
            menurow->always_selected = !menurow->always_selected;

        if (menurow->selected == MENUROW_SCALE)
            menurow->scale_selected = !menurow->scale_selected;

        if ((int)(menurow->selected) != -1)
            g_signal_emit_by_name(widget, "release", menurow->selected, event);

        menurow->selected = MENUROW_NONE;
        gtk_widget_queue_draw(widget);
    }

    return TRUE;
}

static gboolean ui_skinned_menurow_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_MENUROW (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    UiSkinnedMenurow *menurow = UI_SKINNED_MENUROW(widget);

    if (menurow->pushed) {
        menurow->selected = menurow_find_selected(menurow, event->x, event->y);

        gtk_widget_queue_draw(widget);
        g_signal_emit_by_name(widget, "change", menurow->selected);
    }

    return TRUE;
}

static void ui_skinned_menurow_toggle_scaled(UiSkinnedMenurow *menurow) {
    GtkWidget *widget = GTK_WIDGET (menurow);

    menurow->scaled = !menurow->scaled;
    gtk_widget_set_size_request(widget, menurow->width* (menurow->scaled ? cfg.scale_factor : 1),
    menurow->height * (menurow->scaled ? cfg.scale_factor : 1));

    gtk_widget_queue_draw(GTK_WIDGET(menurow));
}
