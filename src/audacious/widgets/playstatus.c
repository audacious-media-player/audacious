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

#include "widgetcore.h"

#include <glib.h>
#include <gdk/gdk.h>

#include "skin.h"
#include "widget.h"

void
playstatus_draw(Widget * w)
{
    PlayStatus *ps = PLAY_STATUS(w);
    GdkPixmap *obj;

    if (!w)
        return;

    obj = ps->ps_widget.parent;
    if (ps->ps_status == STATUS_STOP && ps->ps_status_buffering == TRUE)
        ps->ps_status_buffering = FALSE;
    if (ps->ps_status == STATUS_PLAY && ps->ps_status_buffering == TRUE)
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 39, 0, ps->ps_widget.x,
                         ps->ps_widget.y, 3, 9);
    else if (ps->ps_status == STATUS_PLAY)
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 36, 0, ps->ps_widget.x,
                         ps->ps_widget.y, 3, 9);
    else
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 27, 0, ps->ps_widget.x,
                         ps->ps_widget.y, 2, 9);
    switch (ps->ps_status) {
    case STATUS_STOP:
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 18, 0,
                         ps->ps_widget.x + 2, ps->ps_widget.y, 9, 9);
        break;
    case STATUS_PAUSE:
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 9, 0,
                         ps->ps_widget.x + 2, ps->ps_widget.y, 9, 9);
        break;
    case STATUS_PLAY:
        skin_draw_pixmap(bmp_active_skin, obj, ps->ps_widget.gc,
                         SKIN_PLAYPAUSE, 1, 0,
                         ps->ps_widget.x + 3, ps->ps_widget.y, 8, 9);
        break;
    }
}

void
playstatus_set_status(PlayStatus * ps, PStatus status)
{
    if (!ps)
        return;

    ps->ps_status = status;
    widget_draw(WIDGET(ps));
}

void
playstatus_set_status_buffering(PlayStatus * ps, gboolean status)
{
    if (!ps)
        return;

    ps->ps_status_buffering = status;
    widget_draw(WIDGET(ps));
}

PlayStatus *
create_playstatus(GList ** wlist, GdkPixmap * parent,
                  GdkGC * gc, gint x, gint y)
{
    PlayStatus *ps;

    ps = g_new0(PlayStatus, 1);
    widget_init(&ps->ps_widget, parent, gc, x, y, 11, 9, TRUE);
    ps->ps_widget.draw = playstatus_draw;

    widget_list_add(wlist, WIDGET(ps));
    return ps;
}
