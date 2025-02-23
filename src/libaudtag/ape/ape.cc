/*
 * ape.c
 * Copyright 2010 John Lindgren
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

/* TODO:
 * - Support updating files that have their tag at the beginning?
 */

#include <stdlib.h>
#include <string.h>

#define WANT_AUD_BSWAP
#include <libaudcore/audio.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/runtime.h>
#include <libaudcore/vfs.h>
#include <libaudtag/builtin.h>

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
struct APEHeader {
    char magic[8];
    uint32_t version; /* LE */
    uint32_t length; /* LE */
    uint32_t items; /* LE */
    uint32_t flags; /* LE */
    uint64_t reserved;
};
#pragma pack(pop)

struct ValuePair {
    String key;
    Index<char> value;
};

#define APE_FLAG_HAS_HEADER (1 << 31)
#define APE_FLAG_HAS_NO_FOOTER (1 << 30)
#define APE_FLAG_IS_HEADER (1 << 29)

namespace audtag {

static bool ape_read_header (VFSFile & handle, APEHeader * header)
{
    if (handle.fread (header, 1, sizeof (APEHeader)) != sizeof (APEHeader))
        return false;

    if (strncmp (header->magic, "APETAGEX", 8))
        return false;

    header->version = FROM_LE32 (header->version);
    header->length = FROM_LE32 (header->length);
    header->items = FROM_LE32 (header->items);
    header->flags = FROM_LE32 (header->flags);

    if (header->length < sizeof (APEHeader))
        return false;

    return true;
}

static bool ape_find_header (VFSFile & handle, APEHeader * header,
 int * start, int * length, int * data_start, int * data_length)
{
    APEHeader secondary;

    if (handle.fseek (0, VFS_SEEK_SET))
        return false;

    if (ape_read_header (handle, header))
    {
        AUDDBG ("Found header at 0, length = %d, version = %d.\n",
         (int) header->length, (int) header->version);

        * start = 0;
        * length = header->length;
        * data_start = sizeof (APEHeader);
        * data_length = header->length - sizeof (APEHeader);

        if (! (header->flags & APE_FLAG_HAS_HEADER) || ! (header->flags & APE_FLAG_IS_HEADER))
        {
            AUDWARN ("Invalid header flags (%u).\n", (unsigned) header->flags);
            return false;
        }

        if (! (header->flags & APE_FLAG_HAS_NO_FOOTER))
        {
            if (handle.fseek (header->length, VFS_SEEK_CUR))
                return false;

            if (! ape_read_header (handle, & secondary))
            {
                AUDWARN ("Expected footer, but found none.\n");
                return false;
            }

            * length += sizeof (APEHeader);
        }

        return true;
    }

    if (handle.fseek (-(int) sizeof (APEHeader), VFS_SEEK_END))
        return false;

    if (! ape_read_header (handle, header))
    {
        /* APE tag may be followed by an ID3v1 tag */
        if (handle.fseek (-128 - (int) sizeof (APEHeader), VFS_SEEK_END))
            return false;

        if (! ape_read_header (handle, header))
        {
            AUDDBG ("No header found.\n");
            return false;
        }
    }

    AUDDBG ("Found footer at %d, length = %d, version = %d.\n",
     (int) handle.ftell () - (int) sizeof (APEHeader), (int) header->length,
     (int) header->version);

    * start = handle.ftell () - header->length;
    * length = header->length;
    * data_start = handle.ftell () - header->length;
    * data_length = header->length - sizeof (APEHeader);

    if ((header->flags & APE_FLAG_HAS_NO_FOOTER) || (header->flags & APE_FLAG_IS_HEADER))
    {
        AUDWARN ("Invalid footer flags (%u).\n", (unsigned) header->flags);
        return false;
    }

    if (header->flags & APE_FLAG_HAS_HEADER)
    {
        if (handle.fseek (-(int) header->length - sizeof (APEHeader), VFS_SEEK_CUR))
            return false;

        if (! ape_read_header (handle, & secondary))
        {
            AUDDBG ("Expected header, but found none.\n");
            return false;
        }

        * start -= sizeof (APEHeader);
        * length += sizeof (APEHeader);
    }

    return true;
}

bool APETagModule::can_handle_file (VFSFile & handle)
{
    APEHeader header;
    int start, length, data_start, data_length;

    return ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length);
}

