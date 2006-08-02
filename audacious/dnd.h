/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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

#define bmp_drag_dest_set(widget) \
    gtk_drag_dest_set(widget, \
		      GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, \
		      bmp_drop_types, 5, \
                      GDK_ACTION_COPY | GDK_ACTION_MOVE)
