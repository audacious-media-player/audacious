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

#ifndef AUDACIOUS_UI_SKINNED_TEXTBOX_H
#define AUDACIOUS_UI_SKINNED_TEXTBOX_H

#include <gtk/gtk.h>
#include "ui_skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_TEXTBOX(obj)          GTK_CHECK_CAST (obj, ui_skinned_textbox_get_type (), UiSkinnedTextbox)
#define UI_SKINNED_TEXTBOX_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_textbox_get_type (), UiSkinnedTextboxClass)
#define UI_SKINNED_IS_TEXTBOX(obj)       GTK_CHECK_TYPE (obj, ui_skinned_textbox_get_type ())

typedef struct _UiSkinnedTextbox        UiSkinnedTextbox;
typedef struct _UiSkinnedTextboxClass   UiSkinnedTextboxClass;

struct _UiSkinnedTextbox {
    GtkWidget        widget;

    gint             x, y, width, height;
    gchar            *text;
};

struct _UiSkinnedTextboxClass {
    GtkWidgetClass          parent_class;
    void (* clicked)        (UiSkinnedTextbox *textbox);
    void (* double_clicked) (UiSkinnedTextbox *textbox);
    void (* right_clicked)  (UiSkinnedTextbox *textbox);
    void (* scaled)         (UiSkinnedTextbox *textbox);
    void (* redraw)         (UiSkinnedTextbox *textbox);
};

GtkWidget* ui_skinned_textbox_new (GtkWidget *fixed, gint x, gint y, gint w, gboolean allow_scroll, SkinPixmapId si);
GtkType ui_skinned_textbox_get_type(void);
void ui_skinned_textbox_set_xfont(GtkWidget *widget, gboolean use_xfont, const gchar * fontname);
void ui_skinned_textbox_set_text(GtkWidget *widget, const gchar *text);
void ui_skinned_textbox_set_scroll(GtkWidget *widget, gboolean scroll);
void ui_skinned_textbox_move_relative(GtkWidget *widget, gint x, gint y);

#ifdef __cplusplus
}
#endif

#endif /* AUDACIOUS_UI_SKINNED_TEXTBOX_H */
