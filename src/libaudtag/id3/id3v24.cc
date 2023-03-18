/*
 * id3v24.c
 * Copyright 2009-2014 Paula Stanciu, Tony Vroon, John Lindgren,
 *                     Mikael Magnusson, and Michał Lipski
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
#include <unistd.h>

#include <glib.h>  /* for g_utf8_to_utf16 */

#define WANT_AUD_BSWAP
#include <libaudcore/audio.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/multihash.h>
#include <libaudcore/runtime.h>
#include <libaudtag/builtin.h>
#include <libaudtag/util.h>

#include "id3-common.h"

#define MAX_TAG_SIZE 16777216  /* reject tags over 16 MB */

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
    ID3_RECORDING_TIME,
    ID3_PUBLISHER,
    ID3_TXXX,
    ID3_RVA2,
    ID3_APIC,
    ID3_LYRICS,
    ID3_TAGS_NO
};

static const char * id3_frames[ID3_TAGS_NO] = {
    "TALB",
    "TIT2",
    "TCOM",
    "TCOP",
    "TDAT",
    "TLEN",
    "TPE1",
    "TPE2",
    "TRCK",
    "TYER",
    "TCON",
    "COMM",
    "TSSE",
    "TDRC",
    "TPUB",
    "TXXX",
    "RVA2",
    "APIC",
    "USLT"
};

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
struct ID3v24Header {
    char magic[3];
    unsigned char version;
    unsigned char revision;
    unsigned char flags;
    uint32_t size;
};

struct ID3v24FrameHeader {
    char key[4];
    uint32_t size;
    uint16_t flags;
};
#pragma pack(pop)

struct GenericFrame : public Index<char> {
    String key;
};

typedef Index<GenericFrame> FrameList;
typedef SimpleHash<String, FrameList> FrameDict;

#define ID3_HEADER_SYNCSAFE             0x80
#define ID3_HEADER_HAS_EXTENDED_HEADER  0x40
#define ID3_HEADER_HAS_FOOTER           0x10

#define ID3_FRAME_HAS_GROUP   0x0040
#define ID3_FRAME_COMPRESSED  0x0008
#define ID3_FRAME_ENCRYPTED   0x0004
#define ID3_FRAME_SYNCSAFE    0x0002
#define ID3_FRAME_HAS_LENGTH  0x0001

namespace audtag {

static bool skip_extended_header_3 (VFSFile & handle, int * _size)
{
    uint32_t size;

    if (handle.fread (& size, 1, 4) != 4)
        return false;

    size = FROM_BE32 (size);

    AUDDBG ("Found v2.3 extended header, size = %d.\n", (int) size);

    if (handle.fseek (size, VFS_SEEK_CUR))
        return false;

    * _size = 4 + size;
    return true;
}

static bool skip_extended_header_4 (VFSFile & handle, int * _size)
{
    uint32_t size;

    if (handle.fread (& size, 1, 4) != 4)
        return false;

    size = unsyncsafe32 (FROM_BE32 (size));

    AUDDBG ("Found v2.4 extended header, size = %d.\n", (int) size);

    if (handle.fseek (size - 4, VFS_SEEK_CUR))
        return false;

    * _size = size;
    return true;
}

static bool validate_header (ID3v24Header * header, bool is_footer)
{
    if (memcmp (header->magic, is_footer ? "3DI" : "ID3", 3))
        return false;

    if ((header->version != 3 && header->version != 4) || header->revision != 0)
        return false;

    header->size = unsyncsafe32 (FROM_BE32 (header->size));
    if (header->size > MAX_TAG_SIZE)
        return false;

    AUDDBG ("Found ID3v2 %s:\n", is_footer ? "footer" : "header");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (int) header->version);
    AUDDBG (" revision = %d\n", (int) header->revision);
    AUDDBG (" flags = %x\n", (int) header->flags);
    AUDDBG (" size = %d\n", (int) header->size);
    return true;
}

