#include <glib.h>
#include <glib/gstdio.h>

#include "id3.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

tag_module_t id3 = {id3_can_handle_file, id3_populate_tuple_from_file, id3_write_tuple_to_file};

guint32 read_int32(VFSFile *fd)
{
    guint32 a;
    vfs_fread(&a,4,1,fd);
    return a;
}

guint32 read_int16(VFSFile *fd)
{
    guint16 a;
    vfs_fread(&a,2,1,fd);
    return a;
}


gchar *read_ASCII(VFSFile *fd, int size)
{
    gchar *value= g_new0(gchar,size);
    vfs_fread(value,size,1,fd);
    return value;
}

guint32 read_syncsafe_int32(VFSFile *fd)
{
    guint32 val = read_int32(fd);
    guint32 intVal = 0;
    intVal = ((intVal)  |  (val & 0x7f));
    int i;
    for(i = 0; i<3; i++)
        intVal = ((intVal << 7) | (val >> 8 & 0x7f));
    return intVal;
}


gboolean isExtendedHeader(ID3v2Header header)
{
    if((header->flags & 0x40) == 0x40)
        return TRUE;
    else
        return FALSE;
}

gboolean isUnsynchronisation(ID3v2Header header)
{
        if((header->flags & 0x80) == 0x80)
            return TRUE;
        else
            return FALSE;
}

gboolean isExperimental(ID3v2Header header)
{
        if((header->flags & 0x20) == 0x20)
            return TRUE;
        else
            return FALSE;
}



ID3v2Header *readHeader(VFSFile *fd)
{
    ID3v2Header *header = g_new0(ID3v2Header,1);
    header->id3 = read_ASCII(fd,3);
    header->flags = *read_ASCII(fd,1);
    header->size = read_syncsafe_int32(fd);
    return header;
}

ExtendedHeader *readExtendedHeader(VFSFile *fd)
{
    ExtendedHeader *header = g_new0(ExtendedHeader,1);
    header->header_size = read_int32(fd);
    header->flags = read_int16(fd);
    header->padding_size = read_int32(fd);
    return header;
}

ID3v2FrameHeader *readID3v2Frame(VFSFile *fd)
{
    ID3v2FrameHeader *frameheader = g_new0(ID3v2FrameHeader,1);
    frameheader->frame_id = read_ASCII(fd,4);
    frameheader->size = read_int32(fd);
    frameheader->flags = read_int16(fd);
    return frameheader;
}

gboolean id3_can_handle_file(VFSFile *f)
{
    ID3v2Header *header = readHeader(f);
    if(g_ascii_strncasecmp(header->id3,"ID3",3))
        return TRUE;
    return FALSE;
}



Tuple *id3_populate_tuple_from_file(VFSFile *f)
{
    return NULL;
}

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    return FALSE;
}