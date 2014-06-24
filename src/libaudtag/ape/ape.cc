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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/runtime.h>
#include <libaudcore/vfs.h>

#include "ape.h"

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
    String key, value;
};

#define APE_FLAG_HAS_HEADER (1 << 31)
#define APE_FLAG_HAS_NO_FOOTER (1 << 30)
#define APE_FLAG_IS_HEADER (1 << 29)

static bool ape_read_header (VFSFile * handle, APEHeader * header)
{
    if (vfs_fread (header, 1, sizeof (APEHeader), handle) != sizeof (APEHeader))
        return false;

    if (strncmp (header->magic, "APETAGEX", 8))
        return false;

    header->version = GUINT32_FROM_LE (header->version);
    header->length = GUINT32_FROM_LE (header->length);
    header->items = GUINT32_FROM_LE (header->items);
    header->flags = GUINT32_FROM_LE (header->flags);

    if (header->length < sizeof (APEHeader))
        return false;

    return true;
}

static bool ape_find_header (VFSFile * handle, APEHeader * header,
 int * start, int * length, int * data_start, int * data_length)
{
    APEHeader secondary;

    if (vfs_fseek (handle, 0, SEEK_SET))
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
            AUDDBG ("Invalid header flags (%u).\n", (unsigned int) header->flags);
            return false;
        }

        if (! (header->flags & APE_FLAG_HAS_NO_FOOTER))
        {
            if (vfs_fseek (handle, header->length, SEEK_CUR))
                return false;

            if (! ape_read_header (handle, & secondary))
            {
                AUDDBG ("Expected footer, but found none.\n");
                return false;
            }

            * length += sizeof (APEHeader);
        }

        return true;
    }

    if (vfs_fseek (handle, -(int) sizeof (APEHeader), SEEK_END))
        return false;

    if (! ape_read_header (handle, header))
    {
        /* APE tag may be followed by an ID3v1 tag */
        if (vfs_fseek (handle, -128 - (int) sizeof (APEHeader), SEEK_END))
            return false;

        if (! ape_read_header (handle, header))
        {
            AUDDBG ("No header found.\n");
            return false;
        }
    }

    AUDDBG ("Found footer at %d, length = %d, version = %d.\n",
     (int) vfs_ftell (handle) - (int) sizeof (APEHeader), (int) header->length,
     (int) header->version);

    * start = vfs_ftell (handle) - header->length;
    * length = header->length;
    * data_start = vfs_ftell (handle) - header->length;
    * data_length = header->length - sizeof (APEHeader);

    if ((header->flags & APE_FLAG_HAS_NO_FOOTER) || (header->flags & APE_FLAG_IS_HEADER))
    {
        AUDDBG ("Invalid footer flags (%u).\n", (unsigned) header->flags);
        return false;
    }

    if (header->flags & APE_FLAG_HAS_HEADER)
    {
        if (vfs_fseek (handle, -(int) header->length - sizeof (APEHeader), SEEK_CUR))
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

static bool ape_is_our_file (VFSFile * handle)
{
    APEHeader header;
    int start, length, data_start, data_length;

    return ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length);
}

static bool ape_read_item (void * * data, int length, ValuePair & pair)
{
    uint32_t * header = (uint32_t *) * data;
    char * value;

    if (length < 8)
    {
        AUDDBG ("Expected item, but only %d bytes remain in tag.\n", length);
        return false;
    }

    value = (char *) memchr ((char *) (* data) + 8, 0, length - 8);

    if (value == nullptr)
    {
        AUDDBG ("Unterminated item key (max length = %d).\n", length - 8);
        return false;
    }

    value ++;

    if (header[0] > (unsigned) ((char *) (* data) + length - value))
    {
        AUDDBG ("Item value of length %d, but only %d bytes remain in tag.\n",
         (int) header[0], (int) ((char *) (* data) + length - value));
        return false;
    }

    pair.key = String ((char *) (* data) + 8);
    pair.value = String (str_copy (value, header[0]));

    * data = value + header[0];

    return true;
}