struct HeaderInfo {
    int64_t offset = 0;
    int header_size = 0;
    int data_size = 0;
    int footer_size = 0;
    int version = 0;
    bool syncsafe = false;
    bool valid = false;
};

static HeaderInfo read_header (VFSFile & handle)
{
    HeaderInfo info;
    ID3v24Header header, footer;

    if (handle.fseek (0, VFS_SEEK_SET))
        return info;

    if (handle.fread (& header, 1, sizeof (ID3v24Header)) != sizeof (ID3v24Header))
        return info;

    if (validate_header (& header, false))
    {
        info.version = header.version;
        info.header_size = sizeof (ID3v24Header);
        info.data_size = header.size;

        if (header.flags & ID3_HEADER_HAS_FOOTER)
        {
            if (handle.fseek (header.size, VFS_SEEK_CUR))
                return info;

            if (handle.fread (& footer, 1, sizeof (ID3v24Header)) != sizeof (ID3v24Header))
                return info;

            if (! validate_header (& footer, true))
                return info;

            if (handle.fseek (sizeof (ID3v24Header), VFS_SEEK_SET))
                return info;

            info.footer_size = sizeof (ID3v24Header);
        }
    }
    else
    {
        int64_t end = handle.fsize ();

        if (end < 0)
            return info;

        if (handle.fseek (end - sizeof (ID3v24Header), VFS_SEEK_SET))
            return info;

        if (handle.fread (& footer, 1, sizeof (ID3v24Header)) != sizeof (ID3v24Header))
            return info;

        if (! validate_header (& footer, true))
            return info;

        info.offset = end - 2 * sizeof (ID3v24Header) - footer.size;
        info.version = footer.version;
        info.header_size = sizeof (ID3v24Header);
        info.data_size = footer.size;
        info.footer_size = sizeof (ID3v24Header);

        if (handle.fseek (info.offset, VFS_SEEK_SET))
            return info;

        if (handle.fread (& header, 1, sizeof (ID3v24Header)) != sizeof
         (ID3v24Header))
            return info;

        if (! validate_header (& header, false))
            return info;
    }

    // this flag indicates tag-level unsynchronisation in ID3v2.3
    // ID3v2.4 uses frame-level unsynchronisation, rendering this flag meaningless
    info.syncsafe = (info.version == 3) && (header.flags & ID3_HEADER_SYNCSAFE);

    if (header.flags & ID3_HEADER_HAS_EXTENDED_HEADER)
    {
        int extended_size = 0;

        if (header.version == 3)
        {
            if (! skip_extended_header_3 (handle, & extended_size))
                return info;
        }
        else if (header.version == 4)
        {
            if (! skip_extended_header_4 (handle, & extended_size))
                return info;
        }

        if (extended_size > info.data_size)
            return info;

        info.header_size += extended_size;
        info.data_size -= extended_size;
    }

    AUDDBG ("Offset = %d, header size = %d, data size = %d, footer size = %d.\n",
     (int) info.offset, info.header_size, info.data_size, info.footer_size);

    info.valid = true;
    return info;
}

static void unsyncsafe (Index<char> & data)
{
    const char * get = data.begin (), * end = data.end ();
    char * set = data.begin ();
    const char * c;

    while ((c = (const char *) memchr (get, 0xff, end - get)))
    {
        c ++;
        memmove (set, get, c - get);
        set += c - get;
        get = c;

        if (get < end && ! get[0])
            get ++;
    }

    memmove (set, get, end - get);
    set += end - get;

    data.remove (set - data.begin (), -1);
}

static Index<char> read_tag_data (VFSFile & handle, int size, bool syncsafe)
{
    Index<char> data;
    data.resize (size);
    data.resize (handle.fread (data.begin (), 1, size));

    if (syncsafe)
        unsyncsafe (data);

    return data;
}

struct ReadFrameRet : public GenericFrame {
    int size = 0; /* including header */
    bool valid = false;
};

