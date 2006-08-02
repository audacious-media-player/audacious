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
#ifndef PBUTTON_H
#define PBUTTON_H

#include <glib.h>
#include <gdk/gdk.h>

#include "widget.h"
#include "skin.h"

#define PBUTTON(x)  ((PButton *)(x))
struct _PButton {
    Widget pb_widget;
    gint pb_nx, pb_ny;
    gint pb_px, pb_py;
    gboolean pb_pressed;
    gboolean pb_inside;
    gboolean pb_allow_draw;
    void (*pb_push_cb) (void);
    void (*pb_release_cb) (void);
    SkinPixmapId pb_skin_index1, pb_skin_index2;
};

typedef struct _PButton PButton;

PButton *create_pbutton(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                        gint x, gint y, gint w, gint h, gint nx, gint ny,
                        gint px, gint py, void (*push_cb) (void), SkinPixmapId si);
PButton *create_pbutton_ex(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                           gint x, gint y, gint w, gint h, gint nx,
                           gint ny, gint px, gint py, void (*push_cb) (void),
			   void (*release_cb) (void), SkinPixmapId si1,
			   SkinPixmapId si2);
void free_pbutton(PButton * b);
void pbutton_set_skin_index(PButton * b, SkinPixmapId si);
void pbutton_set_skin_index1(PButton * b, SkinPixmapId si);
void pbutton_set_skin_index2(PButton * b, SkinPixmapId si);
void pbutton_set_button_data(PButton * b, gint nx, gint ny, gint px, gint py);

#endif
