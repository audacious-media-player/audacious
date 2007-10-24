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

#ifndef UISKINNEDMENUROW_H
#define UISKINNEDMENUROW_H

#include <gtk/gtk.h>
#include "skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_MENUROW(obj)          GTK_CHECK_CAST (obj, ui_skinned_menurow_get_type (), UiSkinnedMenurow)
#define UI_SKINNED_MENUROW_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_menurow_get_type (), UiSkinnedMenurowClass)
#define UI_SKINNED_IS_MENUROW(obj)       GTK_CHECK_TYPE (obj, ui_skinned_menurow_get_type ())

typedef struct _UiSkinnedMenurow        UiSkinnedMenurow;
typedef struct _UiSkinnedMenurowClass   UiSkinnedMenurowClass;

typedef enum {
    MENUROW_NONE, MENUROW_OPTIONS, MENUROW_ALWAYS, MENUROW_FILEINFOBOX,
    MENUROW_DOUBLESIZE, MENUROW_VISUALIZATION
} MenuRowItem;

struct _UiSkinnedMenurow {
    GtkWidget        widget;

    gint             x, y, width, height;
    GtkWidget        *fixed;
    gboolean         double_size;
    gint             nx, ny;
    gint             sx, sy;
    MenuRowItem      selected;
    gboolean         always_selected;
    gboolean         doublesize_selected;
    gboolean         pushed;
    SkinPixmapId     skin_index;
};

struct _UiSkinnedMenurowClass {
    GtkWidgetClass          parent_class;
    void (* doubled)        (UiSkinnedMenurow *menurow);
    void (* change)         (UiSkinnedMenurow *menurow);
    void (* release)        (UiSkinnedMenurow *menurow);
};

GtkWidget* ui_skinned_menurow_new (GtkWidget *fixed, gint x, gint y, gint nx, gint ny, gint sx, gint sy, SkinPixmapId si);
GtkType ui_skinned_menurow_get_type(void);

#ifdef __cplusplus
}
#endif

#endif
