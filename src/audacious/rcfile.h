/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */
#ifndef AUDACIOUS_RCFILE_H
#define AUDACIOUS_RCFILE_H

#include <glib.h>

G_BEGIN_DECLS

/** RcLine objects contain key->value mappings. */
typedef struct {
    gchar *key;     /**< Key for the key->value mapping. */
    gchar *value;   /**< Value for the key->value mapping. */
} RcLine;

/** RcSection objects contain collections of key->value mappings. */
typedef struct {
    gchar *name;    /**< Name for the #RcSection. */
    GList *lines;   /**< List of key->value mappings for the #RcSection.*/
} RcSection;

/**
 * An RcFile object contains a collection of key->value mappings
 * organized by section.
 */
typedef struct {
    GList *sections;    /**< A linked list of sections. */
} RcFile;


RcFile *aud_rcfile_new(void);
void aud_rcfile_free(RcFile * file);

RcFile *aud_rcfile_open(const gchar * filename);
gboolean aud_rcfile_write(RcFile * file, const gchar * filename);

gboolean aud_rcfile_read_string(RcFile * file, const gchar * section,
                                const gchar * key, gchar ** value);
gboolean aud_rcfile_read_int(RcFile * file, const gchar * section,
                             const gchar * key, gint * value);
gboolean aud_rcfile_read_bool(RcFile * file, const gchar * section,
                              const gchar * key, gboolean * value);
gboolean aud_rcfile_read_float(RcFile * file, const gchar * section,
                               const gchar * key, gfloat * value);
gboolean aud_rcfile_read_double(RcFile * file, const gchar * section,
                                const gchar * key, gdouble * value);

void aud_rcfile_write_string(RcFile * file, const gchar * section,
                             const gchar * key, const gchar * value);
void aud_rcfile_write_int(RcFile * file, const gchar * section,
                          const gchar * key, gint value);
void aud_rcfile_write_boolean(RcFile * file, const gchar * section,
                              const gchar * key, gboolean value);
void aud_rcfile_write_float(RcFile * file, const gchar * section,
                            const gchar * key, gfloat value);
void aud_rcfile_write_double(RcFile * file, const gchar * section,
                             const gchar * key, gdouble value);

void aud_rcfile_remove_key(RcFile * file, const gchar * section,
                           const gchar * key);

G_END_DECLS

#endif /* AUDACIOUS_RCFILE_H */
