/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007  Audacious development team.
 *
 * Based on:
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2004  BMP development team.
 * XMMS:
 * Copyright (C) 1998-2003  XMMS development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
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

#include "widgets/widgetcore.h"
#include "ui_vis.h"
#include "main.h"
#include "util.h"
#include "strings.h"
#include "playback.h"
#include <string.h>
#include <ctype.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkimage.h>

#define UI_TYPE_VIS           (ui_vis_get_type())

static const gfloat vis_afalloff_speeds[] = { 0.34, 0.5, 1.0, 1.3, 1.6 };
static const gfloat vis_pfalloff_speeds[] = { 1.2, 1.3, 1.4, 1.5, 1.6 };
static const gint vis_redraw_delays[] = { 1, 2, 4, 8 };
static const guint8 vis_scope_colors[] =
    { 21, 21, 20, 20, 19, 19, 18, 19, 19, 20, 20, 21, 21 };
static guchar voiceprint_data[76*16];

enum {
    CLICKED,
    RIGHT_CLICKED,
    DOUBLED,
    REDRAW,
    LAST_SIGNAL
};

static void ui_vis_class_init         (UiVisClass *klass);
static void ui_vis_init               (UiVis *vis);
static void ui_vis_destroy            (GtkObject *object);
static void ui_vis_realize            (GtkWidget *widget);
static void ui_vis_unrealize          (GtkWidget *widget);
static void ui_vis_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_vis_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_vis_expose         (GtkWidget *widget, GdkEventExpose *event);
static gboolean ui_vis_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_vis_button_release (GtkWidget *widget, GdkEventButton *event);
static void ui_vis_toggle_doublesize  (UiVis *vis);

static GtkWidgetClass *parent_class = NULL;
static guint vis_signals[LAST_SIGNAL] = { 0 };