static ReadFrameRet read_frame (const char * data, int max_size, int version)
{
    ReadFrameRet frame;
    ID3v24FrameHeader header;
    unsigned skip = 0;

    if ((max_size -= sizeof (ID3v24FrameHeader)) < 0)
        return frame;

    memcpy (& header, data, sizeof (ID3v24FrameHeader));
    data += sizeof (ID3v24FrameHeader);

    if (! header.key[0]) /* padding */
        return frame;

    header.size = (version == 3) ? FROM_BE32 (header.size) : unsyncsafe32 (FROM_BE32 (header.size));
    header.flags = FROM_BE16 (header.flags);

    if (header.size > (unsigned) max_size)
        return frame;

    // set frame size here so we can continue past empty/invalid frames
    frame.size = sizeof (ID3v24FrameHeader) + header.size;

    if (! header.size)
        return frame;

    AUDDBG ("Found frame:\n");
    AUDDBG (" key = %.4s\n", header.key);
    AUDDBG (" size = %d\n", (int) header.size);
    AUDDBG (" flags = %x\n", (int) header.flags);

    if (header.flags & (ID3_FRAME_COMPRESSED | ID3_FRAME_ENCRYPTED))
    {
        AUDDBG ("Hit compressed/encrypted frame %.4s.\n", header.key);
        return frame;
    }

    if (header.flags & ID3_FRAME_HAS_GROUP)
        skip ++;
    if (header.flags & ID3_FRAME_HAS_LENGTH)
        skip += 4;

    if (skip >= header.size)
        return frame;

    frame.key = String (str_copy (header.key, 4));
    frame.insert (data + skip, 0, header.size - skip);

    if (header.flags & ID3_FRAME_SYNCSAFE)
        unsyncsafe (frame);

    AUDDBG ("Data size = %d.\n", frame.len ());
    frame.valid = true;
    return frame;
}

static void read_all_frames (const Index<char> & data, int version, FrameDict & dict)
{
    for (const char * pos = data.begin (); pos < data.end (); )
    {
        auto frame = read_frame (pos, data.end () - pos, version);
        if (! frame.size)
            break;

        pos += frame.size;
        if (! frame.valid)
            continue;

        FrameList * list = dict.lookup (frame.key);
        if (! list)
            list = dict.add (frame.key, FrameList ());

        list->append (std::move (frame));
    }
}

static bool write_frame (VFSFile & file, const GenericFrame & frame, int version, int * frame_size)
{
    AUDDBG ("Writing frame %s, size %d\n", (const char *) frame.key, frame.len ());

    ID3v24FrameHeader header;

    strncpy (header.key, frame.key, 4);

    uint32_t size = frame.len ();
    if (version > 3)
        size = syncsafe32 (size);

    header.size = TO_BE32 (size);
    header.flags = 0;

    if (file.fwrite (& header, 1, sizeof (ID3v24FrameHeader)) != sizeof (ID3v24FrameHeader))
        return false;

    if (file.fwrite (& frame[0], 1, frame.len ()) != frame.len ())
        return false;

    * frame_size = sizeof (ID3v24FrameHeader) + frame.len ();
    return true;
}

static int write_all_frames (VFSFile & file, FrameDict & dict, int version)
{
    int written_size = 0;
    Index<const FrameList *> lists;
    const FrameList * apic_list = nullptr;

    dict.iterate ([&] (const String & key, const FrameList & list) {
        if (list.len ()) {
            if (strcmp (key, id3_frames[ID3_APIC]))
                lists.append (& list);
            else
                apic_list = & list;
        }
    });

    // write frames in key order
    lists.sort ([] (const FrameList * a, const FrameList * b) {
        return strcmp ((* a)[0].key, (* b)[0].key);
    });

    // put APIC frames last
    if (apic_list)
        lists.append (apic_list);

    for (const FrameList * list : lists)
    {
        for (const GenericFrame & frame : * list)
        {
            int size;
            if (write_frame (file, frame, version, & size))
                written_size += size;
        }
    }

    AUDDBG ("Total frame bytes written = %d.\n", written_size);
    return written_size;
}

