/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _WIDGETCORE_H_
#error Please do not include me directly! Use widgetcore.h instead!
#endif

#ifndef HSLIDER_H
#define HSLIDER_H

#include <glib.h>
#include <gdk/gdk.h>

#include "skin.h"
#include "widget.h"

#define HSLIDER(x)  ((HSlider *)(x))
struct _HSlider {
    Widget hs_widget;
    gint hs_frame, hs_frame_offset, hs_frame_height, hs_min, hs_max;
    gint hs_knob_nx, hs_knob_ny, hs_knob_px, hs_knob_py;
    gint hs_knob_width, hs_knob_height;
    gint hs_position;
    gboolean hs_pressed;
    gint hs_pressed_x, hs_pressed_y;
     gint(*hs_frame_cb) (gint);
    void (*hs_motion_cb) (gint);
    void (*hs_release_cb) (gint);
    SkinPixmapId hs_skin_index;
};

typedef struct _HSlider HSlider;

HSlider *create_hslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                        gint x, gint y, gint w, gint h, gint knx, gint kny,
                        gint kpx, gint kpy, gint kw, gint kh, gint fh,
                        gint fo, gint min, gint max, gint(*fcb) (gint),
                        void (*mcb) (gint), void (*rcb) (gint),
                        SkinPixmapId si);

void hslider_set_position(HSlider * hs, gint pos);
gint hslider_get_position(HSlider * hs);

#endif
