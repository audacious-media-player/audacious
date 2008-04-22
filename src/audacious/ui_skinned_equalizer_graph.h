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

#ifndef UISKINNEDEQUALIZERGRAPH_H
#define UISKINNEDEQUALIZERGRAPH_H

#include <gtk/gtk.h>
#include "ui_skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_EQUALIZER_GRAPH(obj)          GTK_CHECK_CAST (obj, ui_skinned_equalizer_graph_get_type (), UiSkinnedEqualizerGraph)
#define UI_SKINNED_EQUALIZER_GRAPH_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_equalizer_graph_get_type (), UiSkinnedEqualizerGraphClass)
#define UI_SKINNED_IS_EQUALIZER_GRAPH(obj)       GTK_CHECK_TYPE (obj, ui_skinned_equalizer_graph_get_type ())

typedef struct _UiSkinnedEqualizerGraph        UiSkinnedEqualizerGraph;
typedef struct _UiSkinnedEqualizerGraphClass   UiSkinnedEqualizerGraphClass;

struct _UiSkinnedEqualizerGraph {
    GtkWidget        widget;

    gint             x, y, width, height;
    SkinPixmapId     skin_index;
    gboolean         scaled;
};

struct _UiSkinnedEqualizerGraphClass {
    GtkWidgetClass          parent_class;
    void (* scaled)        (UiSkinnedEqualizerGraph *eq_graph);
};

GtkWidget* ui_skinned_equalizer_graph_new(GtkWidget *fixed, gint x, gint y);

#ifdef __cplusplus
}
#endif

#endif
