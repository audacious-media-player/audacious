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

#ifndef _WIDGETCORE_H_
#error Please do not include me directly! Use widgetcore.h instead!
#endif

#ifndef AUDAPBUTTON_H
#define AUDAPBUTTON_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkenums.h>
#include "skin.h"
#include "widget.h"

#define AUDACIOUS_TYPE_PBUTTON		(audacious_pbutton_get_type())
#define AUDACIOUS_PBUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), AUDACIOUS_TYPE_PBUTTON, AudaciousPButton))
#define AUDACIOUS_PBUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AUDACIOUS_TYPE_PBUTTON, AudaciousPButtonClass))
#define AUDACIOUS_IS_PBUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUDACIOUS_TYPE_PBUTTON))
#define AUDACIOUS_IS_PBUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AUDACIOUS_TYPE_PBUTTON))
#define AUDACIOUS_PBUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), AUDACIOUS_TYPE_PBUTTON, GtkFlatButtonClass))

typedef struct _AudaciousPButton		AudaciousPButton;
typedef struct _AudaciousPButtonClass	AudaciousPButtonClass;

enum {
	PRESSED,
	RELEASED,
	CLICKED,
	DOUBLED,
	REDRAW,
	LAST_SIGNAL
};

struct _AudaciousPButton {
	GtkBin bin;

	GdkWindow *event_window;

	gboolean button_down : 1;
	gboolean pressed : 1;
	gboolean hover : 1;
	//Skinned part, used in ui_playlist.c
	gint x, y, nx, ny, px, py;
};

struct _AudaciousPButtonClass {
	GtkBinClass parent_class;
	void (* pressed)  (AudaciousPButton *button);
	void (* released) (AudaciousPButton *button);
	void (* clicked)  (AudaciousPButton *button);
	void (* doubled)  (AudaciousPButton *button);
	void (* redraw)   (AudaciousPButton *button);
};

GType audacious_pbutton_get_type(void) G_GNUC_CONST;
GtkWidget* audacious_pbutton_new();
void audacious_pbutton_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si);
void audacious_pbutton_pressed(AudaciousPButton *button);
void audacious_pbutton_released(AudaciousPButton *button);
void audacious_pbutton_clicked(AudaciousPButton *button);

void _audacious_pbutton_set_pressed(AudaciousPButton *button, gboolean pressed);

#endif