GType ui_vis_get_type() {
    static GType vis_type = 0;
    if (!vis_type) {
        static const GTypeInfo vis_info = {
            sizeof (UiVisClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_vis_class_init,
            NULL,
            NULL,
            sizeof (UiVis),
            0,
            (GInstanceInitFunc) ui_vis_init,
        };
        vis_type = g_type_register_static (GTK_TYPE_WIDGET, "UiVis", &vis_info, 0);
    }

    return vis_type;
}

static void ui_vis_class_init(UiVisClass *klass) {
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_vis_destroy;

    widget_class->realize = ui_vis_realize;
    widget_class->unrealize = ui_vis_unrealize;
    widget_class->expose_event = ui_vis_expose;
    widget_class->size_request = ui_vis_size_request;
    widget_class->size_allocate = ui_vis_size_allocate;
    widget_class->button_press_event = ui_vis_button_press;
    widget_class->button_release_event = ui_vis_button_release;

    klass->clicked = NULL;
    klass->right_clicked = NULL;
    klass->doubled = ui_vis_toggle_doublesize;

    vis_signals[CLICKED] = 
        g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiVisClass, clicked), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    vis_signals[RIGHT_CLICKED] = 
        g_signal_new ("right-clicked", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiVisClass, right_clicked), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    vis_signals[DOUBLED] = 
        g_signal_new ("toggle-double-size", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiVisClass, doubled), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ui_vis_init(UiVis *vis) {
    memset(voiceprint_data, 0, 16*76);
}

GtkWidget* ui_vis_new(GtkWidget *fixed, gint x, gint y, gint width) {
    UiVis *vis = g_object_new (ui_vis_get_type (), NULL);

    vis->x = x;
    vis->y = y;

    vis->width = width;
    vis->height = 16;

    vis->fixed = fixed;
    vis->double_size = FALSE;

    vis->visible_window = TRUE;

    gtk_fixed_put(GTK_FIXED(vis->fixed), GTK_WIDGET(vis), vis->x, vis->y);

    return GTK_WIDGET(vis);
}

static void ui_vis_destroy(GtkObject *object) {
    UiVis *vis;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_IS_VIS (object));

    vis = UI_VIS (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_vis_realize(GtkWidget *widget) {
    UiVis *vis;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_IS_VIS(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    vis = UI_VIS(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;

    if (vis->visible_window)
    {
      attributes.visual = gtk_widget_get_visual(widget);
      attributes.colormap = gtk_widget_get_colormap(widget);
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);
    }
    else
    {
      attributes.wclass = GDK_INPUT_ONLY;
      attributes_mask = GDK_WA_X | GDK_WA_Y;
      widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);
    }

    widget->style = gtk_style_attach(widget->style, widget->window);

    gdk_window_set_user_data(widget->window, widget);
}

static void ui_vis_unrealize(GtkWidget *widget) {
    UiVis *vis;
    vis = UI_VIS(widget);

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void ui_vis_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiVis *vis = UI_VIS(widget);

    requisition->width = vis->width*(1+vis->double_size);
    requisition->height = vis->height*(1+vis->double_size);
}

static void ui_vis_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiVis *vis = UI_VIS (widget);

    widget->allocation = *allocation;
    widget->allocation.x *= (1+vis->double_size);
    widget->allocation.y *= (1+vis->double_size);
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    vis->x = widget->allocation.x/(vis->double_size ? 2 : 1);
    vis->y = widget->allocation.y/(vis->double_size ? 2 : 1);
}

static gboolean ui_vis_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_IS_VIS (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiVis *vis = UI_VIS (widget);

    gint x, y, n, h = 0, h2;
    gfloat delta;
    guchar skin_col[2][3];
    guchar vis_color[24][3];
    guchar vis_voice_color[256][3], voice_c[3];
    guchar rgb_data[76 * 16 * 3 * 2 * 2], *ptr, c;
    guint32 colors[24];
    GdkColor *fgc, *bgc;
    GdkRgbCmap *cmap;

    if (!GTK_WIDGET_VISIBLE(widget))
        return FALSE;

    if (!vis->visible_window)
        return FALSE;

    skin_get_viscolor(bmp_active_skin, vis_color);
    for (y = 0; y < 24; y++) {
        colors[y] =
            vis_color[y][0] << 16 | vis_color[y][1] << 8 | vis_color[y][2];
    }
    cmap = gdk_rgb_cmap_new(colors, 24);

    if (!vis->double_size) {
      if(cfg.vis_type == VIS_VOICEPRINT /*&& cfg.voiceprint_mode != VOICEPRINT_NORMAL*/){
	memset(rgb_data, 0, 76 * 16 * 3);
      }
      else{
	memset(rgb_data, 0, 76 * 16);
	for (y = 1; y < 16; y += 2) {
	  ptr = rgb_data + (y * 76);
	  for (x = 0; x < 76; x += 2, ptr += 2)
	    *ptr = 1;
      }
      }
    }
    else{
      if(cfg.vis_type == VIS_VOICEPRINT /*&& cfg.voiceprint_mode != VOICEPRINT_NORMAL*/){
	memset(rgb_data, 0, 3 * 4 * 16 * 76);
      }
      else{
	memset(rgb_data, 0, 152 * 32);
	for (y = 1; y < 16; y += 2) {
	  ptr = rgb_data + (y * 304);
	  for (x = 0; x < 76; x += 2, ptr += 4) {
	    *ptr = 1;
	    *(ptr + 1) = 1;
	    *(ptr + 152) = 1;
	    *(ptr + 153) = 1;
	}
      }
      }
    }
    if (cfg.vis_type == VIS_ANALYZER) {
      for (x = 0; x < 75; x++) {
	if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
	  h = vis->data[x >> 2];
	else if (cfg.analyzer_type == ANALYZER_LINES)
	  h = vis->data[x];
	if (h && (cfg.analyzer_type == ANALYZER_LINES ||
		  (x % 4) != 3)) {
	  if (!vis->double_size) {
	    ptr = rgb_data + ((16 - h) * 76) + x;
	    switch (cfg.analyzer_mode) {
	    case ANALYZER_NORMAL:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = 18 - h + y;
	      break;
	    case ANALYZER_FIRE:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = y + 2;
	      break;
	    case ANALYZER_VLINES:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = 18 - h;
	      break;
	    }
	  }
	  else{
	    ptr = rgb_data + ((16 - h) * 304) + (x << 1);
	    switch (cfg.analyzer_mode) {
	    case ANALYZER_NORMAL:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = 18 - h + y;
		*(ptr + 1) = 18 - h + y;
		*(ptr + 152) = 18 - h + y;
		*(ptr + 153) = 18 - h + y;
	      }
	      break;
	    case ANALYZER_FIRE:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = y + 2;
		*(ptr + 1) = y + 2;
		*(ptr + 152) = y + 2;
		*(ptr + 153) = y + 2;
	      }
	      break;
	    case ANALYZER_VLINES:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = 18 - h;
		*(ptr + 1) = 18 - h;
		*(ptr + 152) = 18 - h;
		*(ptr + 153) = 18 - h;
	      }
	      
	      break;
	    }
	  }
	}
      }
      if (cfg.analyzer_peaks) {
	for (x = 0; x < 75; x++) {
	  if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
	    h = vis->peak[x >> 2];
	  else if (cfg.analyzer_type == ANALYZER_LINES)
	    h = vis->peak[x];
	  if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3)){
	    
	    if (!vis->double_size) {
	      rgb_data[(16 - h) * 76 + x] = 23;
	    }
	    else{
	      ptr = rgb_data + (16 - h) * 304 + (x << 1);
	      *ptr = 23;
	      *(ptr + 1) = 23;
	      *(ptr + 152) = 23;
	      *(ptr + 153) = 23;
	    }
	  }
	}
      }
    }
    else if (cfg.vis_type == VIS_VOICEPRINT) {
      if(!playback_get_paused() && playback_get_playing()){/*Don't scroll when it's paused or stopped*/
	for (y = 0; y < 16; y ++)
	  for (x = 75; x > 0; x--)
	    voiceprint_data[x + y * 76] = voiceprint_data[x-1+y*76];
	  for(y=0;y<16;y++)
	    voiceprint_data[y * 76] = vis->data[y];
      }
      if(playback_get_playing()){ /*Only draw the data if we're playing*/
	if(cfg.voiceprint_mode == VOICEPRINT_NORMAL){ 
	  /* Create color gradient from the skin's background- and foreground color*/
	  fgc = skin_get_color(bmp_active_skin, SKIN_TEXTFG);
	  bgc = skin_get_color(bmp_active_skin, SKIN_TEXTBG);
	  skin_col[0][0] = fgc->red   >> 8;
	  skin_col[0][1] = fgc->green >> 8;
	  skin_col[0][2] = fgc->blue  >> 8;
	  skin_col[1][0] = bgc->red   >> 8;
	  skin_col[1][1] = bgc->green >> 8;
	  skin_col[1][2] = bgc->blue  >> 8;
	  for(n=0;n<3;n++){
	    for(x=0;x<256;x++){
	      if(skin_col[0][n] > skin_col[1][n]){
		delta = (gfloat)(skin_col[0][n] - skin_col[1][n]) / 256.0;
		vis_voice_color[x][n] = skin_col[1][n] + (gfloat)(delta * x);
	      }
	      else if(skin_col[0][n] == skin_col[1][n]){
		vis_voice_color[x][n] = skin_col[0][n];
	      }
	      else{
		delta = (gfloat)(skin_col[1][n] - skin_col[0][n]) / 256.0;
		vis_voice_color[x][n] = skin_col[1][n] - (gfloat)(delta * x);
	      }
	    }
	  }
	}
	for (y = 0; y < 16; y ++){
	  for (x = 0; x < 76; x++){
	    guint8 d = voiceprint_data[x + y*76];
	    
	    if(cfg.voiceprint_mode == VOICEPRINT_NORMAL){
	      voice_c[0] = vis_voice_color[d][0];
	      voice_c[1] = vis_voice_color[d][1];
	      voice_c[2] = vis_voice_color[d][2];
	    }
	    else if(cfg.voiceprint_mode == VOICEPRINT_FIRE){
	      voice_c[0] = d < 64 ? (d * 2) : 255;
	      voice_c[1] = d < 64 ? 0 : (d < 128 ? (d-64) * 2 : 255);
	      voice_c[2] = d < 128 ? 0 : (d-128) * 2;
	      /* Test for black->blue->green->red. Isn't pretty, though...
		 voice_c[0] = d > 192 ? (d - 192) << 2 : 0;
		 voice_c[1] = d > 64 ? (d < 128 ? (d - 64) << 2 : (d < 192 ? (192 - d) << 2 : 0)) : 0;
		 voice_c[2] = d < 64 ? d << 2 : (d < 128 ? (128 - d) << 2 : 0);
	      */
	    }
	    else if(cfg.voiceprint_mode == VOICEPRINT_ICE){	    
	      voice_c[0] = d;
	      voice_c[1] = d < 128 ? d * 2 : 255;
	      voice_c[2] = d < 64 ? d * 4 : 255; 
	    }
	    if(!vis->double_size){
	      for(n=0;n<3;n++)
		rgb_data[x * 3 + y * 76*3+n] = voice_c[n];
	    }
	    else{
	      ptr = rgb_data + x * 3 * 2 + y * 2 * 76 * 3 * 2;
	      for(n=0;n<3;n++)
		{
		  *(ptr + n) = voice_c[n];
		  *(ptr + n + 3) = voice_c[n];
		  *(ptr + n + 76 * 2 * 3) = voice_c[n];
		  *(ptr + n + 3 + 76 * 2 * 3) = voice_c[n];
		}
	    }
	  }
	}
      }
    }
    if (cfg.vis_type == VIS_SCOPE) {
      for (x = 0; x < 75; x++) {
	switch (cfg.scope_mode) {
	case SCOPE_DOT:
	  h = vis->data[x];
	  if (!vis->double_size) {
	  ptr = rgb_data + ((14 - h) * 76) + x;
	    *ptr = vis_scope_colors[h + 1];
	  }else{
	    ptr = rgb_data + ((14 - h) * 304) + (x << 1);
	    *ptr = vis_scope_colors[h + 1];
	    *(ptr + 1) = vis_scope_colors[h + 1];
	    *(ptr + 152) = vis_scope_colors[h + 1];
	    *(ptr + 153) = vis_scope_colors[h + 1];
	  }
	  break;
	case SCOPE_LINE:
	  if (x != 74) {
	    h = 14 - vis->data[x];
	    h2 = 14 - vis->data[x + 1];
	    if (h > h2) {
	      y = h;
	      h = h2;
	      h2 = y;
	    }
	    if (!vis->double_size) {
	    ptr = rgb_data + (h * 76) + x;
	    for (y = h; y <= h2; y++, ptr += 76)
	      *ptr = vis_scope_colors[y - 2];
	    }
	    else{
	      ptr = rgb_data + (h * 304) + (x << 1);
	      for (y = h; y <= h2; y++, ptr += 304) {
		*ptr = vis_scope_colors[y - 2];
		*(ptr + 1) = vis_scope_colors[y - 2];
		*(ptr + 152) = vis_scope_colors[y - 2];
		*(ptr + 153) = vis_scope_colors[y - 2];
	      }
	    }
	  }
	  else {
	    h = 14 - vis->data[x];
	    if (!vis->double_size) {
	      ptr = rgb_data + (h * 76) + x;
	      *ptr = vis_scope_colors[h + 1];
	    }else{
	      ptr = rgb_data + (h * 304) + (x << 1);
	      *ptr = vis_scope_colors[h + 1];
	      *(ptr + 1) = vis_scope_colors[h + 1];
	      *(ptr + 152) = vis_scope_colors[h + 1];
	      *(ptr + 153) = vis_scope_colors[h + 1];
	    }
	  }
	  break;
	case SCOPE_SOLID:
	  h = 14 - vis->data[x];
	  h2 = 8;
	  c = vis_scope_colors[(gint) vis->data[x]];
	  if (h > h2) {
	    y = h;
	    h = h2;
	    h2 = y;
	  }
	  if (!vis->double_size) {
	    ptr = rgb_data + (h * 76) + x;
	    for (y = h; y <= h2; y++, ptr += 76)
	      *ptr = c;
	  }else{
	    ptr = rgb_data + (h * 304) + (x << 1);
	    for (y = h; y <= h2; y++, ptr += 304) {
	      *ptr = c;
	      *(ptr + 1) = c;
	      *(ptr + 152) = c;
	      *(ptr + 153) = c;
	    }
	  }
	  break;
	}
      }
    }

    GdkPixmap *obj = NULL;
    GdkGC *gc;
    obj = gdk_pixmap_new(NULL, vis->width*(1+vis->double_size), vis->height*(1+vis->double_size), gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    if (!vis->double_size) {
        if (cfg.vis_type == VIS_VOICEPRINT) {
            gdk_draw_rgb_image(obj, gc, 0, 0, vis->width, vis->height,
                               GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data,
                               76 * 3);
        } else {
            gdk_draw_indexed_image(obj, gc, 0, 0, vis->width, vis->height,
                                   GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data,
                                   76 , cmap);
        }
    } else {
        if (cfg.vis_type == VIS_VOICEPRINT) {
            gdk_draw_rgb_image(obj, gc, 0 << 1, 0 << 1,
                               vis->width << 1, vis->height << 1,
                               GDK_RGB_DITHER_NONE, (guchar *) rgb_data,
                               76 * 2 * 3);
        } else {
            gdk_draw_indexed_image(obj, gc, 0 << 1, 0 << 1,
                                   vis->width << 1, vis->height << 1,
                                   GDK_RGB_DITHER_NONE, (guchar *) rgb_data,
                                   76 * 2 , cmap);
        }
    }

    gdk_draw_drawable (widget->window, gc, obj, 0, 0, 0, 0,
                       vis->width*(1+vis->double_size), vis->height*(1+vis->double_size));
    g_object_unref(obj);
    g_object_unref(gc);
    gdk_rgb_cmap_free(cmap);
    return FALSE;
}

