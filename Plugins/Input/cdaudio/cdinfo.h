/*
 *  cdinfo.h   Copyright 1999 Espen Skoglund <esk@ira.uka.de>
 *             Copyright 1999 Håvard Kvålen <havardk@sol.no>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef CDINFO_H
#define CDINFO_H

#include <glib.h>

/*
 * For holding info on a single CD track.
 */
typedef struct {
    gchar *artist;
    gchar *title;
    gint num;
} trackinfo_t;

/*
 * For holding info on a complete CD.
 */
typedef struct {
    gboolean is_valid;
    gchar *albname;
    gchar *artname;
    trackinfo_t tracks[100];
} cdinfo_t;

void cdda_cdinfo_flush(cdinfo_t * cdinfo);
cdinfo_t *cdda_cdinfo_new(void);
void cdda_cdinfo_delete(cdinfo_t * info);
void cdda_cdinfo_track_set(cdinfo_t * cdinfo, gint, gchar *, gchar *);
void cdda_cdinfo_cd_set(cdinfo_t * cdinfo, gchar *, gchar *);
gint cdda_cdinfo_get(cdinfo_t * cdinfo, gint num, gchar **, gchar **,
                     gchar **);
gboolean cdda_cdinfo_read_file(guint32 cddb_discid, cdinfo_t * cdinfo);
void cdda_cdinfo_write_file(guint32 cddb_discid, cdinfo_t * cdinfo);


#endif