static bool write_header (VFSFile & file, int version, int size)
{
    ID3v24Header header;

    memcpy (header.magic, "ID3", 3);
    header.version = version;
    header.revision = 0;
    header.flags = 0;
    header.size = TO_BE32 (syncsafe32 (size));

    return file.fwrite (& header, 1, sizeof (ID3v24Header)) == sizeof (ID3v24Header);
}

static int get_frame_id (const char * key)
{
    for (int id = 0; id < ID3_TAGS_NO; id ++)
    {
        if (! strcmp (key, id3_frames[id]))
            return id;
    }

    return -1;
}

static GenericFrame & add_generic_frame (int id, int size, FrameDict & dict)
{
    String key (id3_frames[id]);
    FrameList * list = dict.add (key, FrameList ());

    GenericFrame & frame = list->append ();

    frame.key = key;
    frame.insert (0, size);

    return frame;
}

static void remove_frame (int id, FrameDict & dict)
{
    AUDDBG ("Deleting frame %s.\n", id3_frames[id]);
    dict.remove (String (id3_frames[id]));
}

static void add_text_frame (int id, const char * text, FrameDict & dict)
{
    if (! text)
    {
        remove_frame (id, dict);
        return;
    }

    AUDDBG ("Adding text frame %s = %s.\n", id3_frames[id], text);

    long words;
    uint16_t * utf16 = g_utf8_to_utf16 (text, -1, nullptr, & words, nullptr);
    g_return_if_fail (utf16);

    GenericFrame & frame = add_generic_frame (id, 3 + 2 * words, dict);

    frame[0] = 1;                             /* UTF-16 encoding */
    * (uint16_t *) (& frame[1]) = 0xfeff;     /* byte order mark */
    memcpy (& frame[3], utf16, 2 * words);

    g_free (utf16);
}

static void add_memo_frame (int id, const char * text, FrameDict & dict)
{
    if (! text)
    {
        remove_frame (id, dict);
        return;
    }

    AUDDBG ("Adding comment frame = %s.\n", text);

    long words;
    uint16_t * utf16 = g_utf8_to_utf16 (text, -1, nullptr, & words, nullptr);
    g_return_if_fail (utf16);

    GenericFrame & frame = add_generic_frame (id, 10 + 2 * words, dict);

    frame[0] = 1;                              /* UTF-16 encoding */
    memcpy (& frame[1], "eng", 3);             /* language */
    * (uint16_t *) (& frame[4]) = 0xfeff;      /* byte order mark */
    * (uint16_t *) (& frame[6]) = 0;           /* end of content description */
    * (uint16_t *) (& frame[8]) = 0xfeff;      /* byte order mark */
    memcpy (& frame[10], utf16, 2 * words);

    g_free (utf16);
}

static void add_frameFromTupleStr (const Tuple & tuple, Tuple::Field field,
 int id3_field, FrameDict & dict)
{
    add_text_frame (id3_field, tuple.get_str (field), dict);
}

static void add_frameFromTupleInt (const Tuple & tuple, Tuple::Field field,
 int id3_field, FrameDict & dict)
{
    if (tuple.get_value_type (field) == Tuple::Int)
        add_text_frame (id3_field, int_to_str (tuple.get_int (field)), dict);
    else
        remove_frame (id3_field, dict);
}

bool ID3v24TagModule::can_handle_file (VFSFile & handle)
{
    auto info = read_header (handle);
    return info.valid;
}

