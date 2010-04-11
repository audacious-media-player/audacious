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

#ifndef APE_H

#define APE_H

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

enum {
    APE_ALBUM = 0,
    APE_TITLE,
    APE_COPYRIGHT,
    APE_ARTIST,
    APE_TRACKNR,
    APE_YEAR,
    APE_GENRE,
    APE_COMMENT,
    APE_ITEMS_NO
};



typedef struct  apeheader
{
    gchar *preamble;  //64 bits
    guint32 version;
    guint32 tagSize;
    guint32 itemCount;
    guint32 flags;
    guint64 reserved;
}APEv2Header;

typedef struct tagitem
{
    guint32 size;
    guint32 flags;
    gchar* key; //null terminated
    gchar* value;
}APETagItem;

mowgli_dictionary_t *tagItems;
mowgli_list_t *tagKeys;
guint32 headerPosition ;

/* TAG plugin API */
gboolean ape_can_handle_file(VFSFile *f);
Tuple *ape_populate_tuple_from_file(Tuple *tuple,VFSFile *f);
gboolean ape_write_tuple_to_file(Tuple* tuple, VFSFile *f);

static const tag_module_t ape = {
    .name = "APE",
    .can_handle_file = ape_can_handle_file,
    .populate_tuple_from_file = ape_populate_tuple_from_file,
    .write_tuple_to_file = ape_write_tuple_to_file,
};
#endif
