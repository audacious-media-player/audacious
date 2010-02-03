#include <glib.h>
#include <glib/gstdio.h>

#include "id3.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"
#include "frame.h"

#define TAG_SIZE 1

tag_module_t id3 = {id3_can_handle_file, id3_populate_tuple_from_file, id3_write_tuple_to_file};
/* reading stuff */

gchar *read_iso8859_1(VFSFile *fd, int size)
{
    gchar *value= g_new0(gchar,size);
    vfs_fread(value,size,1,fd);
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,size,"UTF-8","ISO-8859-1",&bytes_read,&bytes_write,&error);
    g_free(value);
    return retVal;
}

/*
 * Read UTF-16 from the tag and return an uft-8 string for the tuple
 */
gchar* read_unicode(VFSFile *fd, int size)
{
    gchar *value= g_new0(gchar,size);
    vfs_fread(value,size,1,fd);
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,size,"UTF-8","UTF-16",&bytes_read,&bytes_write,&error);
    g_free(value);
    return retVal;
}

guint32 read_syncsafe_int32(VFSFile *fd)
{
    guint32 val = read_BEuint32(fd);
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
    };
    return intVal;
}

ID3v2Header *readHeader(VFSFile *fd)
{
    ID3v2Header *header = g_new0(ID3v2Header,1);
    header->id3 = read_char_data(fd,3);
    header->version = read_LEuint16(fd);
    header->flags = *read_char_data(fd,1);
    header->size = read_syncsafe_int32(fd);
    return header;
}

ExtendedHeader *readExtendedHeader(VFSFile *fd)
{
    ExtendedHeader *header = g_new0(ExtendedHeader,1);
    header->header_size = read_BEuint32(fd);
    header->flags = read_LEuint16(fd);
    header->padding_size = read_BEuint32(fd);
    return header;
}

ID3v2FrameHeader *readID3v2FrameHeader(VFSFile *fd)
{
    ID3v2FrameHeader *frameheader = g_new0(ID3v2FrameHeader,1);
    frameheader->frame_id = read_char_data(fd,4);
    frameheader->size = read_BEuint32(fd);
    frameheader->flags = read_LEuint16(fd);
    return frameheader;
}

TextInformationFrame *readTextFrame(VFSFile *fd, TextInformationFrame *frame)
{
    frame->encoding = read_char_data(fd,1)[0];

    if(frame->encoding == 0)
        frame->text = read_iso8859_1(fd,frame->header.size - 1);

    if(frame->encoding == 1)
        frame->text = read_unicode(fd,frame->header.size - 1);
    return frame;
}


gchar* readFrameBody(VFSFile *fd,int size)
{
    if(size == 0)
        return NULL;
    gchar *b = g_new0(gchar,size);
    vfs_fread(b,size,1,fd);
    return b;
}

GenericFrame *readGenericFrame(VFSFile *fd,GenericFrame *gf)
{
    gf->header = readID3v2FrameHeader(fd);
    gf->frame_body = readFrameBody(fd,gf->header->size);

    return gf;
}


void readAllFrames(VFSFile *fd,int framesSize)
{
    int pos = 0;
    int i = 0;
    while(pos < framesSize)
    {
        GenericFrame *gframe = g_new0(GenericFrame,1);
        gframe = readGenericFrame(fd,gframe);
        if(isValidFrame(gframe))
        {
            mowgli_dictionary_add(frames, gframe->header->frame_id, gframe);
            mowgli_node_add(gframe->header->frame_id, mowgli_node_create(), frameIDs);
            pos += gframe->header->size;
            i++;
        }else
            break;
    }

}


/* writing stuff */
void write_int32(VFSFile *fd, guint32 val)
{
    guint32 be_val = GUINT32_TO_BE(val);
    vfs_fwrite(&be_val,4,1,fd);
}

void  write_syncsafe_int32(VFSFile *fd, guint32 val)
{
    //TODO write the corrent function - this is just for testing
    int i = 0;
    guint32 tmp =0x0;
    guint32 mask =0x7f;
    guint32 syncVal = 0;
    tmp = val & mask;
    syncVal = tmp;
    for(i = 0;i<3;i++)
    {
        tmp = 0;
        mask <<= 7;
        tmp = val & mask;
        tmp <<=1;
        syncVal |=tmp;
    }
    guint32 be_val = GUINT32_TO_BE(syncVal);
    vfs_fwrite(&be_val,4,1,fd);
}


void write_ASCII(VFSFile *fd, int size, gchar* value)
{
    vfs_fwrite(value,size,1,fd);
}


