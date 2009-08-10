#include <glib.h>
#include <glib/gstdio.h>

#include "id3.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"
#include "frame.h"
tag_module_t id3 = {id3_can_handle_file, id3_populate_tuple_from_file, id3_write_tuple_to_file};

guint32 read_int32(VFSFile *fd)
{
    guint32 a;
    vfs_fread(&a,4,1,fd);
    a = GUINT32_FROM_BE(a);
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

gchar *read_iso8859_1(VFSFile *fd, int size)
{
    gchar *value= g_new0(gchar,size);
    vfs_fread(value,size,1,fd);
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,size,"UTF-8","ISO-8859-1",&bytes_read,&bytes_write,&error);
    DEBUG("iso 8859-1 : %s\n",retVal);
    g_free(value);
    return retVal;
}

gboolean write_utf8(VFSFile *fd, int size,gchar* value)
{
/*
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* isoVal = g_convert(value,size,"UTF-8","ISO-8859-1",&bytes_read,&bytes_write,&error);
    DEBUG("iso 8859-1 : %s\n",retVal);
*/
    return FALSE;
}

gchar* read_unicode(VFSFile *fd, int size)
{
    return NULL;
}

guint32 read_syncsafe_int32(VFSFile *fd)
{
    guint32 val = read_int32(fd);
    DEBUG("hex size = %x\n",val);
    guint32 mask = 0x7f;
    guint32 intVal = 0;
    intVal = ((intVal)  |  (val & mask));
    int i;
    for(i = 0; i<3; i++)
    {
        mask = mask << 8;
        guint32 tmp = (val  & mask);
        tmp = tmp >> 1;
        intVal = intVal | tmp;
    }
    DEBUG("header  size = %d\n",intVal);
    return intVal;
}


gboolean isExtendedHeader(ID3v2Header *header)
{
    if( (header->flags & 0x40) == (0x40))
        return TRUE;
    else
        return FALSE;
}

gboolean isUnsynchronisation(ID3v2Header *header)
{
        if((header->flags & 0x80) == 0x80)
            return TRUE;
        else
            return FALSE;
}

gboolean isExperimental(ID3v2Header *header)
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
    header->version = read_int16(fd);
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

ID3v2FrameHeader *readID3v2FrameHeader(VFSFile *fd)
{
    ID3v2FrameHeader *frameheader = g_new0(ID3v2FrameHeader,1);
    frameheader->frame_id = read_ASCII(fd,4);
    frameheader->size = read_int32(fd);
    frameheader->flags = read_int16(fd);
    return frameheader;
}

TextInformationFrame *readTextFrame(VFSFile *fd, TextInformationFrame *frame)
{
    frame->encoding = read_ASCII(fd,1)[0];

    if(frame->encoding == 0)
        frame->text = read_iso8859_1(fd,frame->header.size - 1);

    if(frame->encoding == 1)
        frame->text = read_unicode(fd,frame->header.size - 1);
    return frame;
}

int getFrameID(ID3v2FrameHeader *header)
{
    int i=0;
    for(i = 0; i<ID3_TAGS_NO;i++)
    {
        if(!g_strcmp0(header->frame_id,id3_frames[i]))
            return i;
    }
    return -1;
}


void skipFrame(VFSFile *fd, guint32 size)
{
    vfs_fseek(fd,size,SEEK_CUR);
}

Tuple *assocStrInfo(Tuple *tuple, VFSFile *fd, int field,ID3v2FrameHeader header)
{
  TextInformationFrame *frame = g_new0(TextInformationFrame,1);
  frame->header = header;
  frame = readTextFrame(fd,frame);
/*  DEBUG("field = %s\n",frame->text);*/
  tuple_associate_string(tuple, field, NULL, frame->text);
  return tuple;
}

Tuple *assocIntInfo(Tuple *tuple, VFSFile *fd, int field,ID3v2FrameHeader header)
{
  TextInformationFrame *frame = g_new0(TextInformationFrame,1);
  frame->header = header;
  frame = readTextFrame(fd,frame);
/*  DEBUG("field = %s\n",frame->text);*/
  tuple_associate_int(tuple, field, NULL, atoi(frame->text));
  return tuple;
}

gboolean id3_can_handle_file(VFSFile *f)
{
    DEBUG("id3 can handle file \n");
    ID3v2Header *header = readHeader(f);
    if(!g_strcmp0(header->id3,"ID3"))
        return TRUE;
    return FALSE;
}



Tuple *id3_populate_tuple_from_file(VFSFile *f)
{
    //reset file position
    vfs_fseek(f,0,SEEK_SET);
    DEBUG("id3 populate tuple \n");
    Tuple *tuple = tuple_new_from_filename(f->uri);
    ExtendedHeader *extHeader;
    ID3v2Header *header = readHeader(f);
    int pos = 0;
    if(isExtendedHeader(header))
        extHeader = readExtendedHeader(f);

    while(pos < header->size)
    {
        ID3v2FrameHeader *header = readID3v2FrameHeader(f);
        int id = getFrameID(header);
        pos = pos + header->size + 10;
        switch(id)
        {
            case ID3_ALBUM:
            {
                tuple = assocStrInfo(tuple,f,FIELD_ALBUM,*header);
            }break;
            case ID3_TITLE:
            {
                tuple = assocStrInfo(tuple,f,FIELD_TITLE,*header);
                printTuple(tuple);
            }break;
            case ID3_COMPOSER:
            {
             //   tuple = assocInfo(tuple,f,FIELD_ARTIST,*header);
            }break;
            case ID3_COPYRIGHT:
            {
                tuple = assocStrInfo(tuple,f,FIELD_COPYRIGHT,*header);
            }break;
            case ID3_DATE:
            {
                tuple = assocStrInfo(tuple,f,FIELD_DATE,*header);
            }break;
            case ID3_TIME:
            {
                //tuple = assocIntInfo(tuple,f,FIELD_T,*header);
            }break;
            case ID3_LENGTH:
            {
                tuple = assocIntInfo(tuple,f,FIELD_LENGTH,*header);
            }break;
            case ID3_ARTIST:
            {
                tuple = assocStrInfo(tuple,f,FIELD_ARTIST,*header);
            }break;
            case ID3_TRACKNR:
            {
                tuple = assocIntInfo(tuple,f,FIELD_TRACK_NUMBER,*header);
            }break;
            case ID3_YEAR:
            {
                tuple = assocIntInfo(tuple,f,FIELD_YEAR,*header);
            }break;
            case ID3_GENRE:
            {
                tuple = assocStrInfo(tuple,f,FIELD_GENRE,*header);
            }break;
            default:
            {
                //we a a frame that I dont need so skip it
                skipFrame(f,header->size);
            }
        }
    }
    return tuple;
}

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    return FALSE;
}