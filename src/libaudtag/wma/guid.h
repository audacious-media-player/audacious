/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef _GUID_H
#define _GUID_H

#include <glib.h>
#include <libaudcore/vfs.h>

/* this stuff may be moved to ../util.h if needed by other formats */
typedef struct _guid {
    guint32 le32;
    guint16 le16_1;
    guint16 le16_2;
    guint64 be64;
}GUID_t;


GUID_t *guid_read_from_file(VFSFile *);
gboolean guid_write_to_file(VFSFile *, int);

GUID_t *guid_convert_from_string(const gchar *);
gchar *guid_convert_to_string(const GUID_t*);
gboolean guid_equal(GUID_t *, GUID_t *);
int get_guid_type(GUID_t *);
const gchar *wma_guid_map(int);

#endif /* _GUID_H */
