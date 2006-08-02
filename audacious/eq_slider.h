/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef EQ_SLIDER_H
#define EQ_SLIDER_H

#include <glib.h>
#include <gdk/gdk.h>

#include "widget.h"

#define EQ_SLIDER(x)  ((EqSlider *)(x))
struct _EqSlider {
    Widget es_widget;
    gint es_position;
    gboolean es_isdragging;
    gint es_drag_y;
};

typedef struct _EqSlider EqSlider;

EqSlider *create_eqslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                          gint x, gint y);
void eqslider_set_position(EqSlider * es, gfloat pos);
gfloat eqslider_get_position(EqSlider * es);

#endif