static gboolean ui_vis_button_press(GtkWidget *widget, GdkEventButton *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_IS_VIS (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 1) {
                g_signal_emit(widget, vis_signals[CLICKED], 0);
        }
    }

    return TRUE;
}

static gboolean ui_vis_button_release(GtkWidget *widget, GdkEventButton *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_IS_VIS (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->button == 3)
        g_signal_emit(widget, vis_signals[RIGHT_CLICKED], 0);

    return TRUE;
}

static void ui_vis_toggle_doublesize(UiVis *vis) {
    GtkWidget *widget = GTK_WIDGET (vis);
    vis->double_size = !vis->double_size;

    gtk_widget_set_size_request(widget, vis->width*(1+vis->double_size), vis->height*(1+vis->double_size));

    gtk_widget_queue_draw(GTK_WIDGET(vis));
}

void ui_vis_draw_pixel(GtkWidget *widget, guchar* texture, gint x, gint y, guint8 colour) {
    UiVis *vis = UI_VIS (widget);
    if (vis->double_size){
        texture[y * 76 + x] = colour;
        texture[y * 76 + x + 1] = colour;
        texture[y * 76 * 4 + x] = colour;
        texture[y * 76 * 4 + x + 1] = colour;
    } else {
        texture[y * 76 + x] = colour;
    }
}

