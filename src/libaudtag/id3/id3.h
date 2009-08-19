#ifndef ID3_H

#define ID3_H

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

typedef struct textframe
{
    ID3v2FrameHeader header;
    gchar encoding;
    gchar* text;
}TextInformationFrame;

gchar *read_iso8859_1(VFSFile *fd, int size);

gchar* read_unicode(VFSFile *fd, int size);

guint32 read_syncsafe_int32(VFSFile *fd);

ID3v2Header *readHeader(VFSFile *fd);

ExtendedHeader *readExtendedHeader(VFSFile *fd);

ID3v2FrameHeader *readID3v2FrameHeader(VFSFile *fd);

TextInformationFrame *readTextFrame(VFSFile *fd, TextInformationFrame *frame);

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

gboolean isUnsynchronisation(ID3v2Header *header);

gboolean isExperimental(ID3v2Header *header);

int getFrameID(ID3v2FrameHeader *header);

void skipFrame(VFSFile *fd, guint32 size);

Tuple *assocStrInfo(Tuple *tuple, VFSFile *fd, int field,ID3v2FrameHeader header);

Tuple *assocIntInfo(Tuple *tuple, VFSFile *fd, int field,ID3v2FrameHeader header);

gboolean isValidFrame(GenericFrame *frame);

void add_newISO8859_1FrameFromString(const gchar *value,int id3_field);

void add_newFrameFromTupleStr(Tuple *tuple, int field,int id3_field);

void add_newFrameFromTupleInt(Tuple *tuple,int field,int id3_field);

void add_frameFromTupleStr(Tuple *tuple, int field,int id3_field);

void add_frameFromTupleInt(Tuple *tuple, int field,int id3_field);

gboolean id3_can_handle_file(VFSFile *f);

Tuple *id3_populate_tuple_from_file(Tuple *tuple,VFSFile *f);

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t id3;
mowgli_dictionary_t *frames ;
mowgli_list_t *frameIDs;
#endif
