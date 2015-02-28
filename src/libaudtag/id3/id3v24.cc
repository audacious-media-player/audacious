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
    ID3_PRIVATE,
    ID3_ENCODER,
    ID3_RECORDING_TIME,
    ID3_TXXX,
    ID3_RVA2,
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
    "PRIV",
    "TSSE",
    "TDRC",
    "TXXX",
    "RVA2"
};

/*
static const unsigned char PRIMARY_CLASS_MUSIC[16] = {0xBC, 0x7D, 0x60, 0xD1, 0x23,
 0xE3, 0xE2, 0x4B, 0x86, 0xA1, 0x48, 0xA4, 0x2A, 0x28, 0x44, 0x1E};
static const unsigned char PRIMARY_CLASS_AUDIO[16] = {0x29, 0x0F, 0xCD, 0x01, 0x4E,
 0xDA, 0x57, 0x41, 0x89, 0x7B, 0x62, 0x75, 0xD5, 0x0C, 0x4F, 0x11};
static const unsigned char SECONDARY_CLASS_AUDIOBOOK[16] = {0xEB, 0x6B, 0x23, 0xE0,
 0x81, 0xC2, 0xDE, 0x4E, 0xA3, 0x6D, 0x7A, 0xF7, 0x6A, 0x3D, 0x45, 0xB5};
static const unsigned char SECONDARY_CLASS_SPOKENWORD[16] = {0x13, 0x2A, 0x17, 0x3A,
 0xD9, 0x2B, 0x31, 0x48, 0x83, 0x5B, 0x11, 0x4F, 0x6A, 0x95, 0x94, 0x3F};
static const unsigned char SECONDARY_CLASS_NEWS[16] = {0x9B, 0xDB, 0x77, 0x66, 0xA0,
 0xE5, 0x63, 0x40, 0xA1, 0xAD, 0xAC, 0xEB, 0x52, 0x84, 0x0C, 0xF1};
static const unsigned char SECONDARY_CLASS_TALKSHOW[16] = {0x67, 0x4A, 0x82, 0x1B,
 0x80, 0x3F, 0x3E, 0x4E, 0x9C, 0xDE, 0xF7, 0x36, 0x1B, 0x0F, 0x5F, 0x1B};
static const unsigned char SECONDARY_CLASS_GAMES_CLIP[16] = {0x68, 0x33, 0x03, 0x00,
 0x09, 0x50, 0xC3, 0x4A, 0xA8, 0x20, 0x5D, 0x2D, 0x09, 0xA4, 0xE7, 0xC1};
static const unsigned char SECONDARY_CLASS_GAMES_SONG[16] = {0x31, 0xF7, 0x4F, 0xF2,
 0xFC, 0x96, 0x0F, 0x4D, 0xA2, 0xF5, 0x5A, 0x34, 0x83, 0x68, 0x2B, 0x1A};
*/

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

static bool validate_header (ID3v2Header * header, bool is_footer)
{
    if (memcmp (header->magic, is_footer ? "3DI" : "ID3", 3))
        return false;

    if ((header->version != 3 && header->version != 4) || header->revision != 0)
        return false;

    header->size = unsyncsafe32 (FROM_BE32 (header->size));

    AUDDBG ("Found ID3v2 %s:\n", is_footer ? "footer" : "header");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (int) header->version);
    AUDDBG (" revision = %d\n", (int) header->revision);
    AUDDBG (" flags = %x\n", (int) header->flags);
    AUDDBG (" size = %d\n", (int) header->size);
    return true;
}

