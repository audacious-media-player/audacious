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

#include <glib.h>
#include <glib/gstdio.h>

#include "id3v2.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"
#include "frame.h"

#define TAG_SIZE 1

guint32 read_syncsafe_int32(VFSFile * fd)
{
    guint32 val = read_BEuint32(fd);
    guint32 mask = 0x7f;
    guint32 intVal = 0;
    intVal = ((intVal) | (val & mask));
    int i;
    for (i = 0; i < 3; i++)
    {
        mask = mask << 8;
        guint32 tmp = (val & mask);
        tmp = tmp >> 1;
        intVal = intVal | tmp;
    };
    return intVal;
}

ID3v2Header *readHeader(VFSFile * fd)
{
    ID3v2Header *header = g_new0(ID3v2Header, 1);
    header->id3 = read_char_data(fd, 3);
    header->version = read_LEuint16(fd);
    header->flags = *read_char_data(fd, 1);
    header->size = read_syncsafe_int32(fd);
    return header;
}

ExtendedHeader *readExtendedHeader(VFSFile * fd)
{
    ExtendedHeader *header = g_new0(ExtendedHeader, 1);
    header->header_size = read_syncsafe_int32(fd);
    header->flags = read_LEuint16(fd);
    header->padding_size = read_BEuint32(fd);
    return header;
}

ID3v2FrameHeader *readID3v2FrameHeader(VFSFile * fd)
{
    ID3v2FrameHeader *frameheader = g_new0(ID3v2FrameHeader, 1);
    frameheader->frame_id = read_char_data(fd, 4);
    frameheader->size = read_syncsafe_int32(fd);
    frameheader->flags = read_LEuint16(fd);
    if ((frameheader->flags & 0x100) == 0x100)
        frameheader->size = read_syncsafe_int32(fd);
    return frameheader;
}

static gint unsyncsafe (gchar * data, gint size)
{
    gchar * get = data, * set = data;

    while (size --)
    {
        gchar c = * set ++ = * get ++;

        if (c == (gchar) 0xff && size)
        {
            size --;
            get ++;
        }
    }

    return set - data;
}

static gchar * read_text_frame (VFSFile * handle, ID3v2FrameHeader * header)
{
    gint size = header->size;
    gchar data[size];

    if (vfs_fread (data, 1, size, handle) != size)
        return NULL;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
        data[size] = '\0';
        return str_to_utf8(data + 1);
    case 1:
        if (data[1] == (gchar) 0xff)
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16LE", NULL,
             NULL, NULL);
        else
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16BE", NULL,
             NULL, NULL);
    case 2:
        return g_convert (data + 1, size - 1, "UTF-8", "UTF-16BE", NULL, NULL,
         NULL);
    case 3:
        return g_strndup (data + 1, size - 1);
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 1, (gint) data[0]);
        return NULL;
    }
}

static gchar * read_raw_text_frame (VFSFile * handle, ID3v2FrameHeader * header)
{
    gint size = header->size;
    gchar data[size];

    if (vfs_fread (data, 1, size, handle) != size)
        return NULL;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
	return g_convert (data + 1, size - 1, "UTF-8", "ISO-8859-1", NULL,
	 NULL, NULL);
    case 1:
        if (data[1] == (gchar) 0xff)
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16LE", NULL,
             NULL, NULL);
        else
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16BE", NULL,
             NULL, NULL);
    case 2:
        return g_convert (data + 1, size - 1, "UTF-8", "UTF-16BE", NULL, NULL,
         NULL);
    case 3:
        return g_strndup (data + 1, size - 1);
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 1, (gint) data[0]);
        return NULL;
    }
}

