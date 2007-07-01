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

#ifndef UISKINNEDTEXTBOX_H
#define UISKINNEDTEXTBOX_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkenums.h>
#include "widgets/skin.h"

#define TEXTBOX_SCROLL_SMOOTH_TIMEOUT  30
#define TEXTBOX_SCROLL_WAIT            80

#define UI_TYPE_SKINNED_TEXTBOX            (ui_skinned_textbox_get_type())
#define UI_SKINNED_TEXTBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UI_TYPE_SKINNED_TEXTBOX, UiSkinnedTextbox))
#define UI_SKINNED_TEXTBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UI_TYPE_SKINNED_TEXTBOX, UiSkinnedTextboxClass))
#define UI_IS_SKINNED_TEXTBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UI_TYPE_SKINNED_TEXTBOX))
#define UI_IS_SKINNED_TEXTBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UI_TYPE_SKINNED_TEXTBOX))
#define UI_SKINNED_TEXTBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UI_TYPE_SKINNED_TEXTBOX, GtkFlatTextboxClass))

typedef struct _UiSkinnedTextbox	UiSkinnedTextbox;
typedef struct _UiSkinnedTextboxClass	UiSkinnedTextboxClass;

struct _UiSkinnedTextbox {
    GtkBin bin;
    GdkWindow *event_window;
    gint x, y, width, height;
    gboolean redraw;
    gchar *text;
};

struct _UiSkinnedTextboxClass {
    GtkBinClass parent_class;
    void (* clicked)        (UiSkinnedTextbox *textbox);
    void (* double_clicked) (UiSkinnedTextbox *textbox);
    void (* right_clicked)  (UiSkinnedTextbox *textbox);
    void (* doubled)        (UiSkinnedTextbox *textbox);
    void (* redraw)         (UiSkinnedTextbox *textbox);
};

GType ui_skinned_textbox_get_type(void) G_GNUC_CONST;
GtkWidget* ui_skinned_textbox_new();
void ui_skinned_textbox_setup(GtkWidget *widget, GtkWidget *fixed, GdkPixmap *parent, GdkGC *gc, gint x, gint y, gint w, gboolean allow_scroll, SkinPixmapId si);
void ui_skinned_textbox_set_text(GtkWidget *widget, const gchar *text);
void ui_skinned_textbox_set_xfont(GtkWidget *widget, gboolean use_xfont, const gchar *fontname);
void ui_skinned_textbox_set_scroll(GtkWidget *widget, gboolean scroll);
void ui_skinned_textbox_move_relative(GtkWidget *widget, gint x, gint y);
void ui_skinned_textbox_resize_relative(GtkWidget *widget, gint w, gint h);

#endif