/* returns start of next item or nullptr */
static const char * ape_read_item (const char * data, int length, ValuePair & pair)
{
    auto header = (const uint32_t *) data;
    const char * value;

    if (length < 8)
    {
        AUDWARN ("Expected item, but only %d bytes remain in tag.\n", length);
        return nullptr;
    }

    value = (const char *) memchr (data + 8, 0, length - 8);

    if (! value)
    {
        AUDWARN ("Unterminated item key (max length = %d).\n", length - 8);
        return nullptr;
    }

    value ++;

    uint32_t value_len = FROM_LE32 (header[0]);
    if (value_len > (unsigned) (data + length - value))
    {
        AUDWARN ("Item value of length %d, but only %d bytes remain in tag.\n",
         (int) value_len, (int) (data + length - value));
        return nullptr;
    }

    pair.key = String (data + 8);
    pair.value.insert (value, -1, value_len);
    pair.value.append (0);

    return value + value_len;
}

static Index<ValuePair> ape_read_items (VFSFile & handle)
{
    Index<ValuePair> list;
    APEHeader header;
    int start, length, data_start, data_length;

    if (! ape_find_header (handle, & header, & start, & length, & data_start, & data_length))
        return list;

    if (handle.fseek (data_start, VFS_SEEK_SET))
        return list;

    Index<char> data;
    data.insert (0, data_length);

    if (handle.fread (data.begin (), 1, data_length) != data_length)
        return list;

    AUDDBG ("Reading %d items:\n", header.items);
    const char * item = data.begin ();

    while (header.items --)
    {
        ValuePair pair;
        if (! (item = ape_read_item (item, data.end () - item, pair)))
            break;

        AUDDBG ("Read: %s = %s.\n", (const char *) pair.key, pair.value.begin ());
        list.append (std::move (pair));
    }

    return list;
}