void ui_vis_set_visible(GtkWidget *widget, gboolean window_is_visible)
{
    UiVis *vis;
    gboolean widget_is_visible;

    g_return_if_fail(UI_IS_VIS(widget));

    vis = UI_VIS (widget);
    widget_is_visible = GTK_WIDGET_VISIBLE(widget);

    vis->visible_window = window_is_visible;

    if (GTK_WIDGET_REALIZED (widget))
    {
        if ( widget_is_visible )
            gtk_widget_hide(widget);

        gtk_widget_unrealize(widget);
        gtk_widget_realize(widget);

        if ( widget_is_visible )
            gtk_widget_show(widget);
    }

    if (widget_is_visible)
        gtk_widget_queue_resize(widget);
}

void ui_vis_clear(GtkWidget *widget) {
    UiVis *vis = UI_VIS (widget);
        gdk_window_clear_area(widget->window, 0,
                              0, vis->width,
                              vis->height);
}

void ui_vis_clear_data(GtkWidget *widget) {
    gint i;
    UiVis *vis = UI_VIS (widget);

    memset(voiceprint_data, 0, 16*76);
    for (i = 0; i < 75; i++) {
        vis->data[i] = (cfg.vis_type == VIS_SCOPE) ? 6 : 0;
        vis->peak[i] = 0;
    }
}

