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

#ifndef UISKINNEDBUTTON_H
#define UISKINNEDBUTTON_H

#include <gtk/gtk.h>
#include "skin.h"

#define UI_SKINNED_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ui_skinned_button_get_type(), UiSkinnedButton))
#define UI_SKINNED_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  ui_skinned_button_get_type(), UiSkinnedButtonClass))
#define UI_SKINNED_IS_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ui_skinned_button_get_type()))

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
    gboolean button_down;
    gboolean pressed;
    gboolean hover;
    gboolean inside;
    gint type;
    gint x, y;
};

struct _UiSkinnedButtonClass {
    GtkWidgetClass          parent_class;
    void (* pressed)       (UiSkinnedButton *button);
    void (* released)      (UiSkinnedButton *button);
    void (* clicked)       (UiSkinnedButton *button);
    void (* right_clicked) (UiSkinnedButton *button);
    void (* scaled)        (UiSkinnedButton *button);
    void (* redraw)        (UiSkinnedButton *button);
};

GType ui_skinned_button_get_type(void) G_GNUC_CONST;
GtkWidget* ui_skinned_button_new();
void ui_skinned_push_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, SkinPixmapId si);
void ui_skinned_set_push_button_data(GtkWidget *button, gint nx, gint ny, gint px, gint py);
void ui_skinned_toggle_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h, gint nx, gint ny, gint px, gint py, gint pnx, gint pny, gint ppx, gint ppy, SkinPixmapId si);
void ui_skinned_small_button_setup(GtkWidget *button, GtkWidget *fixed, gint x, gint y, gint w, gint h);
void ui_skinned_button_set_skin_index(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index1(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_set_skin_index2(GtkWidget *button, SkinPixmapId si);
void ui_skinned_button_move_relative(GtkWidget *button, gint x, gint y);

#endif
