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

#ifndef ID3_H

#define ID3_H

#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"


typedef struct id3v2
{
    gchar *id3;
    guint16 version;
    gchar  flags;
    guint32 size;
} ID3v2Header;

typedef struct extHeader
{
    guint32 header_size;
    guint16 flags;
    guint32 padding_size;
}ExtendedHeader;

typedef struct frameheader
{
    gchar* frame_id;
    guint32 size;
    guint16 flags;
}ID3v2FrameHeader;

typedef struct genericframe
{
    ID3v2FrameHeader *header;
    gchar*           frame_body;
}GenericFrame;

guint32 read_syncsafe_int32(VFSFile *fd);

ID3v2Header *readHeader(VFSFile *fd);

ExtendedHeader *readExtendedHeader(VFSFile *fd);

ID3v2FrameHeader *readID3v2FrameHeader(VFSFile *fd);

gchar* readFrameBody(VFSFile *fd,int size);

GenericFrame *readGenericFrame(VFSFile *fd,GenericFrame *gf);

void readAllFrames(VFSFile *fd,int framesSize);

void write_int32(VFSFile *fd, guint32 val);

void  write_syncsafe_int32(VFSFile *fd, guint32 val);

void write_ASCII(VFSFile *fd, int size, gchar* value);

void write_utf8(VFSFile *fd, int size,gchar* value);

guint32 writeAllFramesToFile(VFSFile *fd);

void writeID3HeaderToFile(VFSFile *fd,ID3v2Header *header);

void writePaddingToFile(VFSFile *fd, int ksize);

void writeID3FrameHeaderToFile(VFSFile *fd, ID3v2FrameHeader *header);

void writeGenericFrame(VFSFile *fd,GenericFrame *frame);

gboolean isExtendedHeader(ID3v2Header *header);

int getFrameID(ID3v2FrameHeader *header);

void skipFrame(VFSFile *fd, guint32 size);

gboolean isValidFrame(GenericFrame *frame);

void add_newISO8859_1FrameFromString(const gchar *value,int id3_field);

void add_newFrameFromTupleStr(Tuple *tuple, int field,int id3_field);

void add_newFrameFromTupleInt(Tuple *tuple,int field,int id3_field);

void add_frameFromTupleStr(Tuple *tuple, int field,int id3_field);

void add_frameFromTupleInt(Tuple *tuple, int field,int id3_field);

mowgli_dictionary_t *frames ;
mowgli_list_t *frameIDs;

/* TAG plugin API */
gboolean id3v2_can_handle_file(VFSFile *f);
Tuple *id3v2_populate_tuple_from_file(Tuple *tuple,VFSFile *f);
gboolean id3v2_write_tuple_to_file(Tuple* tuple, VFSFile *f);

static const tag_module_t id3v2 = {
    .name = "ID3v2",
    .can_handle_file = id3v2_can_handle_file,
    .populate_tuple_from_file = id3v2_populate_tuple_from_file,
    .write_tuple_to_file = id3v2_write_tuple_to_file,
};
#endif
