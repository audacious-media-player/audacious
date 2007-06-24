/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifndef DND_H
#define DND_H

#include <gtk/gtk.h>

/* Designate dropped data types that we know and care about */
enum {
    BMP_DROP_STRING,
    BMP_DROP_PLAINTEXT,
    BMP_DROP_URLENCODED,
    BMP_DROP_SKIN,
    BMP_DROP_FONT
};

/* Drag data format listing for gtk_drag_dest_set() */
static const GtkTargetEntry bmp_drop_types[] = {
    {"text/plain", 0, BMP_DROP_PLAINTEXT},
    {"text/uri-list", 0, BMP_DROP_URLENCODED},
    {"STRING", 0, BMP_DROP_STRING},
    {"interface/x-winamp-skin", 0, BMP_DROP_SKIN},
    {"application/x-font-ttf", 0, BMP_DROP_FONT},
};

void bmp_drag_dest_set(GtkWidget*);

#endif
