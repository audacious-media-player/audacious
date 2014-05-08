/*
 * id3v22.c
 * Copyright 2009-2014 Paula Stanciu, Tony Vroon, John Lindgren,
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
#include <libaudcore/runtime.h>

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
struct ID3v2Header {
    char magic[3];
    unsigned char version;
    unsigned char revision;
    unsigned char flags;
    uint32_t size;
};

struct ID3v2FrameHeader {
    char key[3];
    unsigned char size[3];
};
#pragma pack(pop)

struct GenericFrame : public Index<char> {
    String key;
};

#define ID3_HEADER_SYNCSAFE             0x40
#define ID3_HEADER_COMPRESSED           0x20

static bool validate_header (ID3v2Header * header)
{
    if (memcmp (header->magic, "ID3", 3))
        return false;

    if ((header->version != 2))
        return false;

    header->size = unsyncsafe32 (GUINT32_FROM_BE (header->size));

    AUDDBG ("Found ID3v2 header:\n");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (int) header->version);
    AUDDBG (" revision = %d\n", (int) header->revision);
    AUDDBG (" flags = %x\n", (int) header->flags);
    AUDDBG (" size = %d\n", (int) header->size);
    return true;
}

static bool read_header (VFSFile * handle, int * version, bool *
 syncsafe, int64_t * offset, int * header_size, int * data_size)
{
    ID3v2Header header;

    if (vfs_fseek (handle, 0, SEEK_SET))
        return false;

    if (vfs_fread (& header, 1, sizeof (ID3v2Header), handle) != sizeof
     (ID3v2Header))
        return false;

    if (validate_header (& header))
    {
        * offset = 0;
        * version = header.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = header.size;
    }
    else
        return false;

    * syncsafe = (header.flags & ID3_HEADER_SYNCSAFE) ? true : false;

    AUDDBG ("Offset = %d, header size = %d, data size = %d\n",
     (int) * offset, * header_size, * data_size);

    return true;
}

static bool read_frame (VFSFile * handle, int max_size, int version,
 bool syncsafe, int * frame_size, GenericFrame & frame)
{
    ID3v2FrameHeader header;
    uint32_t hdrsz = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return false;

    if (vfs_fread (& header, 1, sizeof (ID3v2FrameHeader), handle) != sizeof
     (ID3v2FrameHeader))
        return false;

    if (! header.key[0]) /* padding */
        return false;

    for (int i = 0; i < 3; i++)
    {
        hdrsz |= (uint32_t) header.size[i] << ((2 - i) * 8);
        AUDDBG ("header.size[%d] = %d hdrsz %d slot %d\n", i, header.size[i], hdrsz, 2 - i);
    }

    if (hdrsz > (unsigned) max_size || hdrsz == 0)
        return FALSE;

    AUDDBG ("Found frame:\n");
    AUDDBG (" key = %.3s\n", header.key);
    AUDDBG (" size = %d\n", (int) hdrsz);

    * frame_size = sizeof (ID3v2FrameHeader) + hdrsz;

    frame.key = str_nget (header.key, 3);
    frame.clear ();
    frame.insert (0, hdrsz);

    if (vfs_fread (& frame[0], 1, frame.len (), handle) != frame.len ())
        return false;

    AUDDBG ("Data size = %d.\n", frame.len ());
    return true;
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
    bool syncsafe;
    int64_t offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size);
}

static bool_t id3v22_read_tag (Tuple * tuple, VFSFile * handle)
{
    int version, header_size, data_size;
    bool syncsafe;
    int64_t offset;
    int pos;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return false;

    AUDDBG ("Reading tags from %i bytes of ID3 data in %s\n", data_size,
     vfs_get_filename (handle));

    for (pos = 0; pos < data_size; )
    {
        int frame_size;
        GenericFrame frame;

        if (! read_frame (handle, data_size - pos, version, syncsafe, & frame_size, frame))
        {
            AUDDBG("read_frame failed at pos %i\n", pos);
            break;
        }

        switch (get_frame_id (frame.key))
        {
          case ID3_ALBUM:
            id3_associate_string (tuple, FIELD_ALBUM, & frame[0], frame.len ());
            break;
          case ID3_TITLE:
            id3_associate_string (tuple, FIELD_TITLE, & frame[0], frame.len ());
            break;
          case ID3_COMPOSER:
            id3_associate_string (tuple, FIELD_COMPOSER, & frame[0], frame.len ());
            break;
          case ID3_COPYRIGHT:
            id3_associate_string (tuple, FIELD_COPYRIGHT, & frame[0], frame.len ());
            break;
          case ID3_DATE:
            id3_associate_string (tuple, FIELD_DATE, & frame[0], frame.len ());
            break;
          case ID3_LENGTH:
            id3_associate_int (tuple, FIELD_LENGTH, & frame[0], frame.len ());
            break;
          case ID3_FUCKO_ARTIST:
          case ID3_ARTIST:
            id3_associate_string (tuple, FIELD_ARTIST, & frame[0], frame.len ());
            break;
          case ID3_TRACKNR:
            id3_associate_int (tuple, FIELD_TRACK_NUMBER, & frame[0], frame.len ());
            break;
          case ID3_YEAR:
            id3_associate_int (tuple, FIELD_YEAR, & frame[0], frame.len ());
            break;
          case ID3_GENRE:
            id3_decode_genre (tuple, & frame[0], frame.len ());
            break;
          case ID3_COMMENT:
            id3_decode_comment (tuple, & frame[0], frame.len ());
            break;
          case ID3_RVA:
            id3_decode_rva (tuple, & frame[0], frame.len ());
            break;
          default:
            AUDDBG ("Ignoring unsupported ID3 frame %s.\n", (const char *) frame.key);
            break;
        }

        pos += frame_size;
    }

    return true;
}

static bool_t id3v22_read_image (VFSFile * handle, void * * image_data, int64_t * image_size)
{
    int version, header_size, data_size, parsed;
    bool syncsafe;
    int64_t offset;
    bool found = false;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return false;

    for (parsed = 0; parsed < data_size && ! found; )
    {
        int frame_size, type;
        GenericFrame frame;

        if (! read_frame (handle, data_size - parsed, version, syncsafe,
         & frame_size, frame))
            break;

        if (! strcmp (frame.key, "PIC") && id3_decode_picture (& frame[0],
         frame.len (), & type, image_data, image_size))
        {
            if (type == 3) /* album cover */
                found = true;
            else if (type == 0) /* iTunes */
                found = true;
            else if (* image_data)
            {
                g_free (* image_data);
                * image_data = nullptr;
            }
        }

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
    "ID3v2.2",
    TAG_TYPE_NONE,
    id3v22_can_handle_file,
    id3v22_read_tag,
    id3v22_read_image,
    id3v22_write_tag
};