static bool read_header (VFSFile & handle, int * version, bool *
 syncsafe, int64_t * offset, int * header_size, int * data_size, int *
 footer_size)
{
    ID3v2Header header, footer;

    if (handle.fseek (0, VFS_SEEK_SET))
        return false;

    if (handle.fread (& header, 1, sizeof (ID3v2Header)) != sizeof
     (ID3v2Header))
        return false;

    if (validate_header (& header, false))
    {
        * offset = 0;
        * version = header.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = header.size;

        if (header.flags & ID3_HEADER_HAS_FOOTER)
        {
            if (handle.fseek (header.size, VFS_SEEK_CUR))
                return false;

            if (handle.fread (& footer, 1, sizeof (ID3v2Header)) != sizeof
             (ID3v2Header))
                return false;

            if (! validate_header (& footer, true))
                return false;

            if (handle.fseek (sizeof (ID3v2Header), VFS_SEEK_SET))
                return false;

            * footer_size = sizeof (ID3v2Header);
        }
        else
            * footer_size = 0;
    }
    else
    {
        int64_t end = handle.fsize ();

        if (end < 0)
            return false;

        if (handle.fseek (end - sizeof (ID3v2Header), VFS_SEEK_SET))
            return false;

        if (handle.fread (& footer, 1, sizeof (ID3v2Header)) != sizeof
         (ID3v2Header))
            return false;

        if (! validate_header (& footer, true))
            return false;

        * offset = end - 2 * sizeof (ID3v2Header) - footer.size;
        * version = footer.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = footer.size;
        * footer_size = sizeof (ID3v2Header);

        if (handle.fseek (* offset, VFS_SEEK_SET))
            return false;

        if (handle.fread (& header, 1, sizeof (ID3v2Header)) != sizeof
         (ID3v2Header))
            return false;

        if (! validate_header (& header, false))
            return false;
    }

    // this flag indicates tag-level unsynchronisation in ID3v2.3
    // ID3v2.4 uses frame-level unsynchronisation, rendering this flag meaningless
    * syncsafe = (* version == 3) && (header.flags & ID3_HEADER_SYNCSAFE);

    if (header.flags & ID3_HEADER_HAS_EXTENDED_HEADER)
    {
        int extended_size = 0;

        if (header.version == 3)
        {
            if (! skip_extended_header_3 (handle, & extended_size))
                return false;
        }
        else if (header.version == 4)
        {
            if (! skip_extended_header_4 (handle, & extended_size))
                return false;
        }

        * header_size += extended_size;
        * data_size -= extended_size;
    }

    AUDDBG ("Offset = %d, header size = %d, data size = %d, footer size = "
     "%d.\n", (int) * offset, * header_size, * data_size, * footer_size);

    return true;
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

static bool read_frame (const char * data, int max_size, int version,
 int * frame_size, GenericFrame & frame)
{
    ID3v2FrameHeader header;
    unsigned skip = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return false;

    memcpy (& header, data, sizeof (ID3v2FrameHeader));
    data += sizeof (ID3v2FrameHeader);

    if (! header.key[0]) /* padding */
        return false;

    header.size = (version == 3) ? FROM_BE32 (header.size) : unsyncsafe32 (FROM_BE32 (header.size));
    header.flags = FROM_BE16 (header.flags);

    if (header.size > (unsigned) max_size || header.size == 0)
        return false;

    AUDDBG ("Found frame:\n");
    AUDDBG (" key = %.4s\n", header.key);
    AUDDBG (" size = %d\n", (int) header.size);
    AUDDBG (" flags = %x\n", (int) header.flags);

    if (header.flags & (ID3_FRAME_COMPRESSED | ID3_FRAME_ENCRYPTED))
    {
        AUDDBG ("Hit compressed/encrypted frame %.4s.\n", header.key);
        return false;
    }

    if (header.flags & ID3_FRAME_HAS_GROUP)
        skip ++;
    if (header.flags & ID3_FRAME_HAS_LENGTH)
        skip += 4;

    if (skip >= header.size)
        return false;

    * frame_size = sizeof (ID3v2FrameHeader) + header.size;

    frame.key = String (str_copy (header.key, 4));
    frame.clear ();
    frame.insert (data + skip, 0, header.size - skip);

    if (header.flags & ID3_FRAME_SYNCSAFE)
        unsyncsafe (frame);

    AUDDBG ("Data size = %d.\n", frame.len ());
    return true;
}

static void read_all_frames (const Index<char> & data, int version, FrameDict & dict)
{
    for (const char * pos = data.begin (); pos < data.end (); )
    {
        int frame_size;
        GenericFrame frame;

        if (! read_frame (pos, data.end () - pos, version, & frame_size, frame))
            break;

        pos += frame_size;

        FrameList * list = dict.lookup (frame.key);
        if (! list)
            list = dict.add (frame.key, FrameList ());

        list->append (std::move (frame));
    }
}

static bool write_frame (int fd, const GenericFrame & frame, int version, int * frame_size)
{
    AUDDBG ("Writing frame %s, size %d\n", (const char *) frame.key, frame.len ());

    ID3v2FrameHeader header;

    strncpy (header.key, frame.key, 4);

    uint32_t size = frame.len ();
    if (version > 3)
        size = syncsafe32 (size);

    header.size = TO_BE32 (size);
    header.flags = 0;

    if (write (fd, & header, sizeof (ID3v2FrameHeader)) != sizeof (ID3v2FrameHeader))
        return false;

    if (write (fd, & frame[0], frame.len ()) != frame.len ())
        return false;

    * frame_size = sizeof (ID3v2FrameHeader) + frame.len ();
    return true;
}

struct WriteState {
    int fd;
    int version;
    int written_size;
};

static void write_frame_list (const String & key, FrameList & list, void * user)
{
    WriteState * state = (WriteState *) user;

    for (const GenericFrame & frame : list)
    {
        int size;
        if (write_frame (state->fd, frame, state->version, & size))
            state->written_size += size;
    }
}

static int write_all_frames (int fd, FrameDict & dict, int version)
{
    WriteState state = {fd, version, 0};
    dict.iterate (write_frame_list, & state);

    AUDDBG ("Total frame bytes written = %d.\n", state.written_size);
    return state.written_size;
}

static bool write_header (int fd, int version, int size)
{
    ID3v2Header header;

    memcpy (header.magic, "ID3", 3);
    header.version = version;
    header.revision = 0;
    header.flags = 0;
    header.size = TO_BE32 (syncsafe32 (size));

    return write (fd, & header, sizeof (ID3v2Header)) == sizeof (ID3v2Header);
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

#if 0
static void decode_private_info (Tuple & tuple, const unsigned char * data, int size)
{
    char * text = g_strndup ((const char *) data, size);

    if (!strncmp(text, "WM/", 3))
    {
        char *separator = strchr(text, 0);
        if (separator == nullptr)
            goto DONE;

        char * value = separator + 1;
        if (!strncmp(text, "WM/MediaClassPrimaryID", 22))
        {
            if (!memcmp(value, PRIMARY_CLASS_MUSIC, 16))
                tuple.set_str (-1, "media-class", "Music");
            if (!memcmp(value, PRIMARY_CLASS_AUDIO, 16))
                tuple.set_str (-1, "media-class", "Audio (non-music)");
        } else if (!strncmp(text, "WM/MediaClassSecondaryID", 24))
        {
            if (!memcmp(value, SECONDARY_CLASS_AUDIOBOOK, 16))
                tuple.set_str (-1, "media-class", "Audio Book");
            if (!memcmp(value, SECONDARY_CLASS_SPOKENWORD, 16))
                tuple.set_str (-1, "media-class", "Spoken Word");
            if (!memcmp(value, SECONDARY_CLASS_NEWS, 16))
                tuple.set_str (-1, "media-class", "News");
            if (!memcmp(value, SECONDARY_CLASS_TALKSHOW, 16))
                tuple.set_str (-1, "media-class", "Talk Show");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_CLIP, 16))
                tuple.set_str (-1, "media-class", "Game Audio (clip)");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_SONG, 16))
                tuple.set_str (-1, "media-class", "Game Soundtrack");
        } else {
            AUDDBG("Unrecognised tag %s (Windows Media) ignored\n", text);
        }
    } else {
        AUDDBG("Unable to decode private data, skipping: %s\n", text);
    }

