/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007  Audacious development team.
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

#ifndef UISKINNEDBUTTON_H
#define UISKINNEDBUTTON_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkenums.h>
#include "widgets/skin.h"

#define UI_TYPE_SKINNED_BUTTON            (ui_skinned_button_get_type())
#define UI_SKINNED_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UI_TYPE_SKINNED_BUTTON, UiSkinnedButton))
#define UI_SKINNED_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UI_TYPE_SKINNED_BUTTON, UiSkinnedButtonClass))
#define UI_SKINNED_IS_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UI_TYPE_SKINNED_BUTTON))
#define UI_IS_SKINNED_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UI_TYPE_SKINNED_BUTTON))
#define UI_SKINNED_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UI_TYPE_SKINNED_BUTTON, GtkFlatButtonClass))

typedef struct _UiSkinnedButton		UiSkinnedButton;
typedef struct _UiSkinnedButtonClass	UiSkinnedButtonClass;

enum {
	TYPE_NOT_SET,
	TYPE_PUSH,
	TYPE_TOGGLE,
	TYPE_SMALL
};

struct _UiSkinnedButton {
	GtkWidget widget;

	GdkWindow *event_window;

	gboolean button_down : 1;
	gboolean pressed : 1;
	gboolean hover : 1;
	gboolean inside : 1;
	gint type;
	//Skinned part, used in ui_playlist.c
	gint x, y, nx, ny, px, py;

	//Toogle button needs also those
	gint pnx, pny, ppx, ppy;

	gboolean redraw;
};

struct _UiSkinnedButtonClass {
	GtkWidgetClass          parent_class;
	void (* pressed)       (UiSkinnedButton *button);
	void (* released)      (UiSkinnedButton *button);
	void (* clicked)       (UiSkinnedButton *button);
	void (* right_clicked) (UiSkinnedButton *button);
	void (* doubled)       (UiSkinnedButton *button);
	void (* redraw)        (UiSkinnedButton *button);
};

GType ui_skinned_button_get_type(void) G_GNUC_CONST;
GtkWidget* ui_skinned_button_new();
void ui_skinned_push_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si);
void ui_skinned_set_push_button_data(GtkWidget *button, gint nx, gint ny, gint px, gint py);
void ui_skinned_toggle_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, gint pnx, gint pny, gint ppx, gint ppy, SkinPixmapId si);
void ui_skinned_small_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h);
void ui_skinned_button_set_skin_index(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index1(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index2(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_move_relative(GtkWidget *button, gint x, gint y);

#endif