static gboolean read_comment_frame (VFSFile * handle, ID3v2FrameHeader * header,
 gchar * * lang, gchar * * type, gchar * * value)
{
    gint size = header->size;
    gchar data[size];
    gchar * pair, * sep;
    gsize converted;

    if (vfs_fread (data, 1, size, handle) != size)
        return FALSE;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
        pair = g_convert (data + 4, size - 4, "UTF-8", "ISO-8859-1", NULL,
         & converted, NULL);
        break;
    case 1:
        if (data[4] == (gchar) 0xff)
            pair = g_convert (data + 6, size - 6, "UTF-8", "UTF-16LE", NULL,
             & converted, NULL);
        else
            pair = g_convert (data + 6, size - 6, "UTF-8", "UTF-16BE", NULL,
             & converted, NULL);
        break;
    case 2:
        pair = g_convert (data + 4, size - 4, "UTF-8", "UTF-16BE", NULL,
         & converted, NULL);
        break;
    case 3:
        pair = g_malloc (size - 3);
        memcpy (pair, data + 4, size - 4);
        pair[size - 4] = 0;
        converted = size - 4;
        break;
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 4, (gint) data[0]);
        pair = NULL;
        break;
    }

    if (pair == NULL || (sep = memchr (pair, 0, converted)) == NULL)
        return FALSE;

    * lang = g_strndup (data + 1, 3);
    * type = g_strdup (pair);
    * value = g_strdup (sep + 1);

    g_free (pair);
    return TRUE;
}

GenericFrame *readGenericFrame(VFSFile * fd, GenericFrame * gf)
{
    gf->header = readID3v2FrameHeader(fd);
    gf->frame_body = read_char_data(fd, gf->header->size);

    return gf;
}


void readAllFrames(VFSFile * fd, int framesSize)
{
    int pos = 0;
    int i = 0;
    while (pos < framesSize)
    {
        GenericFrame *gframe = g_new0(GenericFrame, 1);
        gframe = readGenericFrame(fd, gframe);
        if (isValidFrame(gframe))
        {
            mowgli_dictionary_add(frames, gframe->header->frame_id, gframe);
            mowgli_node_add(gframe->header->frame_id, mowgli_node_create(), frameIDs);
            pos += gframe->header->size;
            i++;
        }
        else
            break;
    }

}

void write_int32(VFSFile * fd, guint32 val)
{
    guint32 be_val = GUINT32_TO_BE(val);
    vfs_fwrite(&be_val, 4, 1, fd);
}

void write_syncsafe_int32(VFSFile * fd, guint32 val)
{
    //TODO write the correct function - this is just for testing
    int i = 0;
    guint32 tmp = 0x0;
    guint32 mask = 0x7f;
    guint32 syncVal = 0;
    tmp = val & mask;
    syncVal = tmp;
    for (i = 0; i < 3; i++)
    {
        tmp = 0;
        mask <<= 7;
        tmp = val & mask;
        tmp <<= 1;
        syncVal |= tmp;
    }
    guint32 be_val = GUINT32_TO_BE(syncVal);
    vfs_fwrite(&be_val, 4, 1, fd);
}


void write_ASCII(VFSFile * fd, int size, gchar * value)
{
    vfs_fwrite(value, size, 1, fd);
}


void write_utf8(VFSFile * fd, int size, gchar * value)
{
    GError *error = NULL;
    gsize bytes_read = 0, bytes_write = 0;
    gchar *isoVal = g_convert(value, size, "ISO-8859-1", "UTF-8", &bytes_read, &bytes_write, &error);
    vfs_fwrite(isoVal, size, 1, fd);
}

guint32 writeAllFramesToFile(VFSFile * fd)
{
    guint32 size = 0;
    mowgli_node_t *n, *tn;
    MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
    {
        GenericFrame *frame = (GenericFrame *) mowgli_dictionary_retrieve(frames, (gchar *) (n->data));
        if (frame)
        {
            writeGenericFrame(fd, frame);
            size += frame->header->size + 10;
        }
    }
    return size;
}

void writeID3HeaderToFile(VFSFile * fd, ID3v2Header * header)
{
    vfs_fwrite(header->id3, 3, 1, fd);
    vfs_fwrite(&header->version, 2, 1, fd);
    vfs_fwrite(&header->flags, 1, 1, fd);
    write_syncsafe_int32(fd, header->size);
}

void writePaddingToFile(VFSFile * fd, int ksize)
{
    gchar padding = 0;
    int i = 0;
    for (i = 0; i < ksize; i++)
        vfs_fwrite(&padding, 1, 1, fd);
}


void writeID3FrameHeaderToFile(VFSFile * fd, ID3v2FrameHeader * header)
{
    vfs_fwrite(header->frame_id, 4, 1, fd);
    write_int32(fd, header->size);
    vfs_fwrite(&header->flags, 2, 1, fd);
}

