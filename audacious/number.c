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

#include "number.h"

#include <glib.h>
#include <gdk/gdk.h>

#include "skin.h"

void
number_set_number(Number * nu,
                  gint number)
{
    if (number == nu->nu_number)
        return;

    nu->nu_number = number;
    widget_draw(WIDGET(nu));
}

void
number_draw(Widget * w)
{
    Number *nu = NUMBER(w);
    GdkPixmap *obj;

    obj = nu->nu_widget.parent;

    if (nu->nu_number <= 11)
        skin_draw_pixmap(bmp_active_skin, obj, nu->nu_widget.gc,
                         nu->nu_skin_index, nu->nu_number * 9, 0,
                         nu->nu_widget.x, nu->nu_widget.y, 9, 13);
    else
        skin_draw_pixmap(bmp_active_skin, obj, nu->nu_widget.gc,
                         nu->nu_skin_index, 90, 0, nu->nu_widget.x,
                         nu->nu_widget.y, 9, 13);
}

Number *
create_number(GList ** wlist,
              GdkPixmap * parent,
              GdkGC * gc,
              gint x, gint y,
              SkinPixmapId si)
{
    Number *nu;

    nu = g_new0(Number, 1);
    widget_init(&nu->nu_widget, parent, gc, x, y, 9, 13, 1);
    nu->nu_widget.draw = number_draw;
    nu->nu_number = 10;
    nu->nu_skin_index = si;

    widget_list_add(wlist, WIDGET(nu));
    return nu;
}