static Index<ValuePair> ape_read_items (VFSFile * handle)
{
    Index<ValuePair> list;
    APEHeader header;
    int start, length, data_start, data_length;
    void * data, * item;

    if (! ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length))
        return list;

    if (vfs_fseek (handle, data_start, SEEK_SET))
        return list;

    data = g_malloc (data_length);

    if (vfs_fread (data, 1, data_length, handle) != data_length)
    {
        g_free (data);
        return list;
    }

    AUDDBG ("Reading %d items:\n", header.items);
    item = data;

    while (header.items --)
    {
        ValuePair pair;
        if (! ape_read_item (& item, (char *) data + data_length - (char *) item, pair))
            break;

        AUDDBG ("Read: %s = %s.\n", (const char *) pair.key, (const char *) pair.value);
        list.append (std::move (pair));
    }

    g_free (data);
    return list;
}

static void parse_gain_text (const char * text, int * value, int * unit)
{
    int sign = 1;

    * value = 0;
    * unit = 1;

    if (* text == '-')
    {
        sign = -1;
        text ++;
    }

    while (* text >= '0' && * text <= '9')
    {
        * value = * value * 10 + (* text - '0');
        text ++;
    }

    if (* text == '.')
    {
        text ++;

        while (* text >= '0' && * text <= '9' && * value < G_MAXINT / 10)
        {
            * value = * value * 10 + (* text - '0');
            * unit = * unit * 10;
            text ++;
        }
    }

    * value = * value * sign;
}

static void set_gain_info (Tuple & tuple, int field, int unit_field,
 const char * text)
{
    int value, unit;

    parse_gain_text (text, & value, & unit);

    if (tuple.get_value_type (unit_field) == TUPLE_INT)
        value = value * (int64_t) tuple.get_int (unit_field) / unit;
    else
        tuple.set_int (unit_field, unit);

    tuple.set_int (field, value);
}

static bool ape_read_tag (Tuple & tuple, VFSFile * handle)
{
    Index<ValuePair> list = ape_read_items (handle);

    for (const ValuePair & pair : list)
    {
        if (! strcmp (pair.key, "Artist"))
            tuple.set_str (FIELD_ARTIST, pair.value);
        else if (! strcmp (pair.key, "Title"))
            tuple.set_str (FIELD_TITLE, pair.value);
        else if (! strcmp (pair.key, "Album"))
            tuple.set_str (FIELD_ALBUM, pair.value);
        else if (! strcmp (pair.key, "Comment"))
            tuple.set_str (FIELD_COMMENT, pair.value);
        else if (! strcmp (pair.key, "Genre"))
            tuple.set_str (FIELD_GENRE, pair.value);
        else if (! strcmp (pair.key, "Track"))
            tuple.set_int (FIELD_TRACK_NUMBER, atoi (pair.value));
        else if (! strcmp (pair.key, "Year"))
            tuple.set_int (FIELD_YEAR, atoi (pair.value));
        else if (! g_ascii_strcasecmp (pair.key, "REPLAYGAIN_TRACK_GAIN"))
            set_gain_info (tuple, FIELD_GAIN_TRACK_GAIN, FIELD_GAIN_GAIN_UNIT, pair.value);
        else if (! g_ascii_strcasecmp (pair.key, "REPLAYGAIN_TRACK_PEAK"))
            set_gain_info (tuple, FIELD_GAIN_TRACK_PEAK, FIELD_GAIN_PEAK_UNIT, pair.value);
        else if (! g_ascii_strcasecmp (pair.key, "REPLAYGAIN_ALBUM_GAIN"))
            set_gain_info (tuple, FIELD_GAIN_ALBUM_GAIN, FIELD_GAIN_GAIN_UNIT, pair.value);
        else if (! g_ascii_strcasecmp (pair.key, "REPLAYGAIN_ALBUM_PEAK"))
            set_gain_info (tuple, FIELD_GAIN_ALBUM_PEAK, FIELD_GAIN_PEAK_UNIT, pair.value);
    }

    return true;
}