void writeGenericFrame(VFSFile * fd, GenericFrame * frame)
{
    writeID3FrameHeaderToFile(fd, frame->header);
    vfs_fwrite(frame->frame_body, frame->header->size, 1, fd);
}

gboolean isExtendedHeader(ID3v2Header * header)
{
    if ((header->flags & 0x40) == (0x40))
        return TRUE;
    else
        return FALSE;
}

int getFrameID(ID3v2FrameHeader * header)
{
    int i = 0;
    for (i = 0; i < ID3_TAGS_NO; i++)
    {
        if (!strcmp(header->frame_id, id3_frames[i]))
            return i;
    }
    return -1;
}


void skipFrame(VFSFile * fd, guint32 size)
{
    vfs_fseek(fd, size, SEEK_CUR);
}

static void associate_string (Tuple * tuple, VFSFile * handle, gint field,
 const gchar * customfield, ID3v2FrameHeader * header)
{
    gchar * text = read_text_frame (handle, header);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("custom field %s = %s\n", customfield, text);
    else
        AUDDBG ("field %i = %s\n", field, text);

    tuple_associate_string (tuple, field, customfield, text);
    g_free (text);
}

static void associate_int (Tuple * tuple, VFSFile * handle, gint field,
 const gchar * customfield, ID3v2FrameHeader * header)
{
    gchar * text = read_text_frame (handle, header);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("custom field %s = %s\n", customfield, text);
    else
        AUDDBG ("field %i = %s\n", field, text);

    tuple_associate_int (tuple, field, customfield, atoi (text));
    g_free (text);
}

static void decode_private_info(Tuple * tuple, VFSFile * fd, ID3v2FrameHeader * header)
{
    gchar *value = read_char_data(fd, header->size);
    if (!strncmp(value, "WM/", 3))
    {
       AUDDBG("Windows Media tag: %s\n", value);
    } else {
       AUDDBG("Unable to decode private data, skipping: %s\n", value);
    }
}

static void decode_comment (Tuple * tuple, VFSFile * handle, ID3v2FrameHeader *
 header)
{
    gchar * lang, * type, * value;

    if (! read_comment_frame (handle, header, & lang, & type, & value))
        return;

    AUDDBG ("comment: lang = %s, type = %s, value = %s\n", lang, type, value);

    if (! type[0]) /* blank type == actual comment */
        tuple_associate_string (tuple, FIELD_COMMENT, NULL, value);

    g_free (lang);
    g_free (type);
    g_free (value);
}

static void decode_txxx (Tuple * tuple, VFSFile * handle, ID3v2FrameHeader * header)
{
    gchar *text = read_raw_text_frame (handle, header);

    if (text == NULL)
        return;

    gchar *separator = strchr(text, 0);

    if (separator == NULL)
        return;

    gchar * value = separator + 1;
    AUDDBG ("Field '%s' has value '%s'\n", text, value);
    tuple_associate_string (tuple, -1, text, value);

    g_free (text);
}

Tuple *decodeGenre(Tuple * tuple, VFSFile * fd, ID3v2FrameHeader header)
{
    gint numericgenre;
    gchar * text = read_text_frame (fd, & header);

    if (text == NULL)
        return tuple;

    if (text[0] == '(')
        numericgenre = atoi (text + 1);
    else
        numericgenre = atoi (text);

    if (numericgenre > 0)
    {
        tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(numericgenre));
        return tuple;
    }
    tuple_associate_string(tuple, FIELD_GENRE, NULL, text);
    g_free (text);
    return tuple;
}

gboolean isValidFrame(GenericFrame * frame)
{
    if (strlen(frame->header->frame_id) != 0)
        return TRUE;
    else
        return FALSE;
}



void add_newISO8859_1FrameFromString(const gchar * value, int id3_field)
{
    GError *error = NULL;
    gsize bytes_read = 0, bytes_write = 0;
    gchar *retVal = g_convert(value, strlen(value), "ISO-8859-1", "UTF-8", &bytes_read, &bytes_write, &error);
    ID3v2FrameHeader *header = g_new0(ID3v2FrameHeader, 1);
    header->frame_id = id3_frames[id3_field];
    header->flags = 0;
    header->size = strlen(retVal) + 1;
    gchar *buf = g_new0(gchar, header->size + 1);
    memcpy(buf + 1, retVal, header->size);
    GenericFrame *frame = g_new0(GenericFrame, 1);
    frame->header = header;
    frame->frame_body = buf;
    mowgli_dictionary_add(frames, header->frame_id, frame);
    mowgli_node_add(frame->header->frame_id, mowgli_node_create(), frameIDs);

}


