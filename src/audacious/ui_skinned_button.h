/*
 * audacious: Cross-platform multimedia player.
 * ui_skinned_button.h: Skinned WA2 buttons                    
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

enum {
	TYPE_NOT_SET,
	TYPE_PUSH,
	TYPE_TOGGLE,
	TYPE_SMALL
};

struct _UiSkinnedButton {
	GtkBin bin;

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
void ui_skinned_set_push_button_data(GtkWidget *button, gint nx, gint ny, gint px, gint py);
void ui_skinned_toggle_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, gint pnx, gint pny, gint ppx, gint ppy, SkinPixmapId si);
void ui_skinned_small_button_setup(GtkWidget *button, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gint h);
void ui_skinned_button_set_skin_index(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index1(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index2(GtkWidget *button, SkinPixmapId si);

#endif
