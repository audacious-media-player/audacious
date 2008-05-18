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

#ifndef AUDACIOUS_UI_SKINNED_NUMBER_H
#define AUDACIOUS_UI_SKINNED_NUMBER_H

#include <gtk/gtk.h>
#include "ui_skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_NUMBER(obj)          GTK_CHECK_CAST (obj, ui_skinned_number_get_type (), UiSkinnedNumber)
#define UI_SKINNED_NUMBER_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_number_get_type (), UiSkinnedNumberClass)
#define UI_SKINNED_IS_NUMBER(obj)       GTK_CHECK_TYPE (obj, ui_skinned_number_get_type ())

typedef struct _UiSkinnedNumber        UiSkinnedNumber;
typedef struct _UiSkinnedNumberClass   UiSkinnedNumberClass;

struct _UiSkinnedNumber {
    GtkWidget        widget;

    gint             x, y, width, height;
    gint             num;
    gboolean         scaled;
    SkinPixmapId     skin_index;
};

struct _UiSkinnedNumberClass {
    GtkWidgetClass          parent_class;
    void (* scaled)        (UiSkinnedNumber *textbox);
};

GtkWidget* ui_skinned_number_new (GtkWidget *fixed, gint x, gint y, SkinPixmapId si);
GtkType ui_skinned_number_get_type(void);
void ui_skinned_number_set_number(GtkWidget *widget, gint num);
void ui_skinned_number_set_size(GtkWidget *widget, gint width, gint height);

#ifdef __cplusplus
}
#endif

#endif /* AUDACIOUS_UI_SKINNED_NUMBER_H */