void add_newFrameFromTupleStr(Tuple * tuple, int field, int id3_field)
{
    const gchar *value = tuple_get_string(tuple, field, NULL);
    add_newISO8859_1FrameFromString(value, id3_field);
}


void add_newFrameFromTupleInt(Tuple * tuple, int field, int id3_field)
{
    int intvalue = tuple_get_int(tuple, field, NULL);
    gchar *value = g_strdup_printf("%d", intvalue);
    add_newISO8859_1FrameFromString(value, id3_field);

}



void add_frameFromTupleStr(Tuple * tuple, int field, int id3_field)
{
    const gchar *value = tuple_get_string(tuple, field, NULL);
    GError *error = NULL;
    gsize bytes_read = 0, bytes_write = 0;
    gchar *retVal = g_convert(value, strlen(value), "ISO-8859-1", "UTF-8", &bytes_read, &bytes_write, &error);

    GenericFrame *frame = mowgli_dictionary_retrieve(frames, id3_frames[id3_field]);
    if (frame != NULL)
    {
        frame->header->size = strlen(retVal) + 1;
        gchar *buf = g_new0(gchar, frame->header->size + 1);
        memcpy(buf + 1, retVal, frame->header->size);
        frame->frame_body = buf;
    }
    else
        add_newFrameFromTupleStr(tuple, field, id3_field);

}

void add_frameFromTupleInt(Tuple * tuple, int field, int id3_field)
{
    int intvalue = tuple_get_int(tuple, field, NULL);
    gchar *value = g_strdup_printf("%d", intvalue);
    GError *error = NULL;
    gsize bytes_read = 0, bytes_write = 0;
    gchar *retVal = g_convert(value, strlen(value), "ISO-8859-1", "UTF-8", &bytes_read, &bytes_write, &error);

    GenericFrame *frame = mowgli_dictionary_retrieve(frames, id3_frames[id3_field]);
    if (frame != NULL)
    {
        frame->header->size = strlen(retVal) + 1;
        gchar *buf = g_new0(gchar, frame->header->size + 1);
        memcpy(buf + 1, retVal, frame->header->size);
        frame->frame_body = buf;
    }
    else
        add_newFrameFromTupleStr(tuple, field, id3_field);

}

gboolean id3v2_can_handle_file(VFSFile * f)
{
    ID3v2Header *header = readHeader(f);
    if (!strcmp(header->id3, "ID3"))
        return TRUE;
    return FALSE;
}



Tuple *id3v2_populate_tuple_from_file(Tuple * tuple, VFSFile * f)
{
    vfs_fseek(f, 0, SEEK_SET);
    ExtendedHeader *extHeader;
    ID3v2Header *header = readHeader(f);
    int pos = 0;
    if (isExtendedHeader(header))
    {
        extHeader = readExtendedHeader(f);
        vfs_fseek(f, 10 + extHeader->header_size, SEEK_SET);
    }

    while (pos < header->size)
    {
        ID3v2FrameHeader *frame = readID3v2FrameHeader(f);
        if (frame->size == 0)
            break;
        int id = getFrameID(frame);
        pos = pos + frame->size + 10;
        if (pos > header->size)
            break;
        switch (id)
        {
          case ID3_ALBUM:
              associate_string (tuple, f, FIELD_ALBUM, NULL, frame);
              break;
          case ID3_TITLE:
              associate_string (tuple, f, FIELD_TITLE, NULL, frame);
              break;
          case ID3_COMPOSER:
              associate_string (tuple, f, FIELD_COMPOSER, NULL, frame);
              break;
          case ID3_COPYRIGHT:
              associate_string (tuple, f, FIELD_COPYRIGHT, NULL, frame);
              break;
          case ID3_DATE:
              associate_string (tuple, f, FIELD_DATE, NULL, frame);
              break;
          case ID3_TIME:
              associate_int (tuple, f, FIELD_LENGTH, NULL, frame);
              break;
          case ID3_LENGTH:
              associate_int (tuple, f, FIELD_LENGTH, NULL, frame);
              break;
          case ID3_ARTIST:
              associate_string (tuple, f, FIELD_ARTIST, NULL, frame);
              break;
          case ID3_TRACKNR:
              associate_int (tuple, f, FIELD_TRACK_NUMBER, NULL, frame);
              break;
          case ID3_YEAR:
          case ID3_RECORDING_TIME:
              associate_int (tuple, f, FIELD_YEAR, NULL, frame);
              break;
          case ID3_GENRE:
              tuple = decodeGenre(tuple, f, *frame);
              break;
          case ID3_COMMENT:
              decode_comment (tuple, f, frame);
              break;
          case ID3_PRIVATE:
              decode_private_info (tuple, f, frame);
              break;
          case ID3_ENCODER:
              associate_string (tuple, f, -1, "encoder", frame);
              break;
          case ID3_TXXX:
              decode_txxx (tuple, f, frame);
              break;
          default:
              AUDDBG("Skipping %i bytes over unsupported ID3 frame %s\n", frame->size, frame->frame_id);
              skipFrame(f, frame->size);
        }
    }
    return tuple;
}


