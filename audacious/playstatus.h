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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef PLAYSTATUS_H
#define PLAYSTATUS_H

#include "widget.h"

typedef enum {
    STATUS_STOP, STATUS_PAUSE, STATUS_PLAY
} PStatus;

#define PLAY_STATUS(x)  ((PlayStatus *)(x))
struct _PlayStatus {
    Widget ps_widget;
    PStatus ps_status;
    gboolean ps_status_buffering;
};

typedef struct _PlayStatus PlayStatus;

void playstatus_set_status(PlayStatus * ps, PStatus status);
void playstatus_set_status_buffering(PlayStatus * ps, gboolean status);
PlayStatus *create_playstatus(GList ** wlist, GdkPixmap * parent,
                              GdkGC * gc, gint x, gint y);

#endif
