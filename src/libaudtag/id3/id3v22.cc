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

#include <stdlib.h>
#include <string.h>

#define WANT_AUD_BSWAP
#include <libaudcore/audio.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/runtime.h>
#include <libaudtag/builtin.h>
#include <libaudtag/util.h>

#include "id3-common.h"

enum
{
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_LENGTH,
    ID3_ARTIST,
    ID3_ALBUM_ARTIST,
    ID3_TRACKNR,
    ID3_YEAR,
    ID3_GENRE,
    ID3_COMMENT,
    ID3_ENCODER,
    ID3_TXX,
    ID3_RVA,
    ID3_PIC,
    ID3_TAGS_NO
};

static const char * id3_frames[ID3_TAGS_NO] = {
    "TAL",
    "TT2",
    "TCM",
    "TCR",
    "TDA",
    "TLE",
    "TP1",
    "TP2",
    "TRK",
    "TYE",
    "TCO",
    "COM",
    "TSS",
    "TXX",
    "RVA",
    "PIC"
};

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

namespace audtag {

static bool validate_header (ID3v2Header * header)
{
    if (memcmp (header->magic, "ID3", 3))
        return false;

    if ((header->version != 2))
        return false;

    header->size = unsyncsafe32 (FROM_BE32 (header->size));

    AUDDBG ("Found ID3v2 header:\n");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (int) header->version);
    AUDDBG (" revision = %d\n", (int) header->revision);
    AUDDBG (" flags = %x\n", (int) header->flags);
    AUDDBG (" size = %d\n", (int) header->size);
    return true;
}

static bool read_header (VFSFile & handle, int * version, bool *
 syncsafe, int64_t * offset, int * header_size, int * data_size)
{
    ID3v2Header header;

    if (handle.fseek (0, VFS_SEEK_SET))
        return false;

    if (handle.fread (& header, 1, sizeof (ID3v2Header)) != sizeof
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

static bool read_frame (VFSFile & handle, int max_size, int version,
 bool syncsafe, int * frame_size, GenericFrame & frame)
{
    ID3v2FrameHeader header;
    uint32_t hdrsz = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return false;

    if (handle.fread (& header, 1, sizeof (ID3v2FrameHeader)) != sizeof
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
        return false;

    AUDDBG ("Found frame:\n");
    AUDDBG (" key = %.3s\n", header.key);
    AUDDBG (" size = %d\n", (int) hdrsz);

    * frame_size = sizeof (ID3v2FrameHeader) + hdrsz;

    frame.key = String (str_copy (header.key, 3));
    frame.clear ();
    frame.insert (0, hdrsz);

    if (handle.fread (& frame[0], 1, frame.len ()) != frame.len ())
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

bool ID3v22TagModule::can_handle_file (VFSFile & handle)
{
    int version, header_size, data_size;
    bool syncsafe;
    int64_t offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size);
}

bool ID3v22TagModule::read_tag (VFSFile & handle, Tuple & tuple, Index<char> * image)
{
    int version, header_size, data_size;
    bool syncsafe;
    int64_t offset;
    int pos;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return false;

    AUDDBG ("Reading tags from %i bytes of ID3 data in %s\n", data_size,
     handle.filename ());

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
            id3_associate_string (tuple, Tuple::Album, & frame[0], frame.len ());
            break;
          case ID3_TITLE:
            id3_associate_string (tuple, Tuple::Title, & frame[0], frame.len ());
            break;
          case ID3_COMPOSER:
            id3_associate_string (tuple, Tuple::Composer, & frame[0], frame.len ());
            break;
          case ID3_COPYRIGHT:
            id3_associate_string (tuple, Tuple::Copyright, & frame[0], frame.len ());
            break;
          case ID3_DATE:
            id3_associate_string (tuple, Tuple::Date, & frame[0], frame.len ());
            break;
          case ID3_LENGTH:
            id3_associate_length (tuple, & frame[0], frame.len ());
            break;
          case ID3_ARTIST:
            id3_associate_string (tuple, Tuple::Artist, & frame[0], frame.len ());
            break;
          case ID3_ALBUM_ARTIST:
            id3_associate_string (tuple, Tuple::AlbumArtist, & frame[0], frame.len ());
            break;
          case ID3_TRACKNR:
            id3_associate_int (tuple, Tuple::Track, & frame[0], frame.len ());
            break;
          case ID3_YEAR:
            id3_associate_int (tuple, Tuple::Year, & frame[0], frame.len ());
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
          case ID3_PIC:
            if (image)
                * image = id3_decode_picture (& frame[0], frame.len ());
            break;
          default:
            AUDDBG ("Ignoring unsupported ID3 frame %s.\n", (const char *) frame.key);
            break;
        }

        pos += frame_size;
    }

    return true;
}

}
