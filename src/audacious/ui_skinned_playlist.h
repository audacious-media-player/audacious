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

#ifndef UISKINNEDPLAYLIST_H
#define UISKINNEDPLAYLIST_H

#include <gtk/gtk.h>
#include "skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_SKINNED_PLAYLIST(obj)          GTK_CHECK_CAST (obj, ui_skinned_playlist_get_type (), UiSkinnedPlaylist)
#define UI_SKINNED_PLAYLIST_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_playlist_get_type (), UiSkinnedPlaylistClass)
#define UI_SKINNED_IS_PLAYLIST(obj)       GTK_CHECK_TYPE (obj, ui_skinned_playlist_get_type ())

typedef struct _UiSkinnedPlaylist        UiSkinnedPlaylist;
typedef struct _UiSkinnedPlaylistClass   UiSkinnedPlaylistClass;

struct _UiSkinnedPlaylist {
    GtkWidget   widget;
    gboolean    pressed;
    gint        x, y;
    gint        first;
    gint        num_visible;
    gint        prev_selected, prev_min, prev_max;
    gboolean    drag_motion;
    gint        drag_motion_x, drag_motion_y;
    gint        fheight;
};

struct _UiSkinnedPlaylistClass {
    GtkWidgetClass    parent_class;
    void (* redraw)   (UiSkinnedPlaylist *playlist);
};

GtkWidget* ui_skinned_playlist_new(GtkWidget *fixed, gint x, gint y, gint w, gint h);
GtkType ui_skinned_playlist_get_type(void);
void ui_skinned_playlist_resize_relative(GtkWidget *widget, gint w, gint h);
void ui_skinned_playlist_set_font(const gchar * font);
void ui_skinned_playlist_move_up(UiSkinnedPlaylist *pl);
void ui_skinned_playlist_move_down(UiSkinnedPlaylist *pl);
gint ui_skinned_playlist_get_position(GtkWidget *widget, gint x, gint y);

#ifdef __cplusplus
}
#endif

#endif