DONE:
    g_free (text);
}
#endif

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

static void add_comment_frame (const char * text, FrameDict & dict)
{
    if (! text)
    {
        remove_frame (ID3_COMMENT, dict);
        return;
    }

    AUDDBG ("Adding comment frame = %s.\n", text);

    long words;
    uint16_t * utf16 = g_utf8_to_utf16 (text, -1, nullptr, & words, nullptr);
    g_return_if_fail (utf16);

    GenericFrame & frame = add_generic_frame (ID3_COMMENT, 10 + 2 * words, dict);

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
    if (tuple.get_value_type (field) != Tuple::Int)
    {
        remove_frame (id3_field, dict);
        return;
    }

    add_text_frame (id3_field, int_to_str (tuple.get_int (field)), dict);
}

bool ID3v24TagModule::can_handle_file (VFSFile & handle)
{
    int version, header_size, data_size, footer_size;
    bool syncsafe;
    int64_t offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size);
}

bool ID3v24TagModule::read_tag (Tuple & tuple, VFSFile & handle)
{
    int version, header_size, data_size, footer_size;
    bool syncsafe;
    int64_t offset;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size))
        return false;

    Index<char> data = read_tag_data (handle, data_size, syncsafe);

    for (const char * pos = data.begin (); pos < data.end (); )
    {
        int frame_size;
        GenericFrame frame;

        if (! read_frame (pos, data.end () - pos, version, & frame_size, frame))
            break;

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
          case ID3_GENRE:
            id3_decode_genre (tuple, & frame[0], frame.len ());
            break;
          case ID3_COMMENT:
            id3_decode_comment (tuple, & frame[0], frame.len ());
            break;
#if 0
          case ID3_PRIVATE:
            decode_private_info (tuple, & frame[0], frame.len ());
            break;
#endif
          case ID3_RVA2:
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

Index<char> ID3v24TagModule::read_image (VFSFile & handle)
{
    Index<char> buf;
    int version, header_size, data_size, footer_size;
    bool syncsafe;
    int64_t offset;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size))
        return buf;

    Index<char> data = read_tag_data (handle, data_size, syncsafe);

    for (const char * pos = data.begin (); pos < data.end (); )
    {
        int frame_size;
        GenericFrame frame;

        if (! read_frame (pos, data.end () - pos, version, & frame_size, frame))
            break;

        if (! strcmp (frame.key, "APIC"))
            buf = id3_decode_picture (& frame[0], frame.len ());

        pos += frame_size;
    }

    return buf;
}

