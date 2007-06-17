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

#ifndef UISKINNEDBUTTON_H
#define UISKINNEDBUTTON_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkenums.h>
#include "widgets/skin.h"

#define UI_TYPE_SKINNED_BUTTON            (ui_skinned_button_get_type())
#define UI_SKINNED_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UI_TYPE_SKINNED_BUTTON, UiSkinnedButton))
#define UI_SKINNED_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UI_TYPE_SKINNED_BUTTON, UiSkinnedButtonClass))
#define UI_IS_SKINNED_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UI_TYPE_SKINNED_BUTTON))
#define UI_IS_SKINNED_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UI_TYPE_SKINNED_BUTTON))
#define UI_SKINNED_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UI_TYPE_SKINNED_BUTTON, GtkFlatButtonClass))

typedef struct _UiSkinnedButton		UiSkinnedButton;
typedef struct _UiSkinnedButtonClass	UiSkinnedButtonClass;

enum {
	PRESSED,
	RELEASED,
	CLICKED,
	DOUBLED,
	REDRAW,
	LAST_SIGNAL
};

struct _UiSkinnedButton {
	GtkBin bin;

	GdkWindow *event_window;

	gboolean button_down : 1;
	gboolean pressed : 1;
	gboolean hover : 1;
	//Skinned part, used in ui_playlist.c
	gint x, y, nx, ny, px, py;

	gboolean redraw;
};

struct _UiSkinnedButtonClass {
	GtkBinClass parent_class;
	void (* pressed)  (UiSkinnedButton *button);
	void (* released) (UiSkinnedButton *button);
	void (* clicked)  (UiSkinnedButton *button);
	void (* doubled)  (UiSkinnedButton *button);
	void (* redraw)   (UiSkinnedButton *button);
};

GType ui_skinned_button_get_type(void) G_GNUC_CONST;
GtkWidget* ui_skinned_button_new();
void ui_skinned_push_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si);

#endif