void write_utf8(VFSFile *fd, int size,gchar* value)
{
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* isoVal = g_convert(value,size,"ISO-8859-1","UTF-8",&bytes_read,&bytes_write,&error);
    vfs_fwrite(isoVal,size,1,fd);
}

guint32 writeAllFramesToFile(VFSFile *fd)
{
   guint32 size = 0;
   mowgli_node_t *n, *tn;
   MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
   {
       GenericFrame *frame = (GenericFrame*)mowgli_dictionary_retrieve(frames,(gchar*)(n->data));
       if(frame)
       {
           writeGenericFrame(fd,frame);
           size += frame->header->size+10;
       }
   }
   return size;
}

void writeID3HeaderToFile(VFSFile *fd,ID3v2Header *header)
{
    vfs_fwrite(header->id3,3,1,fd);
    vfs_fwrite(&header->version,2,1,fd);
    vfs_fwrite(&header->flags,1,1,fd);
    write_syncsafe_int32(fd,header->size);
}

void writePaddingToFile(VFSFile *fd, int ksize)
{
    gchar padding = 0;
    int i = 0;
    for(i=0; i<ksize;i++)
        vfs_fwrite(&padding,1,1,fd);
}


void writeID3FrameHeaderToFile(VFSFile *fd, ID3v2FrameHeader *header)
{
    vfs_fwrite(header->frame_id,4,1,fd);
    write_int32(fd,header->size);
    vfs_fwrite(&header->flags,2,1,fd);
}

void writeGenericFrame(VFSFile *fd,GenericFrame *frame)
{
    writeID3FrameHeaderToFile(fd,frame->header);
    vfs_fwrite(frame->frame_body,frame->header->size,1,fd);
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


int getFrameID(ID3v2FrameHeader *header)
{
    int i=0;
    for(i = 0; i<ID3_TAGS_NO;i++)
    {
        if(!strcmp(header->frame_id,id3_frames[i]))
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
/*  AUDDBG("field = %s\n",frame->text);*/
  tuple_associate_string(tuple, field, NULL, frame->text);
  return tuple;
}

Tuple *assocIntInfo(Tuple *tuple, VFSFile *fd, int field,ID3v2FrameHeader header)
{
  TextInformationFrame *frame = g_new0(TextInformationFrame,1);
  frame->header = header;
  frame = readTextFrame(fd,frame);
/*  AUDDBG("field = %s\n",frame->text);*/
  tuple_associate_int(tuple, field, NULL, atoi(frame->text));
  return tuple;
}

gboolean isValidFrame(GenericFrame *frame)
{
    if(strlen(frame->header->frame_id) != 0)
        return TRUE;
    else
        return FALSE;
}



void add_newISO8859_1FrameFromString(const gchar *value,int id3_field)
{
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,strlen(value),"ISO-8859-1","UTF-8",&bytes_read,&bytes_write,&error);
    ID3v2FrameHeader *header = g_new0(ID3v2FrameHeader,1);
    header->frame_id = id3_frames[id3_field];
    header->flags = 0;
    header->size = strlen(retVal)+1;
    gchar* buf = g_new0(gchar,header->size+1);
    memcpy(buf+1,retVal,header->size);
    GenericFrame *frame = g_new0(GenericFrame,1);
    frame->header = header;
    frame->frame_body  = buf;
    mowgli_dictionary_add(frames,header->frame_id,frame);
    mowgli_node_add(frame->header->frame_id, mowgli_node_create(), frameIDs);

}


void add_newFrameFromTupleStr(Tuple *tuple, int field,int id3_field)
{
    const gchar *value = tuple_get_string(tuple,field,NULL);
    add_newISO8859_1FrameFromString(value,id3_field);
}


void add_newFrameFromTupleInt(Tuple *tuple,int field,int id3_field)
{
    int intvalue = tuple_get_int(tuple,field,NULL);
    gchar *value = g_strdup_printf("%d",intvalue);
    add_newISO8859_1FrameFromString(value,id3_field);

}



void add_frameFromTupleStr(Tuple *tuple, int field,int id3_field)
{
    const gchar *value = tuple_get_string(tuple,field,NULL);
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,strlen(value),"ISO-8859-1","UTF-8",&bytes_read,&bytes_write,&error);

    GenericFrame *frame = mowgli_dictionary_retrieve(frames,id3_frames[id3_field]);
    if(frame != NULL)
    {
        frame->header->size = strlen(retVal)+1;
        gchar* buf = g_new0(gchar,frame->header->size+1);
        memcpy(buf+1,retVal,frame->header->size);
        frame->frame_body = buf;
    }
    else
        add_newFrameFromTupleStr(tuple,field,id3_field);

}

