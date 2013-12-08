/*
 * id3v22.c
 * Copyright 2009-2011 Paula Stanciu, Tony Vroon, John Lindgren,
 *                     and William Pitcock
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "id3-common.h"
#include "id3v22.h"
#include "../util.h"

enum
{
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_LENGTH,
    ID3_ARTIST,
    ID3_TRACKNR,
    ID3_YEAR,
    ID3_GENRE,
    ID3_COMMENT,
    ID3_ENCODER,
    ID3_TXX,
    ID3_RVA,
    ID3_FUCKO_ARTIST,
    ID3_TAGS_NO
};

static const char * id3_frames[ID3_TAGS_NO] = {"TAL", "TT2", "TCM", "TCR",
"TDA", "TLE", "TPE", "TRK", "TYE", "TCO", "COM", "TSS", "TXX", "RVA", "TP1"};

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
typedef struct
{
    char magic[3];
    unsigned char version;
    unsigned char revision;
    unsigned char flags;
    uint32_t size;
}
ID3v2Header;

typedef struct
{
    char key[3];
    unsigned char size[3];
}
ID3v2FrameHeader;
#pragma pack(pop)

typedef struct
{
    char key[5];
    unsigned char * data;
    int size;
}
GenericFrame;

#define ID3_HEADER_SYNCSAFE             0x40
#define ID3_HEADER_COMPRESSED           0x20

#define TAG_SIZE 1

#define write_syncsafe_int32(x) vfs_fput_be32 (syncsafe32 (x))

static bool_t validate_header (ID3v2Header * header)
{
    if (memcmp (header->magic, "ID3", 3))
        return FALSE;

    if ((header->version != 2))
        return FALSE;

    header->size = unsyncsafe32(GUINT32_FROM_BE(header->size));

    TAGDBG ("Found ID3v2 header:\n");
    TAGDBG (" magic = %.3s\n", header->magic);
    TAGDBG (" version = %d\n", (int) header->version);
    TAGDBG (" revision = %d\n", (int) header->revision);
    TAGDBG (" flags = %x\n", (int) header->flags);
    TAGDBG (" size = %d\n", (int) header->size);
    return TRUE;
}

static bool_t read_header (VFSFile * handle, int * version, bool_t *
 syncsafe, gsize * offset, int * header_size, int * data_size)
{
    ID3v2Header header;

    if (vfs_fseek (handle, 0, SEEK_SET))
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2Header), handle) != sizeof
     (ID3v2Header))
        return FALSE;

    if (validate_header (& header))
    {
        * offset = 0;
        * version = header.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = header.size;
    } else return FALSE;

    * syncsafe = (header.flags & ID3_HEADER_SYNCSAFE) ? TRUE : FALSE;

    TAGDBG ("Offset = %d, header size = %d, data size = %d\n",
     (int) * offset, * header_size, * data_size);

    return TRUE;
}

static bool_t read_frame (VFSFile * handle, int max_size, int version,
 bool_t syncsafe, int * frame_size, char * key, char * * data, int * size)
{
    ID3v2FrameHeader header;
    int i;
    uint32_t hdrsz = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2FrameHeader), handle) != sizeof
     (ID3v2FrameHeader))
        return FALSE;

    if (! header.key[0]) /* padding */
        return FALSE;

    for (i = 0; i < 3; i++)
    {
        hdrsz |= (uint32_t) header.size[i] << ((2 - i) * 8);
        TAGDBG("header.size[%d] = %d hdrsz %d slot %d\n", i, header.size[i], hdrsz, 2 - i);
    }

//    hdrsz = GUINT32_TO_BE(hdrsz);
    if (hdrsz > max_size || hdrsz == 0)
        return FALSE;

    TAGDBG ("Found frame:\n");
    TAGDBG (" key = %.3s\n", header.key);
    TAGDBG (" size = %d\n", (int) hdrsz);

    * frame_size = sizeof (ID3v2FrameHeader) + hdrsz;
    g_strlcpy (key, header.key, 4);

    * size = hdrsz;
    * data = g_malloc (* size);

    if (vfs_fread (* data, 1, * size, handle) != * size)
        return FALSE;

    TAGDBG ("Data size = %d.\n", * size);
    return TRUE;
}


