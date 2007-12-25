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
#include "ui_skinned_playlist_slider.h"
#include "main.h"
#include "util.h"
#include "ui_playlist.h"

#define UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ui_skinned_playlist_slider_get_type(), UiSkinnedPlaylistSliderPrivate))
typedef struct _UiSkinnedPlaylistSliderPrivate UiSkinnedPlaylistSliderPrivate;

enum {
    REDRAW,
    LAST_SIGNAL
};

struct _UiSkinnedPlaylistSliderPrivate {
    SkinPixmapId     skin_index;
    gint             width, height;

    gint             resize_height;
    gint             move_x;
    gint             prev_y;
    gint             drag_y;
};

static void ui_skinned_playlist_slider_class_init         (UiSkinnedPlaylistSliderClass *klass);
static void ui_skinned_playlist_slider_init               (UiSkinnedPlaylistSlider *playlist_slider);
static void ui_skinned_playlist_slider_destroy            (GtkObject *object);
static void ui_skinned_playlist_slider_realize            (GtkWidget *widget);
static void ui_skinned_playlist_slider_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_playlist_slider_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_playlist_slider_expose         (GtkWidget *widget, GdkEventExpose *event);
static void ui_skinned_playlist_slider_set_position       (GtkWidget *widget, gint y);
static gboolean ui_skinned_playlist_slider_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_playlist_slider_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_playlist_slider_motion_notify  (GtkWidget *widget, GdkEventMotion *event);
static void ui_skinned_playlist_slider_redraw             (UiSkinnedPlaylistSlider *playlist_slider);

static GtkWidgetClass *parent_class = NULL;
static guint playlist_slider_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_playlist_slider_get_type() {
    static GType playlist_slider_type = 0;
    if (!playlist_slider_type) {
        static const GTypeInfo playlist_slider_info = {
            sizeof (UiSkinnedPlaylistSliderClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_playlist_slider_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedPlaylistSlider),
            0,
            (GInstanceInitFunc) ui_skinned_playlist_slider_init,
        };
        playlist_slider_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedPlaylistSlider", &playlist_slider_info, 0);
    }

    return playlist_slider_type;
}

static void ui_skinned_playlist_slider_class_init(UiSkinnedPlaylistSliderClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_playlist_slider_destroy;

    widget_class->realize = ui_skinned_playlist_slider_realize;
    widget_class->expose_event = ui_skinned_playlist_slider_expose;
    widget_class->size_request = ui_skinned_playlist_slider_size_request;
    widget_class->size_allocate = ui_skinned_playlist_slider_size_allocate;
    widget_class->button_press_event = ui_skinned_playlist_slider_button_press;
    widget_class->button_release_event = ui_skinned_playlist_slider_button_release;
    widget_class->motion_notify_event = ui_skinned_playlist_slider_motion_notify;

    klass->redraw = ui_skinned_playlist_slider_redraw;

    playlist_slider_signals[REDRAW] = 
        g_signal_new ("redraw", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedPlaylistSliderClass, redraw), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (gobject_class, sizeof (UiSkinnedPlaylistSliderPrivate));
}

static void ui_skinned_playlist_slider_init(UiSkinnedPlaylistSlider *playlist_slider) {
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(playlist_slider);
    playlist_slider->pressed = FALSE;
    priv->resize_height = 0;
    priv->move_x = 0;
    priv->drag_y = 0;
    priv->prev_y = 0;
}

GtkWidget* ui_skinned_playlist_slider_new(GtkWidget *fixed, gint x, gint y, gint h) {

    UiSkinnedPlaylistSlider *hs = g_object_new (ui_skinned_playlist_slider_get_type (), NULL);
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(hs);

    hs->x = x;
    hs->y = y;
    priv->width = 8;
    priv->height = h;
    priv->skin_index = SKIN_PLEDIT;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(hs), hs->x, hs->y);

    return GTK_WIDGET(hs);
}

static void ui_skinned_playlist_slider_destroy(GtkObject *object) {
    UiSkinnedPlaylistSlider *playlist_slider;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYLIST_SLIDER (object));

    playlist_slider = UI_SKINNED_PLAYLIST_SLIDER (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_playlist_slider_realize(GtkWidget *widget) {
    UiSkinnedPlaylistSlider *playlist_slider;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYLIST_SLIDER(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    playlist_slider = UI_SKINNED_PLAYLIST_SLIDER(widget);

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

static void ui_skinned_playlist_slider_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);

    requisition->width = priv->width;
    requisition->height = priv->height;
}

static void ui_skinned_playlist_slider_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedPlaylistSlider *playlist_slider = UI_SKINNED_PLAYLIST_SLIDER (widget);
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(playlist_slider);

    widget->allocation = *allocation;
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    if (playlist_slider->x + priv->move_x == widget->allocation.x)
        priv->move_x = 0;
    playlist_slider->x = widget->allocation.x;
    playlist_slider->y = widget->allocation.y;

    if (priv->height != widget->allocation.height) {
        priv->height = priv->height + priv->resize_height;
        priv->resize_height = 0;
        gtk_widget_queue_draw(widget);
    }
}