void add_frameFromTupleInt(Tuple *tuple, int field,int id3_field)
{
    int intvalue = tuple_get_int(tuple,field,NULL);
    gchar *value = g_strdup_printf("%d",intvalue);
    GError *error = NULL;
    gsize bytes_read = 0 , bytes_write = 0;
    gchar* retVal = g_convert(value,strlen(value),"ISO-8859-1","UTF-8",&bytes_read,&bytes_write,&error);

    GenericFrame *frame = mowgli_dictionary_retrieve(frames,id3_frames[id3_field]);
    if(frame != NULL)
    {
        frame->header->size = strlen(retVal)+1;
        gchar* buf = g_new0(gchar,frame->header->size+1);
        memcpy(buf+1,retVal,frame->header->size);
        frame->frame_body = buf;
    }
    else
        add_newFrameFromTupleStr(tuple,field,id3_field);

}

gboolean id3_can_handle_file(VFSFile *f)
{
    ID3v2Header *header = readHeader(f);
    if(!strcmp(header->id3,"ID3"))
        return TRUE;
    return FALSE;
}



Tuple *id3_populate_tuple_from_file(Tuple *tuple, VFSFile *f)
{
    //reset file position
    vfs_fseek(f,0,SEEK_SET);
//    Tuple *tuple = tuple_new_from_filename(f->uri);
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
             //   tuple = assocIntInfo(tuple,f,FIELD_LENGTH,*header);
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
            case ID3_COMMENT:
            {
                tuple = assocStrInfo(tuple,f,FIELD_COMMENT,*header);
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
    VFSFile *tmp;
    vfs_fseek(f,0,SEEK_SET);

    ExtendedHeader *extHeader;
    if(frameIDs != NULL)
    {
        mowgli_node_t *n, *tn;
        MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
        {
            mowgli_node_delete(n,frameIDs);
        }
    }
    frameIDs = mowgli_list_create();
    ID3v2Header *header = readHeader(f);
    int framesSize = header->size;

    if(isExtendedHeader(header))
    {
        extHeader = readExtendedHeader(f);
        framesSize -= 10;
        framesSize -= extHeader->padding_size;
    }

    //read all frames into generic frames;
    frames = mowgli_dictionary_create(strcasecmp);
    readAllFrames(f,header->size);

    //make the new frames from tuple and replace in the dictinonary the old frames with the new ones
    if(tuple_get_string(tuple, FIELD_ARTIST, NULL))
        add_frameFromTupleStr(tuple, FIELD_ARTIST,ID3_ARTIST);


   if(tuple_get_string(tuple, FIELD_TITLE, NULL))
        add_frameFromTupleStr(tuple, FIELD_TITLE,ID3_TITLE);


   if(tuple_get_string(tuple, FIELD_ALBUM, NULL))
        add_frameFromTupleStr(tuple, FIELD_ALBUM,ID3_ALBUM);

   if(tuple_get_string(tuple,FIELD_COMMENT,NULL))
       add_frameFromTupleStr(tuple,FIELD_COMMENT,ID3_COMMENT);

    if(tuple_get_string(tuple,FIELD_GENRE,NULL))
        add_frameFromTupleStr(tuple,FIELD_GENRE,ID3_GENRE);

    if(tuple_get_int(tuple,FIELD_YEAR,NULL) !=0 )
        add_frameFromTupleInt(tuple,FIELD_YEAR,ID3_YEAR);

    if(tuple_get_int(tuple,FIELD_TRACK_NUMBER,NULL) != 0)
        add_frameFromTupleInt(tuple,FIELD_TRACK_NUMBER,ID3_TRACKNR);

    const gchar *tmpdir = g_get_tmp_dir();
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "tmp.mpc");
    tmp = vfs_fopen(tmp_path, "w+");

    int oldSize = header->size;
    header->size = TAG_SIZE*1024;

    writeID3HeaderToFile(tmp,header);

    int size = writeAllFramesToFile(tmp);
    writePaddingToFile(tmp,TAG_SIZE*1024-size-10);

    copyAudioToFile(f,tmp,oldSize);


    gchar *uri = g_strdup(f -> uri);
    vfs_fclose(tmp);
    gchar* f1 = g_filename_from_uri(tmp_path,NULL,NULL);
    gchar* f2 = g_filename_from_uri(uri,NULL,NULL);
    if (g_rename(f1,f2 ) == 0) {
        AUDDBG("the tag was updated successfully\n");
    } else {
        AUDDBG("an error has occured\n");
    }
    return TRUE;
}