bool ID3v24TagModule::read_tag (VFSFile & handle, Tuple & tuple, Index<char> * image)
{
    auto info = read_header (handle);
    if (! info.valid)
        return false;

    auto data = read_tag_data (handle, info.data_size, info.syncsafe);
    FrameList rva_frames;

    for (const char * pos = data.begin (); pos < data.end (); )
    {
        auto frame = read_frame (pos, data.end () - pos, info.version);
        if (! frame.size)
            break;

        pos += frame.size;
        if (! frame.valid)
            continue;

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
          case ID3_RECORDING_TIME:
            id3_associate_int (tuple, Tuple::Year, & frame[0], frame.len ());
            break;
          case ID3_PUBLISHER:
            id3_associate_string (tuple, Tuple::Publisher, & frame[0], frame.len ());
            break;
          case ID3_GENRE:
            id3_decode_genre (tuple, & frame[0], frame.len ());
            break;
          case ID3_COMMENT:
            id3_associate_memo (tuple, Tuple::Comment, & frame[0], frame.len ());
            break;
          case ID3_TXXX:
            id3_decode_txxx (tuple, & frame[0], frame.len ());
            break;
          case ID3_RVA2:
            rva_frames.append (std::move (frame));
            break;
          case ID3_APIC:
            if (image)
                * image = id3_decode_apic (& frame[0], frame.len ());
            break;
          case ID3_LYRICS:
            id3_associate_memo (tuple, Tuple::Lyrics, & frame[0], frame.len ());
            break;
          default:
            AUDDBG ("Ignoring unsupported ID3 frame %s.\n", (const char *) frame.key);
            break;
        }
    }

    /* only decode RVA2 frames if Replay Gain was not found in TXXX frames */
    if (! tuple.is_set (Tuple::GainDivisor) && ! tuple.is_set (Tuple::PeakDivisor))
    {
        for (auto & rva : rva_frames)
            id3_decode_rva (tuple, & rva[0], rva.len ());
    }

    return true;
}

bool ID3v24TagModule::write_tag (VFSFile & f, const Tuple & tuple)
{
    //read all frames into generic frames;
    FrameDict dict;

    auto info = read_header (f);
    if (info.valid)
        read_all_frames (read_tag_data (f, info.data_size, info.syncsafe), info.version, dict);

    //make the new frames from tuple and replace in the dictionary the old frames with the new ones
    add_frameFromTupleStr (tuple, Tuple::Title, ID3_TITLE, dict);
    add_frameFromTupleStr (tuple, Tuple::Artist, ID3_ARTIST, dict);
    add_frameFromTupleStr (tuple, Tuple::Album, ID3_ALBUM, dict);
    add_frameFromTupleStr (tuple, Tuple::AlbumArtist, ID3_ALBUM_ARTIST, dict);
    add_frameFromTupleStr (tuple, Tuple::Composer, ID3_COMPOSER, dict);
    add_frameFromTupleStr (tuple, Tuple::Publisher, ID3_PUBLISHER, dict);
    add_frameFromTupleStr (tuple, Tuple::Copyright, ID3_COPYRIGHT, dict);
    add_frameFromTupleInt (tuple, Tuple::Year, ID3_YEAR, dict);
    add_frameFromTupleInt (tuple, Tuple::Track, ID3_TRACKNR, dict);
    add_frameFromTupleStr (tuple, Tuple::Genre, ID3_GENRE, dict);

    String comment = tuple.get_str (Tuple::Comment);
    add_memo_frame (ID3_COMMENT, comment, dict);

    String lyrics = tuple.get_str (Tuple::Lyrics);
    add_memo_frame (ID3_LYRICS, lyrics, dict);

    /* location and size of non-tag data */
    int64_t mp3_offset = 0;
    int64_t mp3_size = -1;

    if (info.valid && ! info.offset)  /* existing tag at start */
        mp3_offset = info.header_size + info.data_size + info.footer_size;
    if (info.valid && info.offset)    /* existing tag at end */
        mp3_size = info.offset;

    auto temp = VFSFile::tmpfile ();
    if (! temp)
        return false;

    /* write empty header (will be overwritten later) */
    int version = info.valid ? info.version : 3;
    if (! write_header (temp, version, 0))
        return false;

    /* write tag data */
    int data_size = write_all_frames (temp, dict, version);

    /* copy non-tag data */
    if (f.fseek (mp3_offset, VFS_SEEK_SET) < 0 || ! temp.copy_from (f, mp3_size))
        return false;

    /* go back to beginning and write real header */
    if (temp.fseek (0, VFS_SEEK_SET) < 0 || ! write_header (temp, version, data_size))
        return false;

    if (! f.replace_with (temp))
        return false;

    return true;
}

}
