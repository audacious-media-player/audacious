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
#ifndef RCFILE_H
#define RCFILE_H

#include <glib.h>

typedef struct {
    gchar *key;
    gchar *value;
} RcLine;

typedef struct {
    gchar *name;
    GList *lines;
} RcSection;

typedef struct {
    GList *sections;
} RcFile;

G_BEGIN_DECLS

RcFile *bmp_rcfile_new(void);
void bmp_rcfile_free(RcFile * file);

RcFile *bmp_rcfile_open(const gchar * filename);
gboolean bmp_rcfile_write(RcFile * file, const gchar * filename);

gboolean bmp_rcfile_read_string(RcFile * file, const gchar * section,
                                const gchar * key, gchar ** value);
gboolean bmp_rcfile_read_int(RcFile * file, const gchar * section,
                             const gchar * key, gint * value);
gboolean bmp_rcfile_read_bool(RcFile * file, const gchar * section,
                              const gchar * key, gboolean * value);
gboolean bmp_rcfile_read_float(RcFile * file, const gchar * section,
                               const gchar * key, gfloat * value);
gboolean bmp_rcfile_read_double(RcFile * file, const gchar * section,
                                const gchar * key, gdouble * value);

void bmp_rcfile_write_string(RcFile * file, const gchar * section,
                             const gchar * key, const gchar * value);
void bmp_rcfile_write_int(RcFile * file, const gchar * section,
                          const gchar * key, gint value);
void bmp_rcfile_write_boolean(RcFile * file, const gchar * section,
                              const gchar * key, gboolean value);
void bmp_rcfile_write_float(RcFile * file, const gchar * section,
                            const gchar * key, gfloat value);
void bmp_rcfile_write_double(RcFile * file, const gchar * section,
                             const gchar * key, gdouble value);

void bmp_rcfile_remove_key(RcFile * file, const gchar * section,
                           const gchar * key);

G_END_DECLS

#endif // RCFILE_H
