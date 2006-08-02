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

#include "monostereo.h"

#include <glib.h>
#include <gdk/gdk.h>

#include "skin.h"
#include "widget.h"

void
monostereo_draw(Widget * widget)
{
    MonoStereo *ms = (MonoStereo *) widget;
    GdkPixmap *obj;

    obj = ms->ms_widget.parent;

    switch (ms->ms_num_channels) {
    case 0:
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 29, 12,
                         ms->ms_widget.x, ms->ms_widget.y, 27, 12);
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 0, 12,
                         ms->ms_widget.x + 27, ms->ms_widget.y, 29, 12);
        break;
    case 1:
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 29, 0,
                         ms->ms_widget.x, ms->ms_widget.y, 27, 12);
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 0, 12,
                         ms->ms_widget.x + 27, ms->ms_widget.y, 29, 12);
        break;
    case 2:
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 29, 12,
                         ms->ms_widget.x, ms->ms_widget.y, 27, 12);
        skin_draw_pixmap(bmp_active_skin, obj, ms->ms_widget.gc,
                         ms->ms_skin_index, 0, 0,
                         ms->ms_widget.x + 27, ms->ms_widget.y, 29, 12);
        break;
    }
}

void
monostereo_set_num_channels(MonoStereo * ms,
                            gint nch)
{
    if (!ms)
        return;

    ms->ms_num_channels = nch;
    widget_draw(WIDGET(ms));
}

MonoStereo *
create_monostereo(GList ** wlist,
                  GdkPixmap * parent,
                  GdkGC * gc,
                  gint x, gint y, 
                  SkinPixmapId si)
{
    MonoStereo *ms;

    ms = g_new0(MonoStereo, 1);
    widget_init(&ms->ms_widget, parent, gc, x, y, 56, 12, 1);
    ms->ms_widget.draw = monostereo_draw;
    ms->ms_skin_index = si;

    widget_list_add(wlist, WIDGET(ms));
    return ms;
}
