/*
 *  cddb.h  Copyright 1999 Håvard Kvålen <havardk@sol.no>
 *
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef CDDB_H
#define CDDB_H

#include <glib.h>

typedef struct {
    gchar *category;
    guint32 discid;
} cddb_disc_header_t;

#define CDDB_MAX_PROTOCOL_LEVEL 3
#define CDDB_HOSTNAME_LEN 100
#define CDDB_LOG_MAX 100

#endif
