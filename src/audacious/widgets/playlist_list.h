/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _WIDGETCORE_H_
#error Please do not include me directly! Use widgetcore.h instead!
#endif

#ifndef PLAYLIST_LIST_H
#define PLAYLIST_LIST_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "widget.h"

#define PLAYLIST_LIST(x)    ((PlayList_List *)(x))
struct _PlayList_List {
    Widget pl_widget;
    gint pl_first, pl_fheight, pl_prev_selected, pl_prev_min, pl_prev_max;
    gint pl_num_visible, pl_drag_pos;
    gboolean pl_dragging, pl_auto_drag_down, pl_auto_drag_up;
    gint pl_auto_drag_up_tag, pl_auto_drag_down_tag;
    gboolean pl_drag_motion;
    gint drag_motion_x, drag_motion_y;
    gboolean pl_tooltips;
};

typedef struct _PlayList_List PlayList_List;

PlayList_List *create_playlist_list(GList ** wlist, GdkPixmap * parent,
                                    GdkGC * gc, gint x, gint y, gint w,
                                    gint h);
void playlist_list_move_up(PlayList_List * pl);
void playlist_list_move_down(PlayList_List * pl);
int playlist_list_get_playlist_position(PlayList_List * pl, gint x, gint y);
void playlist_list_set_font(const gchar * font);
GdkPixmap *rootpix;
GdkPixmap *shade_pixmap(GdkDrawable *in, gint x, gint y, gint x_offset, gint y_offset, gint w, gint h, GdkColor *shade_color);
GdkDrawable *get_transparency_pixmap(void);

#endif