bool ID3v24TagModule::write_tag (const Tuple & tuple, VFSFile & f)
{
    int version = 3;
    int header_size, data_size, footer_size;
    bool syncsafe;
    int64_t offset;

    //read all frames into generic frames;
    FrameDict dict;

    if (read_header (f, & version, & syncsafe, & offset, & header_size, & data_size, & footer_size))
        read_all_frames (read_tag_data (f, data_size, syncsafe), version, dict);

    //make the new frames from tuple and replace in the dictionary the old frames with the new ones
    add_frameFromTupleStr (tuple, Tuple::Title, ID3_TITLE, dict);
    add_frameFromTupleStr (tuple, Tuple::Artist, ID3_ARTIST, dict);
    add_frameFromTupleStr (tuple, Tuple::Album, ID3_ALBUM, dict);
    add_frameFromTupleStr (tuple, Tuple::AlbumArtist, ID3_ALBUM_ARTIST, dict);
    add_frameFromTupleInt (tuple, Tuple::Year, ID3_YEAR, dict);
    add_frameFromTupleInt (tuple, Tuple::Track, ID3_TRACKNR, dict);
    add_frameFromTupleStr (tuple, Tuple::Genre, ID3_GENRE, dict);

    String comment = tuple.get_str (Tuple::Comment);
    add_comment_frame (comment, dict);

    /* location and size of non-tag data */
    int64_t mp3_offset = offset ? 0 : header_size + data_size + footer_size;
    int64_t mp3_size = offset ? offset : -1;

    TempFile temp = TempFile ();
    if (! open_temp_file_for (& temp, f))
        return false;

    /* write empty header (will be overwritten later) */
    if (! write_header (temp.fd, version, 0))
        return false;

    /* write tag data */
    data_size = write_all_frames (temp.fd, dict, version);

    /* copy non-tag data */
    if (! copy_region_to_temp_file (& temp, f, mp3_offset, mp3_size))
        return false;

    /* go back to beginning and write real header */
    if (lseek (temp.fd, 0, SEEK_SET) < 0 || ! write_header (temp.fd, version, data_size))
        return false;

    if (! replace_with_temp_file (& temp, f))
        return false;

    return true;
}

}
