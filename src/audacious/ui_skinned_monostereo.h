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

#ifndef UISKINNEDMONOSTEREO_H
#define UISKINNEDMONOSTEREO_H

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_MONOSTEREO(obj)          GTK_CHECK_CAST (obj, ui_skinned_monostereo_get_type (), UiSkinnedMonoStereo)
#define UI_SKINNED_MONOSTEREO_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_monostereo_get_type (), UiSkinnedMonoStereoClass)
#define UI_SKINNED_IS_MONOSTEREO(obj)       GTK_CHECK_TYPE (obj, ui_skinned_monostereo_get_type ())

typedef struct _UiSkinnedMonoStereo        UiSkinnedMonoStereo;
typedef struct _UiSkinnedMonoStereoClass   UiSkinnedMonoStereoClass;

struct _UiSkinnedMonoStereo {
    GtkWidget        widget;

    gint             x, y, width, height;
    gint             num_channels;
    SkinPixmapId     skin_index;
    GtkWidget        *fixed;
    gboolean         double_size;
};

struct _UiSkinnedMonoStereoClass {
    GtkWidgetClass          parent_class;
    void (* doubled)        (UiSkinnedMonoStereo *menurow);
};

GtkWidget* ui_skinned_monostereo_new (GtkWidget *fixed, gint x, gint y, SkinPixmapId si);
GtkType ui_skinned_monostereo_get_type(void);
void ui_skinned_monostereo_set_num_channels(GtkWidget *widget, gint nch);

#ifdef __cplusplus
}
#endif

#endif
