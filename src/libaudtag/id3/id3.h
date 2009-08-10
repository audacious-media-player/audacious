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
    guint32 flags;
}ID3v2FrameHeader;

typedef struct genericframe
{
    ID3v2FrameHeader header;
    gchar*           frame_body;
}GenericFrame;

typedef struct textframe
{
    ID3v2FrameHeader header;
    gchar encoding;
    gchar* text;
}TextInformationFrame;


gboolean id3_can_handle_file(VFSFile *f);

Tuple *id3_populate_tuple_from_file(VFSFile *f);

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t id3;

#endif