gboolean id3v2_write_tuple_to_file(Tuple * tuple, VFSFile * f)
{
    VFSFile *tmp;
    vfs_fseek(f, 0, SEEK_SET);

    ExtendedHeader *extHeader;
    if (frameIDs != NULL)
    {
        mowgli_node_t *n, *tn;
        MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
        {
            mowgli_node_delete(n, frameIDs);
        }
    }
    frameIDs = mowgli_list_create();
    ID3v2Header *header = readHeader(f);
    int framesSize = header->size;

    if (isExtendedHeader(header))
    {
        extHeader = readExtendedHeader(f);
        framesSize -= 10;
        framesSize -= extHeader->padding_size;
    }

    //read all frames into generic frames;
    frames = mowgli_dictionary_create(strcasecmp);
    readAllFrames(f, header->size);

    //make the new frames from tuple and replace in the dictionary the old frames with the new ones
    if (tuple_get_string(tuple, FIELD_ARTIST, NULL))
        add_frameFromTupleStr(tuple, FIELD_ARTIST, ID3_ARTIST);

    if (tuple_get_string(tuple, FIELD_TITLE, NULL))
        add_frameFromTupleStr(tuple, FIELD_TITLE, ID3_TITLE);

    if (tuple_get_string(tuple, FIELD_ALBUM, NULL))
        add_frameFromTupleStr(tuple, FIELD_ALBUM, ID3_ALBUM);

    if (tuple_get_string(tuple, FIELD_COMMENT, NULL))
        add_frameFromTupleStr(tuple, FIELD_COMMENT, ID3_COMMENT);

    if (tuple_get_string(tuple, FIELD_GENRE, NULL))
        add_frameFromTupleStr(tuple, FIELD_GENRE, ID3_GENRE);

    if (tuple_get_int(tuple, FIELD_YEAR, NULL) != 0)
        add_frameFromTupleInt(tuple, FIELD_YEAR, ID3_YEAR);

    if (tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL) != 0)
        add_frameFromTupleInt(tuple, FIELD_TRACK_NUMBER, ID3_TRACKNR);

    const gchar *tmpdir = g_get_tmp_dir();
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "tmp.mpc");
    tmp = vfs_fopen(tmp_path, "w+");

    int oldSize = header->size;
    header->size = TAG_SIZE * 1024;

    writeID3HeaderToFile(tmp, header);

    int size = writeAllFramesToFile(tmp);
    writePaddingToFile(tmp, TAG_SIZE * 1024 - size - 10);

    copyAudioToFile(f, tmp, oldSize);


    gchar *uri = g_strdup(f->uri);
    vfs_fclose(tmp);
    gchar *f1 = g_filename_from_uri(tmp_path, NULL, NULL);
    gchar *f2 = g_filename_from_uri(uri, NULL, NULL);
    if (g_rename(f1, f2) == 0)
    {
        AUDDBG("the tag was updated successfully\n");
    }
    else
    {
        AUDDBG("an error has occured\n");
    }
    return TRUE;
}