static bool ape_write_item (VFSFile * handle, const char * key,
 const char * value, int * written_length)
{
    int key_len = strlen (key) + 1;
    int value_len = strlen (value);
    uint32_t header[2];

    AUDDBG ("Write: %s = %s.\n", key, value);

    header[0] = GUINT32_TO_LE (value_len);
    header[1] = 0;

    if (vfs_fwrite (header, 1, 8, handle) != 8)
        return false;

    if (vfs_fwrite (key, 1, key_len, handle) != key_len)
        return false;

    if (vfs_fwrite (value, 1, value_len, handle) != value_len)
        return false;

    * written_length += 8 + key_len + value_len;
    return true;
}

static bool write_string_item (const Tuple & tuple, int field, VFSFile *
 handle, const char * key, int * written_length, int * written_items)
{
    String value = tuple.get_str (field);

    if (value == nullptr)
        return true;

    bool success = ape_write_item (handle, key, value, written_length);

    if (success)
        (* written_items) ++;

    return success;
}

static bool write_integer_item (const Tuple & tuple, int field, VFSFile *
 handle, const char * key, int * written_length, int * written_items)
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
 VFSFile * handle)
{
    APEHeader header;

    memcpy (header.magic, "APETAGEX", 8);
    header.version = GUINT32_TO_LE (2000);
    header.length = GUINT32_TO_LE (data_length + sizeof (APEHeader));
    header.items = GUINT32_TO_LE (items);
    header.flags = is_header ? GUINT32_TO_LE (APE_FLAG_HAS_HEADER |
     APE_FLAG_IS_HEADER) : GUINT32_TO_LE (APE_FLAG_HAS_HEADER);
    header.reserved = 0;

    return vfs_fwrite (& header, 1, sizeof (APEHeader), handle) == sizeof
     (APEHeader);
}

static bool ape_write_tag (const Tuple & tuple, VFSFile * handle)
{
    Index<ValuePair> list = ape_read_items (handle);
    APEHeader header;
    int start, length, data_start, data_length, items;

    if (ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length))
    {
        if (start + length != vfs_fsize (handle))
        {
            AUDDBG ("Writing tags is only supported at end of file.\n");
            return false;
        }

        if (vfs_ftruncate (handle, start))
            return false;
    }
    else
    {
        start = vfs_fsize (handle);

        if (start < 0)
            return false;
    }

    if (vfs_fseek (handle, start, SEEK_SET) || ! write_header (0, 0, true, handle))
        return false;

    length = 0;
    items = 0;

    if (! write_string_item (tuple, FIELD_ARTIST, handle, "Artist", & length, & items) ||
     ! write_string_item (tuple, FIELD_TITLE, handle, "Title", & length, & items) ||
     ! write_string_item (tuple, FIELD_ALBUM, handle, "Album", & length, & items) ||
     ! write_string_item (tuple, FIELD_COMMENT, handle, "Comment", & length, & items) ||
     ! write_string_item (tuple, FIELD_GENRE, handle, "Genre", & length, & items) ||
     ! write_integer_item (tuple, FIELD_TRACK_NUMBER, handle, "Track", & length, & items) ||
     ! write_integer_item (tuple, FIELD_YEAR, handle, "Year", & length, & items))
        return false;

    for (const ValuePair & pair : list)
    {
        if (! strcmp (pair.key, "Artist") || ! strcmp (pair.key, "Title") ||
         ! strcmp (pair.key, "Album") || ! strcmp (pair.key, "Comment") ||
         ! strcmp (pair.key, "Genre") || ! strcmp (pair.key, "Track") ||
         ! strcmp (pair.key, "Year"))
            continue;

        if (! ape_write_item (handle, pair.key, pair.value, & length))
            return false;

        items ++;
    }

    AUDDBG ("Wrote %d items, %d bytes.\n", items, length);

    if (! write_header (length, items, false, handle) || vfs_fseek (handle,
     start, SEEK_SET) || ! write_header (length, items, true, handle))
        return false;

    return true;
}

tag_module_t ape =
{
    "APE",
    TAG_TYPE_APE,
    ape_is_our_file,
    ape_read_tag,
    0,  // read_image
    ape_write_tag
};