static gboolean ui_skinned_playlist_slider_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_PLAYLIST_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedPlaylistSlider *ps = UI_SKINNED_PLAYLIST_SLIDER (widget);
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(ps);
    g_return_val_if_fail (priv->width > 0 || priv->height > 0, FALSE);

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, priv->width, priv->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    gint num_visible;
    num_visible = playlistwin_list_get_visible_count();


    Playlist *playlist = playlist_get_active();

    gint y;
    if (playlist_get_length(playlist) > num_visible)
        y = (playlistwin_list_get_first() * (priv->height - 19)) /
            (playlist_get_length(playlist) - num_visible);
    else
        y = 0;

    if (y < 0) y=0;
    if (y > priv->height - 19) y = priv->height - 19;

    priv->prev_y = y;

    /* FIXME: uses bmp_active_skin->pixmaps directly and may need calibration */
    /* drawing background */
    gint c;
    for (c = 0; c <= priv->height / 29; c++) {
         gdk_draw_drawable(obj, gc, bmp_active_skin->pixmaps[SKIN_PLEDIT].pixmap,
                           36, 42, 0, c*29, priv->width, 29);
    }

    /* drawing knob */
    skin_draw_pixmap(widget, bmp_active_skin, obj, gc, priv->skin_index, ps->pressed ? 61 : 52, 53, 0, y, priv->width, 18);

    gdk_draw_drawable(widget->window, gc, obj, 0, 0, 0, 0, priv->width, priv->height);
    g_object_unref(obj);
    g_object_unref(gc);

    return FALSE;
}

static void ui_skinned_playlist_slider_set_position(GtkWidget *widget, gint y) {
    gint pos;
    Playlist *playlist = playlist_get_active();
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);

    y = CLAMP(y, 0, priv->height - 19);

    pos = (y * (playlist_get_length(playlist) - playlistwin_list_get_visible_count())) / (priv->height - 19);
    playlistwin_set_toprow(pos);

    gtk_widget_queue_draw(widget);
}

static gboolean ui_skinned_playlist_slider_button_press(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedPlaylistSlider *ps = UI_SKINNED_PLAYLIST_SLIDER (widget);
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);

    if (event->button != 1 && event->button != 2)
        return TRUE;

    gint y = event->y;
    if (event->type == GDK_BUTTON_PRESS) {
        ps->pressed = TRUE;
        if ((y >= priv->prev_y && y < priv->prev_y + 18)) {
            priv->drag_y = y - priv->prev_y;
        } else if (event->button == 2) {
            ui_skinned_playlist_slider_set_position(widget, y);
            priv->drag_y = 0;
        } else {
            gint n = playlistwin_list_get_visible_count() / 2;
            if (y < priv->prev_y)
                n *= -1;
            playlistwin_scroll(n);
        }
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean ui_skinned_playlist_slider_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedPlaylistSlider *ps = UI_SKINNED_PLAYLIST_SLIDER(widget);

    if (event->button == 1 || event->button == 2) {
        ps->pressed = FALSE;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean ui_skinned_playlist_slider_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    UiSkinnedPlaylistSlider *ps = UI_SKINNED_PLAYLIST_SLIDER(widget);
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);

    if (ps->pressed) {
        gint y = event->y - priv->drag_y;
        ui_skinned_playlist_slider_set_position(widget, y);
    }
    return TRUE;
}

static void ui_skinned_playlist_slider_redraw(UiSkinnedPlaylistSlider *playlist_slider) {
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(playlist_slider);

    if (priv->resize_height)
        gtk_widget_set_size_request(GTK_WIDGET(playlist_slider), priv->width, priv->height+priv->resize_height);
    if (priv->move_x)
        gtk_fixed_move(GTK_FIXED(gtk_widget_get_parent(GTK_WIDGET(playlist_slider))), GTK_WIDGET(playlist_slider),
                       playlist_slider->x+priv->move_x, playlist_slider->y);

    gtk_widget_queue_draw(GTK_WIDGET(playlist_slider));
}

void ui_skinned_playlist_slider_move_relative(GtkWidget *widget, gint x) {
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);
    priv->move_x += x;
}

void ui_skinned_playlist_slider_resize_relative(GtkWidget *widget, gint h) {
    UiSkinnedPlaylistSliderPrivate *priv = UI_SKINNED_PLAYLIST_SLIDER_GET_PRIVATE(widget);
    priv->resize_height += h;
}