bool APETagModule::read_tag (VFSFile & handle, Tuple & tuple, Index<char> * image)
{
    Index<ValuePair> list = ape_read_items (handle);

    for (const ValuePair & pair : list)
    {
        if (! strcmp_nocase (pair.key, "Artist"))
            tuple.set_str (Tuple::Artist, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Title"))
            tuple.set_str (Tuple::Title, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Album"))
            tuple.set_str (Tuple::Album, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Album Artist"))
            tuple.set_str (Tuple::AlbumArtist, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Comment"))
            tuple.set_str (Tuple::Comment, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Genre"))
            tuple.set_str (Tuple::Genre, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "Track"))
            tuple.set_int (Tuple::Track, atoi (pair.value.begin ()));
        else if (! strcmp_nocase (pair.key, "Disc"))
            tuple.set_int (Tuple::Disc, atoi (pair.value.begin ()));
        else if (! strcmp_nocase (pair.key, "Year"))
            tuple.set_int (Tuple::Year, atoi (pair.value.begin ()));
        else if (! strcmp_nocase (pair.key, "REPLAYGAIN_TRACK_GAIN"))
            tuple.set_gain (Tuple::TrackGain, Tuple::GainDivisor, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "REPLAYGAIN_TRACK_PEAK"))
            tuple.set_gain (Tuple::TrackPeak, Tuple::PeakDivisor, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "REPLAYGAIN_ALBUM_GAIN"))
            tuple.set_gain (Tuple::AlbumGain, Tuple::GainDivisor, pair.value.begin ());
        else if (! strcmp_nocase (pair.key, "REPLAYGAIN_ALBUM_PEAK"))
            tuple.set_gain (Tuple::AlbumPeak, Tuple::PeakDivisor, pair.value.begin ());
    }

    return true;
}

static bool ape_write_item (VFSFile & handle, const char * key,
 const char * value, int * written_length)
{
    int key_len = strlen (key) + 1;
    int value_len = strlen (value);
    uint32_t header[2];

    AUDDBG ("Write: %s = %s.\n", key, value);

    header[0] = TO_LE32 (value_len);
    header[1] = 0;

    if (handle.fwrite (header, 1, 8) != 8)
        return false;

    if (handle.fwrite (key, 1, key_len) != key_len)
        return false;

    if (handle.fwrite (value, 1, value_len) != value_len)
        return false;

    * written_length += 8 + key_len + value_len;
    return true;
}

static bool write_string_item (const Tuple & tuple, Tuple::Field field,
 VFSFile & handle, const char * key, int * written_length, int * written_items)
{
    String value = tuple.get_str (field);

    if (! value)
        return true;

    bool success = ape_write_item (handle, key, value, written_length);

    if (success)
        (* written_items) ++;

    return success;
}

static bool write_integer_item (const Tuple & tuple, Tuple::Field field,
 VFSFile & handle, const char * key, int * written_length, int * written_items)
{
    int value = tuple.get_int (field);

    if (value <= 0)
        return true;

    if (! ape_write_item (handle, key, int_to_str (value), written_length))
        return false;

    (* written_items) ++;
    return true;
}

static bool write_header (int data_length, int items, bool is_header,
 VFSFile & handle)
{
    APEHeader header;

    memcpy (header.magic, "APETAGEX", 8);
    header.version = TO_LE32 (2000);
    header.length = TO_LE32 (data_length + sizeof (APEHeader));
    header.items = TO_LE32 (items);
    header.flags = is_header ? TO_LE32 (APE_FLAG_HAS_HEADER |
     APE_FLAG_IS_HEADER) : TO_LE32 (APE_FLAG_HAS_HEADER);
    header.reserved = 0;

    return handle.fwrite (& header, 1, sizeof (APEHeader)) == sizeof (APEHeader);
}

bool APETagModule::write_tag (VFSFile & handle, const Tuple & tuple)
{
    Index<ValuePair> list = ape_read_items (handle);
    APEHeader header;
    int start, length, data_start, data_length, items;

    if (ape_find_header (handle, & header, & start, & length, & data_start, & data_length))
    {
        if (start + length != handle.fsize ())
        {
            AUDERR ("Writing tags is only supported at end of file.\n");
            return false;
        }

        if (handle.ftruncate (start))
            return false;
    }
    else
    {
        start = handle.fsize ();

        if (start < 0)
            return false;
    }

    if (handle.fseek (start, VFS_SEEK_SET) || ! write_header (0, 0, true, handle))
        return false;

    length = 0;
    items = 0;

    if (! write_string_item (tuple, Tuple::Artist, handle, "Artist", & length, & items) ||
     ! write_string_item (tuple, Tuple::Title, handle, "Title", & length, & items) ||
     ! write_string_item (tuple, Tuple::Album, handle, "Album", & length, & items) ||
     ! write_string_item (tuple, Tuple::AlbumArtist, handle, "Album Artist", & length, & items) ||
     ! write_string_item (tuple, Tuple::Comment, handle, "Comment", & length, & items) ||
     ! write_string_item (tuple, Tuple::Genre, handle, "Genre", & length, & items) ||
     ! write_integer_item (tuple, Tuple::Track, handle, "Track", & length, & items) ||
     ! write_integer_item (tuple, Tuple::Disc, handle, "Disc", & length, & items) ||
     ! write_integer_item (tuple, Tuple::Year, handle, "Year", & length, & items))
        return false;

    for (const ValuePair & pair : list)
    {
        if (! strcmp_nocase (pair.key, "Artist") || ! strcmp_nocase (pair.key, "Title") ||
         ! strcmp_nocase (pair.key, "Album") || ! strcmp_nocase (pair.key, "Album Artist") ||
         ! strcmp_nocase (pair.key, "Comment") || ! strcmp_nocase (pair.key, "Genre") ||
         ! strcmp_nocase (pair.key, "Track") || ! strcmp_nocase (pair.key, "Disc") ||
         ! strcmp_nocase (pair.key, "Year"))
            continue;

        if (! ape_write_item (handle, pair.key, pair.value.begin (), & length))
            return false;

        items ++;
    }

    AUDDBG ("Wrote %d items, %d bytes.\n", items, length);

    if (! write_header (length, items, false, handle))
        return false;

    if (handle.fseek (start, VFS_SEEK_SET) < 0)
        return false;

    if (! write_header (length, items, true, handle))
        return false;

    return true;
}

}