static int get_frame_id (const char * key)
{
    int id;

    for (id = 0; id < ID3_TAGS_NO; id ++)
    {
        if (! strcmp (key, id3_frames[id]))
            return id;
    }

    return -1;
}

static bool_t id3v22_can_handle_file (VFSFile * handle)
{
    int version, header_size, data_size;
    bool_t syncsafe;
    gsize offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size);
}

bool_t id3v22_read_tag (Tuple * tuple, VFSFile * handle)
{
    int version, header_size, data_size;
    bool_t syncsafe;
    gsize offset;
    int pos;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return FALSE;

    TAGDBG ("Reading tags from %i bytes of ID3 data in %s\n", data_size,
     vfs_get_filename (handle));

    for (pos = 0; pos < data_size; )
    {
        int frame_size, size, id;
        char key[5];
        char * data;

        if (! read_frame (handle, data_size - pos, version, syncsafe,
         & frame_size, key, & data, & size))
        {
            TAGDBG("read_frame failed at pos %i\n", pos);
                break;
        }

        id = get_frame_id (key);

        switch (id)
        {
          case ID3_ALBUM:
            id3_associate_string (tuple, FIELD_ALBUM, data, size);
            break;
          case ID3_TITLE:
            id3_associate_string (tuple, FIELD_TITLE, data, size);
            break;
          case ID3_COMPOSER:
            id3_associate_string (tuple, FIELD_COMPOSER, data, size);
            break;
          case ID3_COPYRIGHT:
            id3_associate_string (tuple, FIELD_COPYRIGHT, data, size);
            break;
          case ID3_DATE:
            id3_associate_string (tuple, FIELD_DATE, data, size);
            break;
          case ID3_LENGTH:
            id3_associate_int (tuple, FIELD_LENGTH, data, size);
            break;
          case ID3_FUCKO_ARTIST:
          case ID3_ARTIST:
            id3_associate_string (tuple, FIELD_ARTIST, data, size);
            break;
          case ID3_TRACKNR:
            id3_associate_int (tuple, FIELD_TRACK_NUMBER, data, size);
            break;
          case ID3_YEAR:
            id3_associate_int (tuple, FIELD_YEAR, data, size);
            break;
          case ID3_GENRE:
            id3_decode_genre (tuple, data, size);
            break;
          case ID3_COMMENT:
            id3_decode_comment (tuple, data, size);
            break;
          case ID3_RVA:
            id3_decode_rva (tuple, data, size);
            break;
          default:
            TAGDBG ("Ignoring unsupported ID3 frame %s.\n", key);
            break;
        }

        g_free (data);
        pos += frame_size;
    }

    return TRUE;
}

static bool_t id3v22_read_image (VFSFile * handle, void * * image_data, int64_t * image_size)
{
    int version, header_size, data_size, parsed;
    bool_t syncsafe;
    gsize offset;
    bool_t found = FALSE;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return FALSE;

    for (parsed = 0; parsed < data_size && ! found; )
    {
        int frame_size, size, type;
        char key[5];
        char * data;
        int frame_length;

        if (! read_frame (handle, data_size - parsed, version, syncsafe,
         & frame_size, key, & data, & size))
            break;

        frame_length = size;

        if (! strcmp (key, "PIC") && id3_decode_picture (data, frame_length,
         & type, image_data, image_size))
        {
            if (type == 3) /* album cover */
                found = TRUE;
            else if (type == 0) /* iTunes */
                found = TRUE;
            else if (*image_data != NULL)
            {
                g_free(*image_data);
                *image_data = NULL;
            }
        }

        g_free (data);
        parsed += frame_size;
    }

    return found;
}

static bool_t id3v22_write_tag (const Tuple * tuple, VFSFile * f)
{
    fprintf (stderr, "Writing ID3v2.2 tags is not implemented yet, sorry.\n");
    return FALSE;
}

tag_module_t id3v22 =
{
    .name = "ID3v2.2",
    .can_handle_file = id3v22_can_handle_file,
    .read_tag = id3v22_read_tag,
    .read_image = id3v22_read_image,
    .write_tag = id3v22_write_tag,
};
