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

#ifndef AAC_H
#define	AAC_H
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

#define FTYP    "ftyp"
#define MOOV    "moov"
#define ILST    "ilst"
#define UDTA    "udta"
#define META    "meta"
#define ILST    "ilst"
#define FREE    "free"

enum {
    MP4_ALBUM = 0,
    MP4_TITLE,
    MP4_COPYRIGHT,
    MP4_ARTIST,
    MP4_ARTIST2,
    MP4_TRACKNR,
    MP4_YEAR,
    MP4_GENRE,
    MP4_COMMENT,
    MP4_ITEMS_NO
};

typedef struct mp4atom
{
    guint32 size;
    gchar* name; //4 bytes
    gchar* body;
    int type;
}Atom;


typedef struct strdataatom
{
    guint32 atomsize;
    gchar* name;
    guint32 datasize;
    gchar* dataname;
    guint32 vflag;
    guint32 nullData;
    gchar*  data;
    int type;
}StrDataAtom;

Atom *ilstAtom;
guint64 ilstFileOffset;
guint32 newilstSize ;
mowgli_list_t *dataAtoms;
mowgli_dictionary_t *ilstAtoms;

/* TAG plugin API */
gboolean aac_can_handle_file(VFSFile *f);
gboolean aac_read_tag (Tuple * tuple, VFSFile * handle);
gboolean aac_write_tag (Tuple * tuple, VFSFile * handle);

static const tag_module_t aac = {
    .name = "AAC",
    .can_handle_file = aac_can_handle_file,
    .read_tag = aac_read_tag,
    .write_tag = aac_write_tag,
};
#endif
