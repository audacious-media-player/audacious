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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef SVIS_H
#define SVIS_H

#include <glib.h>
#include <gdk/gdk.h>

#include "svis.h"
#include "widget.h"

#define SVIS(x)  ((SVis *)(x))
struct _SVis {
    Widget vs_widget;
    gint vs_data[75];
    gint vs_refresh_delay;
};

typedef struct _SVis SVis;

void svis_draw(Widget * w);
void svis_timeout_func(SVis * svis, guchar * data);
SVis *create_svis(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x,
                  gint y);
void svis_set_data(SVis * vis, guchar * data);
void svis_clear_data(SVis * vis);
void svis_clear(SVis * vis);

#endif