void ui_vis_timeout_func(GtkWidget *widget, guchar * data) {
    UiVis *vis = UI_VIS (widget);
    static GTimer *timer = NULL;
    gulong micros = 9999999;
    gboolean falloff = FALSE;
    gint i;

    if (!timer) {
        timer = g_timer_new();
        g_timer_start(timer);
    }
    else {
      g_timer_elapsed(timer, &micros);
      if (micros > 14000)
	g_timer_reset(timer);
    }
    if (cfg.vis_type == VIS_ANALYZER) {
        if (micros > 14000)
            falloff = TRUE;
        if (data || falloff) {
            for (i = 0; i < 75; i++) {
                if (data && data[i] > vis->data[i]) {
                    vis->data[i] = data[i];
                    if (vis->data[i] > vis->peak[i]) {
                        vis->peak[i] = vis->data[i];
                        vis->peak_speed[i] = 0.01;

                    }
                    else if (vis->peak[i] > 0.0) {
                        vis->peak[i] -= vis->peak_speed[i];
                        vis->peak_speed[i] *=
                            vis_pfalloff_speeds[cfg.peaks_falloff];
                        if (vis->peak[i] < vis->data[i])
                            vis->peak[i] = vis->data[i];
                        if (vis->peak[i] < 0.0)
                            vis->peak[i] = 0.0;
                    }
                }
                else if (falloff) {
                    if (vis->data[i] > 0.0) {
                        vis->data[i] -=
                            vis_afalloff_speeds[cfg.analyzer_falloff];
                        if (vis->data[i] < 0.0)
                            vis->data[i] = 0.0;
                    }
                    if (vis->peak[i] > 0.0) {
                        vis->peak[i] -= vis->peak_speed[i];
                        vis->peak_speed[i] *=
                            vis_pfalloff_speeds[cfg.peaks_falloff];
                        if (vis->peak[i] < vis->data[i])
                            vis->peak[i] = vis->data[i];
                        if (vis->peak[i] < 0.0)
                            vis->peak[i] = 0.0;
                    }
                }
            }
        }
    }
    else if (cfg.vis_type == VIS_VOICEPRINT && data){
      for(i = 0; i < 16; i++)
	{
	  vis->data[i] = data[15 - i];
	}
    }
    else if (data) {
        for (i = 0; i < 75; i++)
            vis->data[i] = data[i];
    }

    if (micros > 14000) {
        if (!vis->refresh_delay) {
            gtk_widget_queue_draw(widget);
            vis->refresh_delay = vis_redraw_delays[cfg.vis_refresh];
        }
        vis->refresh_delay--;
    }
}
